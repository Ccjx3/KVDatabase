/* ************************************************************************
> File Name:     skiplist_mvcc.h
> Description:   支持MVCC的跳表实现
>                1. 多版本并发控制（MVCC）
>                2. 事务隔离级别：读已提交（Read Committed）
>                3. 支持事务的ACID特性
>                4. 基于时间戳的版本管理
 ************************************************************************/

#ifndef SKIPLIST_MVCC_H
#define SKIPLIST_MVCC_H

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <fstream>
#include <mutex>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>

#define STORE_FILE_MVCC "store/dumpFile_mvcc"

// 事务状态枚举
enum class TransactionState {
    ACTIVE,      // 活跃状态
    COMMITTED,   // 已提交
    ABORTED      // 已回滚
};

// 版本记录结构
template<typename K, typename V>
struct Version {
    V value;                    // 值
    uint64_t create_ts;         // 创建时间戳（事务ID）
    uint64_t delete_ts;         // 删除时间戳（MAX表示未删除）
    bool is_committed;          // 创建该版本的事务是否已提交
    std::shared_ptr<Version<K, V>> next;  // 指向下一个旧版本
    
    Version(V v, uint64_t create, uint64_t del = UINT64_MAX) 
        : value(v), create_ts(create), delete_ts(del), is_committed(false), next(nullptr) {}
    
    // 判断版本对指定事务是否可见（读已提交隔离级别）
    bool is_visible(uint64_t txn_id) const {
        // 1. 如果是当前事务创建的版本，直接可见
        if (create_ts == txn_id) {
            return delete_ts > txn_id;
        }
        // 2. 如果是其他事务创建的版本，必须已提交才可见
        return is_committed && create_ts < txn_id && delete_ts > txn_id;
    }
};

// MVCC节点结构
template<typename K, typename V>
class NodeMVCC {
public:
    NodeMVCC() {}
    NodeMVCC(K k, int level);
    ~NodeMVCC();
    
    K get_key() const;
    
    // 版本链管理
    void add_version(V value, uint64_t txn_id);
    std::shared_ptr<Version<K, V>> get_visible_version(uint64_t txn_id);
    void mark_deleted(uint64_t txn_id);
    void commit_version(uint64_t txn_id);  // 标记版本为已提交
    
    // 垃圾回收：清理对所有活跃事务都不可见的旧版本
    void gc_versions(uint64_t min_active_txn_id);
    
    NodeMVCC<K, V> **forward;
    int node_level;
    
private:
    K key;
    std::shared_ptr<Version<K, V>> version_head;  // 版本链头（最新版本）
    std::mutex version_mutex;  // 保护版本链的互斥锁
};

template<typename K, typename V>
NodeMVCC<K, V>::NodeMVCC(K k, int level) {
    this->key = k;
    this->node_level = level;
    this->forward = new NodeMVCC<K, V>*[level + 1];
    memset(this->forward, 0, sizeof(NodeMVCC<K, V>*) * (level + 1));
    this->version_head = nullptr;
}

template<typename K, typename V>
NodeMVCC<K, V>::~NodeMVCC() {
    delete[] forward;
}

template<typename K, typename V>
K NodeMVCC<K, V>::get_key() const {
    return key;
}

template<typename K, typename V>
void NodeMVCC<K, V>::add_version(V value, uint64_t txn_id) {
    std::lock_guard<std::mutex> lock(version_mutex);
    auto new_version = std::make_shared<Version<K, V>>(value, txn_id);
    new_version->next = version_head;
    version_head = new_version;
}

template<typename K, typename V>
std::shared_ptr<Version<K, V>> NodeMVCC<K, V>::get_visible_version(uint64_t txn_id) {
    std::lock_guard<std::mutex> lock(version_mutex);
    auto current = version_head;
    while (current != nullptr) {
        if (current->is_visible(txn_id)) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

template<typename K, typename V>
void NodeMVCC<K, V>::mark_deleted(uint64_t txn_id) {
    std::lock_guard<std::mutex> lock(version_mutex);
    if (version_head != nullptr) {
        version_head->delete_ts = txn_id;
    }
}

template<typename K, typename V>
void NodeMVCC<K, V>::commit_version(uint64_t txn_id) {
    std::lock_guard<std::mutex> lock(version_mutex);
    auto current = version_head;
    while (current != nullptr) {
        if (current->create_ts == txn_id) {
            current->is_committed = true;
        }
        current = current->next;
    }
}

template<typename K, typename V>
void NodeMVCC<K, V>::gc_versions(uint64_t min_active_txn_id) {
    std::lock_guard<std::mutex> lock(version_mutex);
    
    if (version_head == nullptr) return;
    
    // 保留第一个版本（最新版本）
    auto current = version_head->next;
    auto prev = version_head;
    
    while (current != nullptr) {
        // 如果版本对所有活跃事务都不可见，则可以回收
        if (current->delete_ts < min_active_txn_id) {
            prev->next = current->next;
            current = prev->next;
        } else {
            prev = current;
            current = current->next;
        }
    }
}

// 事务描述符
template<typename K, typename V>
class Transaction {
public:
    uint64_t txn_id;
    TransactionState state;
    std::chrono::steady_clock::time_point start_time;
    std::vector<NodeMVCC<K, V>*> modified_nodes;  // 记录修改的节点
    
    Transaction(uint64_t id) 
        : txn_id(id), 
          state(TransactionState::ACTIVE),
          start_time(std::chrono::steady_clock::now()) {}
    
    void commit() {
        state = TransactionState::COMMITTED;
    }
    
    void abort() {
        state = TransactionState::ABORTED;
    }
    
    bool is_active() const {
        return state == TransactionState::ACTIVE;
    }
    
    void add_modified_node(NodeMVCC<K, V>* node) {
        modified_nodes.push_back(node);
    }
};

// 支持MVCC的跳表
template<typename K, typename V>
class SkipListMVCC {
public:
    SkipListMVCC(int max_level, bool silent = false);
    ~SkipListMVCC();
    
    void set_silent(bool silent) { _silent = silent; }
    
    // 事务管理
    std::shared_ptr<Transaction<K, V>> begin_transaction();
    bool commit_transaction(std::shared_ptr<Transaction<K, V>> txn);
    void abort_transaction(std::shared_ptr<Transaction<K, V>> txn);
    
    // 事务操作（需要传入事务对象）
    int insert_element(std::shared_ptr<Transaction<K, V>> txn, K key, V value);
    bool search_element(std::shared_ptr<Transaction<K, V>> txn, K key, V* value);
    void delete_element(std::shared_ptr<Transaction<K, V>> txn, K key);
    
    // 范围查询
    std::vector<std::pair<K, V>> range_query(std::shared_ptr<Transaction<K, V>> txn, K start_key, K end_key);
    
    // 显示和持久化
    void display_list();
    void dump_file();
    void load_file();
    int size();
    
    // 垃圾回收
    void gc();
    
    // 统计信息
    void print_stats();
    
private:
    int get_random_level();
    NodeMVCC<K, V>* create_node(K key, int level);
    void clear(NodeMVCC<K, V>* node);
    uint64_t get_min_active_txn_id();
    
private:
    int _max_level;
    int _skip_list_level;
    NodeMVCC<K, V>* _header;
    
    // 事务管理
    std::atomic<uint64_t> _next_txn_id;
    std::unordered_map<uint64_t, std::shared_ptr<Transaction<K, V>>> _active_transactions;
    std::mutex _txn_mutex;
    
    // 统计信息
    std::atomic<uint64_t> _total_commits;
    std::atomic<uint64_t> _total_aborts;
    std::atomic<uint64_t> _total_versions;
    
    std::mutex _global_mutex;
    std::ofstream _file_writer;
    std::ifstream _file_reader;
    
    bool _silent;  // 静默模式
};

template<typename K, typename V>
SkipListMVCC<K, V>::SkipListMVCC(int max_level, bool silent) 
    : _max_level(max_level),
      _skip_list_level(0),
      _next_txn_id(1),
      _total_commits(0),
      _total_aborts(0),
      _total_versions(0),
      _silent(silent) {
    K k;
    this->_header = new NodeMVCC<K, V>(k, _max_level);
}

template<typename K, typename V>
SkipListMVCC<K, V>::~SkipListMVCC() {
    if (_file_writer.is_open()) {
        _file_writer.close();
    }
    if (_file_reader.is_open()) {
        _file_reader.close();
    }
    
    if (_header->forward[0] != nullptr) {
        clear(_header->forward[0]);
    }
    delete _header;
}

template<typename K, typename V>
void SkipListMVCC<K, V>::clear(NodeMVCC<K, V>* node) {
    if (node->forward[0] != nullptr) {
        clear(node->forward[0]);
    }
    delete node;
}

template<typename K, typename V>
int SkipListMVCC<K, V>::get_random_level() {
    int k = 1;
    while (rand() % 2) {
        k++;
    }
    k = (k < _max_level) ? k : _max_level;
    return k;
}

template<typename K, typename V>
NodeMVCC<K, V>* SkipListMVCC<K, V>::create_node(K key, int level) {
    return new NodeMVCC<K, V>(key, level);
}

// 开始事务
template<typename K, typename V>
std::shared_ptr<Transaction<K, V>> SkipListMVCC<K, V>::begin_transaction() {
    uint64_t txn_id = _next_txn_id.fetch_add(1);
    auto txn = std::make_shared<Transaction<K, V>>(txn_id);
    
    std::lock_guard<std::mutex> lock(_txn_mutex);
    _active_transactions[txn_id] = txn;
    
    if (!_silent) {
        std::cout << "[TXN " << txn_id << "] BEGIN" << std::endl;
    }
    return txn;
}

// 提交事务
template<typename K, typename V>
bool SkipListMVCC<K, V>::commit_transaction(std::shared_ptr<Transaction<K, V>> txn) {
    if (!txn || !txn->is_active()) {
        return false;
    }
    
    // 标记所有修改的版本为已提交
    for (auto node : txn->modified_nodes) {
        node->commit_version(txn->txn_id);
    }
    
    txn->commit();
    
    {
        std::lock_guard<std::mutex> lock(_txn_mutex);
        _active_transactions.erase(txn->txn_id);
    }
    
    _total_commits.fetch_add(1);
    if (!_silent) {
        std::cout << "[TXN " << txn->txn_id << "] COMMIT" << std::endl;
    }
    return true;
}

// 回滚事务
template<typename K, typename V>
void SkipListMVCC<K, V>::abort_transaction(std::shared_ptr<Transaction<K, V>> txn) {
    if (!txn || !txn->is_active()) {
        return;
    }
    
    txn->abort();
    
    {
        std::lock_guard<std::mutex> lock(_txn_mutex);
        _active_transactions.erase(txn->txn_id);
    }
    
    _total_aborts.fetch_add(1);
    if (!_silent) {
        std::cout << "[TXN " << txn->txn_id << "] ABORT" << std::endl;
    }
}

// 插入元素
template<typename K, typename V>
int SkipListMVCC<K, V>::insert_element(std::shared_ptr<Transaction<K, V>> txn, K key, V value) {
    if (!txn || !txn->is_active()) {
        std::cout << "Transaction is not active!" << std::endl;
        return -1;
    }
    
    std::lock_guard<std::mutex> lock(_global_mutex);
    
    NodeMVCC<K, V>* current = _header;
    std::vector<NodeMVCC<K, V>*> update(_max_level + 1, nullptr);
    
    // 查找插入位置
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] != nullptr && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    
    current = current->forward[0];
    
    // 如果key已存在，添加新版本
    if (current != nullptr && current->get_key() == key) {
        current->add_version(value, txn->txn_id);
        txn->add_modified_node(current);  // 记录修改的节点
        _total_versions.fetch_add(1);
        if (!_silent) {
            std::cout << "[TXN " << txn->txn_id << "] UPDATE key:" << key << ", value:" << value << std::endl;
        }
        return 0;
    }
    
    // 插入新节点
    int random_level = get_random_level();
    
    if (random_level > _skip_list_level) {
        for (int i = _skip_list_level + 1; i < random_level + 1; i++) {
            update[i] = _header;
        }
        _skip_list_level = random_level;
    }
    
    NodeMVCC<K, V>* new_node = create_node(key, random_level);
    new_node->add_version(value, txn->txn_id);
    txn->add_modified_node(new_node);  // 记录修改的节点
    _total_versions.fetch_add(1);
    
    for (int i = 0; i <= random_level; i++) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }
    
    if (!_silent) {
        std::cout << "[TXN " << txn->txn_id << "] INSERT key:" << key << ", value:" << value << std::endl;
    }
    return 0;
}

// 查找元素
template<typename K, typename V>
bool SkipListMVCC<K, V>::search_element(std::shared_ptr<Transaction<K, V>> txn, K key, V* value) {
    if (!txn || !txn->is_active()) {
        std::cout << "Transaction is not active!" << std::endl;
        return false;
    }
    
    NodeMVCC<K, V>* current = _header;
    
    // 查找key
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
    }
    
    current = current->forward[0];
    
    if (current && current->get_key() == key) {
        // 获取对当前事务可见的版本
        auto version = current->get_visible_version(txn->txn_id);
        if (version != nullptr) {
            *value = version->value;
            if (!_silent) {
                std::cout << "[TXN " << txn->txn_id << "] FOUND key:" << key << ", value:" << *value << std::endl;
            }
            return true;
        }
    }
    
    if (!_silent) {
        std::cout << "[TXN " << txn->txn_id << "] NOT FOUND key:" << key << std::endl;
    }
    return false;
}

// 删除元素
template<typename K, typename V>
void SkipListMVCC<K, V>::delete_element(std::shared_ptr<Transaction<K, V>> txn, K key) {
    if (!txn || !txn->is_active()) {
        std::cout << "Transaction is not active!" << std::endl;
        return;
    }
    
    std::lock_guard<std::mutex> lock(_global_mutex);
    
    NodeMVCC<K, V>* current = _header;
    
    // 查找key
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
    }
    
    current = current->forward[0];
    
    if (current && current->get_key() == key) {
        // 标记删除（不是物理删除）
        current->mark_deleted(txn->txn_id);
        if (!_silent) {
            std::cout << "[TXN " << txn->txn_id << "] DELETE key:" << key << std::endl;
        }
    }
}

// 范围查询
template<typename K, typename V>
std::vector<std::pair<K, V>> SkipListMVCC<K, V>::range_query(
    std::shared_ptr<Transaction<K, V>> txn, K start_key, K end_key) {
    
    std::vector<std::pair<K, V>> result;
    
    if (!txn || !txn->is_active()) {
        std::cout << "Transaction is not active!" << std::endl;
        return result;
    }
    
    if (start_key > end_key) {
        return result;
    }
    
    NodeMVCC<K, V>* current = _header;
    
    // 找到起始位置
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] != nullptr && current->forward[i]->get_key() < start_key) {
            current = current->forward[i];
        }
    }
    
    current = current->forward[0];
    
    // 收集范围内的可见版本
    while (current != nullptr && current->get_key() <= end_key) {
        auto version = current->get_visible_version(txn->txn_id);
        if (version != nullptr) {
            result.push_back(std::make_pair(current->get_key(), version->value));
        }
        current = current->forward[0];
    }
    
    std::cout << "[TXN " << txn->txn_id << "] RANGE_QUERY [" << start_key << ", " << end_key 
              << "] found " << result.size() << " elements" << std::endl;
    return result;
}

// 显示跳表
template<typename K, typename V>
void SkipListMVCC<K, V>::display_list() {
    std::lock_guard<std::mutex> lock(_global_mutex);
    
    std::cout << "\n*****Skip List MVCC*****" << std::endl;
    for (int i = 0; i <= _skip_list_level; i++) {
        NodeMVCC<K, V>* node = _header->forward[i];
        std::cout << "Level " << i << ": ";
        while (node != nullptr) {
            std::cout << node->get_key() << ";";
            node = node->forward[i];
        }
        std::cout << std::endl;
    }
}

// 获取最小活跃事务ID
template<typename K, typename V>
uint64_t SkipListMVCC<K, V>::get_min_active_txn_id() {
    std::lock_guard<std::mutex> lock(_txn_mutex);
    
    if (_active_transactions.empty()) {
        return _next_txn_id.load();
    }
    
    uint64_t min_id = UINT64_MAX;
    for (const auto& pair : _active_transactions) {
        min_id = std::min(min_id, pair.first);
    }
    return min_id;
}

// 垃圾回收
template<typename K, typename V>
void SkipListMVCC<K, V>::gc() {
    std::lock_guard<std::mutex> lock(_global_mutex);
    
    uint64_t min_active_txn_id = get_min_active_txn_id();
    
    NodeMVCC<K, V>* current = _header->forward[0];
    int gc_count = 0;
    
    while (current != nullptr) {
        size_t before = _total_versions.load();
        current->gc_versions(min_active_txn_id);
        size_t after = _total_versions.load();
        gc_count += (before - after);
        current = current->forward[0];
    }
    
    std::cout << "[GC] Collected " << gc_count << " old versions" << std::endl;
}

// 获取元素数量
template<typename K, typename V>
int SkipListMVCC<K, V>::size() {
    std::lock_guard<std::mutex> lock(_global_mutex);
    
    int count = 0;
    NodeMVCC<K, V>* current = _header->forward[0];
    while (current != nullptr) {
        count++;
        current = current->forward[0];
    }
    return count;
}

// 持久化
template<typename K, typename V>
void SkipListMVCC<K, V>::dump_file() {
    std::lock_guard<std::mutex> lock(_global_mutex);
    
    _file_writer.open(STORE_FILE_MVCC);
    NodeMVCC<K, V>* node = _header->forward[0];
    
    // 创建一个临时事务用于读取最新已提交的版本
    auto txn = std::make_shared<Transaction<K, V>>(_next_txn_id.load());
    
    while (node != nullptr) {
        auto version = node->get_visible_version(txn->txn_id);
        if (version != nullptr) {
            _file_writer << node->get_key() << ":" << version->value << "\n";
        }
        node = node->forward[0];
    }
    
    _file_writer.flush();
    _file_writer.close();
    std::cout << "Data dumped to file" << std::endl;
}

// 从文件加载
template<typename K, typename V>
void SkipListMVCC<K, V>::load_file() {
    _file_reader.open(STORE_FILE_MVCC);
    std::cout << "Loading data from file..." << std::endl;
    
    auto txn = begin_transaction();
    std::string line;
    
    while (getline(_file_reader, line)) {
        size_t pos = line.find(":");
        if (pos != std::string::npos) {
            K key = std::stoi(line.substr(0, pos));
            V value = line.substr(pos + 1);
            insert_element(txn, key, value);
        }
    }
    
    commit_transaction(txn);
    _file_reader.close();
}

// 打印统计信息
template<typename K, typename V>
void SkipListMVCC<K, V>::print_stats() {
    std::cout << "\n===== MVCC Statistics =====" << std::endl;
    std::cout << "Total commits: " << _total_commits.load() << std::endl;
    std::cout << "Total aborts: " << _total_aborts.load() << std::endl;
    std::cout << "Total versions: " << _total_versions.load() << std::endl;
    std::cout << "Active transactions: " << _active_transactions.size() << std::endl;
    std::cout << "Skip list size: " << size() << std::endl;
    std::cout << "==========================\n" << std::endl;
}

#endif // SKIPLIST_MVCC_H

