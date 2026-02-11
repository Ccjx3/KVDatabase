/* ************************************************************************
> File Name:     skiplist_optimized.h
> Description:   优化版跳表实现
>                1. 细粒度锁优化 - 分段锁机制
>                2. 内存池优化 - 减少频繁new/delete
 ************************************************************************/

#ifndef SKIPLIST_OPTIMIZED_H
#define SKIPLIST_OPTIMIZED_H

#include <iostream> 
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <fstream>
#include <mutex>
#include "segment_lock.h"
#include "memory_pool.h"

#define STORE_FILE_OPT "store/dumpFile_optimized"

// 前向声明
template<typename K, typename V> 
class NodeMemoryPool;

//Class template to implement optimized node
template<typename K, typename V> 
class NodeOpt {
public:
    NodeOpt() {} 

    NodeOpt(K k, V v, int); 

    ~NodeOpt();

    K get_key() const;

    V get_value() const;

    void set_value(V);
    
    // 为内存池提供的重新设置键值的方法
    void set_key_value(K k, V v);
    
    // Linear array to hold pointers to next node of different level
    NodeOpt<K, V> **forward;

    int node_level;

private:
    K key;
    V value;
    
    // 声明内存池为友元类
    friend class NodeMemoryPool<K, V>;
};

template<typename K, typename V> 
NodeOpt<K, V>::NodeOpt(const K k, const V v, int level) {
    this->key = k;
    this->value = v;
    this->node_level = level; 

    // level + 1, because array index is from 0 - level
    this->forward = new NodeOpt<K, V>*[level+1];
    
    // Fill forward array with 0(NULL) 
    memset(this->forward, 0, sizeof(NodeOpt<K, V>*)*(level+1));
}

template<typename K, typename V> 
NodeOpt<K, V>::~NodeOpt() {
    delete []forward;
}

template<typename K, typename V> 
K NodeOpt<K, V>::get_key() const {
    return key;
}

template<typename K, typename V> 
V NodeOpt<K, V>::get_value() const {
    return value;
}

template<typename K, typename V> 
void NodeOpt<K, V>::set_value(V value) {
    this->value = value;
}

template<typename K, typename V> 
void NodeOpt<K, V>::set_key_value(K k, V v) {
    this->key = k;
    this->value = v;
}

// Class template for Skip list with optimizations
template <typename K, typename V> 
class SkipListOptimized {
public: 
    SkipListOptimized(int max_level, int segment_count = 16);
    ~SkipListOptimized();
    
    int get_random_level();
    NodeOpt<K, V>* create_node(K, V, int);
    int insert_element(K, V);
    void display_list();
    bool search_element(K);
    bool search_element_silent(K);  // 静默查询，不输出信息
    void delete_element(K);
    void dump_file();
    void load_file();
    void clear(NodeOpt<K,V>*);
    int size();
    
    // 获取内存池统计信息
    void print_memory_pool_stats();

private:
    void get_key_value_from_string(const std::string& str, std::string* key, std::string* value);
    bool is_valid_string(const std::string& str);

private:    
    int _max_level;                                      // 跳表最大层级
    int _skip_list_level;                                // 当前跳表层级
    NodeOpt<K, V> *_header;                              // 头节点指针
    std::ofstream _file_writer;                          // 文件写入流
    std::ifstream _file_reader;                          // 文件读取流
    int _element_count;                                  // 元素计数
    
    // 优化模块
    SegmentLockManager<K> _lock_manager;                 // 分段锁管理器
    NodeMemoryPool<K, V> _memory_pool;                   // 内存池
    
    std::mutex _global_mutex;                            // 全局操作的互斥锁（如display_list）
    std::mutex _level_mutex;                             // 保护 _skip_list_level 的互斥锁
    std::mutex _count_mutex;                             // 保护 _element_count 的互斥锁
};

// 构造函数
template<typename K, typename V> 
SkipListOptimized<K, V>::SkipListOptimized(int max_level, int segment_count) 
    : _max_level(max_level),
      _skip_list_level(0),
      _element_count(0),
      _lock_manager(segment_count),
      _memory_pool(100) {
    
    K k;
    V v;
    this->_header = new NodeOpt<K, V>(k, v, _max_level);
}

// 析构函数
template<typename K, typename V> 
SkipListOptimized<K, V>::~SkipListOptimized() {
    if (_file_writer.is_open()) {
        _file_writer.close();
    }
    if (_file_reader.is_open()) {
        _file_reader.close();
    }

    // 递归删除跳表链条
    if(_header->forward[0] != nullptr){
        clear(_header->forward[0]);
    }
    delete(_header);
}

// 递归清理节点
template <typename K, typename V>
void SkipListOptimized<K, V>::clear(NodeOpt<K, V>* cur) {
    if(cur->forward[0] != nullptr){
        clear(cur->forward[0]);
    }
    // 使用内存池回收节点（注意：这里为了彻底清理，直接delete）
    delete(cur);
}

// 使用内存池创建节点
template<typename K, typename V>
NodeOpt<K, V>* SkipListOptimized<K, V>::create_node(const K k, const V v, int level) {
    return _memory_pool.allocate(k, v, level);
}

// 插入元素 - 使用分段锁
template<typename K, typename V>
int SkipListOptimized<K, V>::insert_element(const K key, const V value) {
    // 获取key所属的段索引
    int segment_index = _lock_manager.get_segment_index(key);
    
    // 获取该段的写锁
    auto lock = _lock_manager.get_write_lock(segment_index);
    
    // 同时获取层级锁，避免在遍历过程中层级被其他线程修改
    std::lock_guard<std::mutex> level_lock(_level_mutex);
    
    NodeOpt<K, V> *current = this->_header;
    std::vector<NodeOpt<K, V>*> update(_max_level+1, nullptr);  

    // 从最高层开始查找插入位置
    for(int i = _skip_list_level; i >= 0; i--) {
        while(current->forward[i] != NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i]; 
        }
        update[i] = current;
    }

    current = current->forward[0];

    // 如果key已存在
    if (current != NULL && current->get_key() == key) {
        std::cout << "key: " << key << ", exists" << std::endl;
        return 1;
    }

    // 插入新节点
    if (current == NULL || current->get_key() != key) {
        int random_level = get_random_level();

        // 更新跳表层级
        if (random_level > _skip_list_level) {
            for (int i = _skip_list_level+1; i < random_level+1; i++) {
                update[i] = _header;
            }
            _skip_list_level = random_level;
        }

        // 使用内存池创建节点
        NodeOpt<K, V>* inserted_node = create_node(key, value, random_level);
        
        for (int i = 0; i <= random_level; i++) {
            inserted_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = inserted_node;
        }
        
        std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
        
        // 更新元素计数（需要加锁保护）
        {
            std::lock_guard<std::mutex> count_lock(_count_mutex);
            _element_count++;
        }
    }
    
    return 0;
}

// 查找元素 - 使用分段读锁
template<typename K, typename V> 
bool SkipListOptimized<K, V>::search_element(K key) {
    std::cout << "search_element-----------------" << std::endl;
    
    // 获取key所属的段索引
    int segment_index = _lock_manager.get_segment_index(key);
    
    // 获取该段的读锁（共享锁）
    auto lock = _lock_manager.get_read_lock(segment_index);
    
    // 读取当前层级（快速读取后立即释放锁）
    int current_level;
    {
        std::lock_guard<std::mutex> level_lock(_level_mutex);
        current_level = _skip_list_level;
    }
    
    NodeOpt<K, V> *current = _header;

    for (int i = current_level; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
    }

    current = current->forward[0];

    if (current and current->get_key() == key) {
        std::cout << "Found key: " << key << ", value: " << current->get_value() << std::endl;
        return true;
    }

    std::cout << "Not Found Key:" << key << std::endl;
    return false;
}

// 静默查找元素 - 不输出信息，用于性能测试
template<typename K, typename V> 
bool SkipListOptimized<K, V>::search_element_silent(K key) {
    // 获取key所属的段索引
    int segment_index = _lock_manager.get_segment_index(key);
    
    // 获取该段的读锁（共享锁）
    auto lock = _lock_manager.get_read_lock(segment_index);
    
    // 读取当前层级（快速读取后立即释放锁）
    int current_level;
    {
        std::lock_guard<std::mutex> level_lock(_level_mutex);
        current_level = _skip_list_level;
    }
    
    NodeOpt<K, V> *current = _header;

    for (int i = current_level; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
    }

    current = current->forward[0];

    if (current and current->get_key() == key) {
        return true;
    }

    return false;
}

// 删除元素 - 使用分段锁
template<typename K, typename V> 
void SkipListOptimized<K, V>::delete_element(K key) {
    // 获取key所属的段索引
    int segment_index = _lock_manager.get_segment_index(key);
    
    // 获取该段的写锁
    auto lock = _lock_manager.get_write_lock(segment_index);
    
    // 同时获取层级锁
    std::lock_guard<std::mutex> level_lock(_level_mutex);
    
    NodeOpt<K, V> *current = this->_header; 
    std::vector<NodeOpt<K, V>*> update(_max_level+1, nullptr);

    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] != NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];
    
    if (current != NULL && current->get_key() == key) {
        for (int i = 0; i <= _skip_list_level; i++) {
            if (update[i]->forward[i] != current) 
                break;
            update[i]->forward[i] = current->forward[i];
        }

        // 更新跳表层级
        while (_skip_list_level > 0 && _header->forward[_skip_list_level] == 0) {
            _skip_list_level--; 
        }

        std::cout << "Successfully deleted key "<< key << std::endl;
        
        // 将节点归还到内存池（这里为了简化，直接delete）
        // 在实际应用中，可以归还到内存池复用
        delete current;
        
        // 更新元素计数（需要加锁保护）
        {
            std::lock_guard<std::mutex> count_lock(_count_mutex);
            _element_count--;
        }
    }
}

// 显示跳表 - 需要全局读锁
template<typename K, typename V> 
void SkipListOptimized<K, V>::display_list() {
    std::lock_guard<std::mutex> lock(_global_mutex);
    
    // 读取当前层级（需要加锁保护）
    int current_level;
    {
        std::lock_guard<std::mutex> level_lock(_level_mutex);
        current_level = _skip_list_level;
    }
    
    std::cout << "\n*****Skip List (Optimized)*****"<<"\n"; 
    for (int i = 0; i <= current_level; i++) {
        NodeOpt<K, V> *node = this->_header->forward[i]; 
        std::cout << "Level " << i << ": ";
        while (node != NULL) {
            std::cout << node->get_key() << ":" << node->get_value() << ";";
            node = node->forward[i];
        }
        std::cout << std::endl;
    }
}

// 持久化到文件 - 需要获取所有段的写锁
template<typename K, typename V> 
void SkipListOptimized<K, V>::dump_file() {
    std::cout << "dump_file-----------------" << std::endl;
    
    // 获取所有段的写锁
    auto locks = _lock_manager.get_all_write_locks();
    
    _file_writer.open(STORE_FILE_OPT);
    NodeOpt<K, V> *node = this->_header->forward[0]; 

    while (node != NULL) {
        _file_writer << node->get_key() << ":" << node->get_value() << "\n";
        std::cout << node->get_key() << ":" << node->get_value() << ";\n";
        node = node->forward[0];
    }

    _file_writer.flush();
    _file_writer.close();
}

// 从文件加载
template<typename K, typename V> 
void SkipListOptimized<K, V>::load_file() {
    _file_reader.open(STORE_FILE_OPT);
    std::cout << "load_file-----------------" << std::endl;
    std::string line;
    std::string* key = new std::string();
    std::string* value = new std::string();
    
    while (getline(_file_reader, line)) {
        get_key_value_from_string(line, key, value);
        if (key->empty() || value->empty()) {
            continue;
        }
        insert_element(stoi(*key), *value);
        std::cout << "key:" << *key << " value:" << *value << std::endl;
    }
    
    delete key;
    delete value;
    _file_reader.close();
}

// 获取元素数量
template<typename K, typename V> 
int SkipListOptimized<K, V>::size() { 
    std::lock_guard<std::mutex> count_lock(_count_mutex);
    return _element_count;
}

// 解析字符串获取key-value
template<typename K, typename V>
void SkipListOptimized<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value) {
    if(!is_valid_string(str)) {
        return;
    }
    std::string delim = ":";
    *key = str.substr(0, str.find(delim));
    *value = str.substr(str.find(delim)+1, str.length());
}

// 验证字符串格式
template<typename K, typename V>
bool SkipListOptimized<K, V>::is_valid_string(const std::string& str) {
    if (str.empty()) {
        return false;
    }
    if (str.find(":") == std::string::npos) {
        return false;
    }
    return true;
}

// 获取随机层级
template<typename K, typename V>
int SkipListOptimized<K, V>::get_random_level(){
    int k = 1;
    while (rand() % 2) {
        k++;
    }
    k = (k < _max_level) ? k : _max_level;
    return k;
}

// 打印内存池统计信息
template<typename K, typename V>
void SkipListOptimized<K, V>::print_memory_pool_stats() {
    std::cout << "\n===== Memory Pool Statistics =====" << std::endl;
    std::cout << "Total allocations: " << _memory_pool.get_allocated_count() << std::endl;
    std::cout << "Reused allocations: " << _memory_pool.get_reused_count() << std::endl;
    std::cout << "Free list size: " << _memory_pool.get_free_list_size() << std::endl;
    
    if (_memory_pool.get_allocated_count() > 0) {
        double reuse_rate = (double)_memory_pool.get_reused_count() / 
                           (_memory_pool.get_allocated_count() + _memory_pool.get_reused_count()) * 100;
        std::cout << "Memory reuse rate: " << reuse_rate << "%" << std::endl;
    }
    std::cout << "==================================\n" << std::endl;
}

#endif // SKIPLIST_OPTIMIZED_H

