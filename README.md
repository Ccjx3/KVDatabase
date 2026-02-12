# 跳表（Skip List）项目

一个高性能的跳表数据结构实现，包含三个版本：基础版、优化版（细粒度锁+内存池）和 MVCC 版（多版本并发控制）。

## 📋 项目简介

本项目实现了跳表（Skip List）数据结构的三个不同版本，每个版本针对不同的应用场景进行了优化：

- **基础版**：经典跳表实现，支持基本的增删改查和范围查询
- **优化版**：采用分段锁和内存池技术，提升并发性能
- **MVCC 版**：支持多版本并发控制，实现事务隔离和 ACID 特性

## 🚀 编译运行

### 环境要求

- C++ 编译器（支持 C++11 及以上）
- Make 工具
- 操作系统：Linux / macOS / Windows (WSL)

### 编译命令

```bash
# 编译基础版跳表示例
make main

# 编译优化版测试程序
make test_optimized

# 编译 MVCC 版测试程序
make test_mvcc

# 编译所有程序
make all

# 清理编译产物
make clean
```

### 运行程序

```bash
# 运行基础版示例
./bin/main

# 运行优化版测试
./bin/test_optimized

# 运行 MVCC 版测试
./bin/test_mvcc
```

## 📚 功能介绍

### 1. 基础版跳表 (`skiplist.h`)

经典的跳表实现，提供基本的键值存储功能。

**核心功能：**
- 插入元素
- 查找元素
- 删除元素
- 范围查询
- 持久化到文件
- 从文件加载

**实现伪代码：**

```cpp
template <typename K, typename V> 
class SkipList {
public:
    // 构造函数：初始化跳表
    SkipList(int max_level) {
        初始化最大层级
        创建头节点
        元素计数 = 0
    }
    
    // 插入元素：O(log n)
    int insert_element(K key, V value) {
        从最高层开始查找插入位置
        记录每层的前驱节点到 update 数组
        
        if (key 已存在) {
            返回失败
        }
        
        随机生成新节点层级
        创建新节点
        
        for (每一层) {
            插入新节点到链表中
        }
        
        元素计数++
        返回成功
    }
    
    // 查找元素：O(log n)
    bool search_element(K key) {
        current = header
        
        for (i = 最高层; i >= 0; i--) {
            while (current->forward[i] 存在 && current->forward[i]->key < key) {
                current = current->forward[i]  // 在当前层向右移动
            }
        }
        
        current = current->forward[0]
        return (current != NULL && current->key == key)
    }
    
    // 删除元素：O(log n)
    void delete_element(K key) {
        查找要删除的节点
        记录每层的前驱节点
        
        if (找到节点) {
            for (每一层) {
                更新前驱节点的指针
            }
            删除节点
            更新跳表层级
            元素计数--
        }
    }
    
    // 范围查询：O(log n + m)，m 为结果数量
    vector<pair<K, V>> range_query(K start_key, K end_key) {
        if (start_key > end_key) {
            返回空结果
        }
        
        // 第一步：找到起始位置（O(log n)）
        从最高层开始查找 start_key 的前驱
        移动到第 0 层的下一个节点
        
        // 第二步：顺序收集结果（O(m)）
        while (current != NULL && current->key <= end_key) {
            将 (key, value) 加入结果集
            current = current->forward[0]
        }
        
        返回结果集
    }
    
    // 持久化到文件
    void dump_file() {
        打开文件
        遍历第 0 层所有节点
        将 key:value 写入文件
        关闭文件
    }
    
    // 从文件加载
    void load_file() {
        打开文件
        逐行读取 key:value
        调用 insert_element 插入
        关闭文件
    }

private:
    int _max_level;              // 最大层级
    int _skip_list_level;        // 当前层级
    Node<K, V> *_header;         // 头节点
    int _element_count;          // 元素数量
};
```

---

### 2. 优化版跳表 (`skiplist_optimized.h`)

在基础版基础上增加了两项关键优化，显著提升并发性能。

**优化技术：**

#### 2.1 分段锁机制 (`segment_lock.h`)

类似 ConcurrentHashMap 的设计，将数据空间分成多个段，每个段有独立的锁。

```cpp
template<typename K>
class SegmentLockManager {
public:
    SegmentLockManager(int segment_count = 16) {
        分配 segment_count 个互斥锁
    }
    
    // 根据 key 计算所属段索引
    int get_segment_index(const K& key) {
        hash_value = hash(key)
        return hash_value % segment_count
    }
    
    // 获取指定段的锁
    unique_lock<mutex> get_write_lock(int segment_index) {
        return unique_lock(segment_locks[segment_index])
    }
    
    // 获取所有段的锁（用于全局操作）
    vector<unique_lock<mutex>> get_all_write_locks() {
        按顺序获取所有锁（避免死锁）
        return locks
    }

private:
    int segment_count;
    mutex* segment_locks;
};
```

**优势：** 不同段的操作可以并发执行，提升多线程性能。

#### 2.2 内存池优化 (`memory_pool.h`)

通过对象复用减少频繁的 new/delete 操作。

```cpp
template<typename K, typename V>
class NodeMemoryPool {
public:
    // 分配节点
    NodeOpt<K, V>* allocate(K key, V value, int level) {
        lock_guard<mutex> lock(pool_mutex)
        
        if (free_list 不为空) {
            node = free_list.pop_back()
            重新初始化节点(node, key, value, level)
            reused_count++
        } else {
            node = new NodeOpt(key, value, level)
            allocated_count++
        }
        
        return node
    }
    
    // 回收节点
    void deallocate(NodeOpt<K, V>* node) {
        lock_guard<mutex> lock(pool_mutex)
        将节点放回 free_list（不释放内存）
    }
    
    // 重新初始化节点
    void reinitialize_node(node, key, value, level) {
        if (node->level != level) {
            重新分配 forward 数组
        }
        重置 forward 数组为 NULL
        设置新的 key 和 value
    }

private:
    vector<NodeOpt<K, V>*> free_list;  // 空闲节点列表
    mutex pool_mutex;
    int allocated_count;               // 统计信息
    int reused_count;
};
```

**优势：** 减少内存分配开销和内存碎片，提升高并发场景性能。

#### 2.3 优化版跳表核心实现

```cpp
template <typename K, typename V> 
class SkipListOptimized {
public:
    // 插入元素（使用分段锁）
    int insert_element(K key, V value) {
        segment_index = lock_manager.get_segment_index(key)
        lock = lock_manager.get_write_lock(segment_index)  // 只锁定该段
        
        查找插入位置
        
        if (key 不存在) {
            node = memory_pool.allocate(key, value, level)  // 从内存池分配
            插入节点
            element_count++
        }
        
        return 结果
    }
    
    // 查找元素（使用分段读锁）
    bool search_element(K key) {
        segment_index = lock_manager.get_segment_index(key)
        lock = lock_manager.get_read_lock(segment_index)  // 共享锁
        
        执行查找逻辑
        return 结果
    }

private:
    SegmentLockManager<K> lock_manager;    // 分段锁管理器
    NodeMemoryPool<K, V> memory_pool;      // 内存池
};
```

---

### 3. MVCC 版跳表 (`skiplist_mvcc.h`)

支持多版本并发控制（Multi-Version Concurrency Control），实现事务隔离。

**核心概念：**

#### 3.1 版本记录结构

```cpp
template<typename K, typename V>
struct Version {
    V value;                    // 值
    uint64_t create_ts;         // 创建时间戳（事务 ID）
    uint64_t delete_ts;         // 删除时间戳
    bool is_committed;          // 是否已提交
    shared_ptr<Version> next;   // 指向旧版本
    
    // 判断版本对事务是否可见（读已提交隔离级别）
    bool is_visible(uint64_t txn_id) {
        if (create_ts == txn_id) {
            return delete_ts > txn_id  // 当前事务创建的版本
        }
        
        // 其他事务创建的版本必须已提交
        return is_committed && create_ts < txn_id && delete_ts > txn_id
    }
};
```

#### 3.2 MVCC 节点

```cpp
template<typename K, typename V>
class NodeMVCC {
public:
    // 添加新版本
    void add_version(V value, uint64_t txn_id) {
        lock_guard<mutex> lock(version_mutex)
        
        new_version = new Version(value, txn_id)
        new_version->next = version_head
        version_head = new_version  // 插入到版本链头部
    }
    
    // 获取可见版本
    shared_ptr<Version> get_visible_version(uint64_t txn_id) {
        lock_guard<mutex> lock(version_mutex)
        
        current = version_head
        while (current != NULL) {
            if (current->is_visible(txn_id)) {
                return current
            }
            current = current->next
        }
        
        return NULL  // 无可见版本
    }
    
    // 标记删除
    void mark_deleted(uint64_t txn_id) {
        lock_guard<mutex> lock(version_mutex)
        version_head->delete_ts = txn_id
    }
    
    // 提交版本
    void commit_version(uint64_t txn_id) {
        遍历版本链
        将 create_ts == txn_id 的版本标记为已提交
    }
    
    // 垃圾回收
    void gc_versions(uint64_t min_active_txn_id) {
        删除对所有活跃事务都不可见的旧版本
    }

private:
    K key;
    shared_ptr<Version> version_head;  // 版本链头（最新版本）
    mutex version_mutex;
};
```

#### 3.3 事务管理

```cpp
template<typename K, typename V>
class Transaction {
public:
    uint64_t txn_id;                          // 事务 ID
    TransactionState state;                   // 状态：ACTIVE/COMMITTED/ABORTED
    vector<NodeMVCC<K, V>*> modified_nodes;   // 修改的节点列表
    
    void commit() { state = COMMITTED; }
    void abort() { state = ABORTED; }
};

template<typename K, typename V>
class SkipListMVCC {
public:
    // 开始事务
    shared_ptr<Transaction> begin_transaction() {
        txn_id = next_txn_id++
        txn = new Transaction(txn_id)
        active_transactions[txn_id] = txn
        return txn
    }
    
    // 提交事务
    bool commit_transaction(shared_ptr<Transaction> txn) {
        // 标记所有修改的版本为已提交
        for (node : txn->modified_nodes) {
            node->commit_version(txn->txn_id)
        }
        
        txn->commit()
        active_transactions.erase(txn->txn_id)
        total_commits++
    }
    
    // 回滚事务
    void abort_transaction(shared_ptr<Transaction> txn) {
        txn->abort()
        active_transactions.erase(txn->txn_id)
        total_aborts++
        // 未提交的版本自动对其他事务不可见
    }
    
    // 插入元素（事务操作）
    int insert_element(shared_ptr<Transaction> txn, K key, V value) {
        if (!txn->is_active()) {
            return 失败
        }
        
        查找 key
        
        if (key 已存在) {
            node->add_version(value, txn->txn_id)  // 添加新版本
        } else {
            创建新节点
            node->add_version(value, txn->txn_id)
            插入跳表
        }
        
        txn->add_modified_node(node)  // 记录修改
        return 成功
    }
    
    // 查找元素（事务操作）
    bool search_element(shared_ptr<Transaction> txn, K key, V* value) {
        查找 key
        
        if (找到节点) {
            version = node->get_visible_version(txn->txn_id)
            if (version != NULL) {
                *value = version->value
                return true
            }
        }
        
        return false
    }
    
    // 范围查询（事务操作）
    vector<pair<K, V>> range_query(shared_ptr<Transaction> txn, K start, K end) {
        找到起始位置
        
        while (current != NULL && current->key <= end) {
            version = current->get_visible_version(txn->txn_id)
            if (version != NULL) {
                result.push_back((current->key, version->value))
            }
            current = current->forward[0]
        }
        
        return result
    }
    
    // 垃圾回收
    void gc() {
        min_active_txn_id = get_min_active_txn_id()
        
        遍历所有节点 {
            node->gc_versions(min_active_txn_id)
        }
    }

private:
    atomic<uint64_t> next_txn_id;                                    // 下一个事务 ID
    unordered_map<uint64_t, shared_ptr<Transaction>> active_transactions;  // 活跃事务
};
```

**MVCC 特性：**
- **事务隔离级别**：读已提交（Read Committed）
- **无锁读**：读操作不阻塞写操作
- **版本管理**：每次更新创建新版本，旧版本保留
- **垃圾回收**：定期清理对所有事务不可见的旧版本
- **ACID 支持**：原子性、一致性、隔离性、持久性

---

## 🗂️ 项目结构

```
Skiplist/
├── bin/                          # 可执行文件目录
│   ├── main                      # 基础版示例程序
│   ├── test_optimized            # 优化版测试程序
│   └── test_mvcc                 # MVCC 版测试程序
├── store/                        # 数据持久化目录
│   ├── dumpFile                  # 基础版数据文件
│   ├── dumpFile_optimized        # 优化版数据文件
│   └── dumpFile_mvcc             # MVCC 版数据文件
├── stress-test/                  # 压力测试
│   └── stress_test.cpp           # 压力测试程序
├── skiplist.h                    # 基础版跳表实现
├── skiplist_optimized.h          # 优化版跳表实现
├── skiplist_mvcc.h               # MVCC 版跳表实现
├── segment_lock.h                # 分段锁实现
├── memory_pool.h                 # 内存池实现
├── main.cpp                      # 基础版示例程序
├── test_optimized.cpp            # 优化版测试程序
├── test_mvcc.cpp                 # MVCC 版测试程序
├── makefile                      # 编译配置
├── LICENSE                       # GPL v3 许可证
└── README.md                     # 项目文档
```

---

## 🔧 使用示例

### 基础版使用

```cpp
#include "skiplist.h"

int main() {
    // 创建跳表，最大层级为 6
    SkipList<int, std::string> skipList(6);
    
    // 插入元素
    skipList.insert_element(1, "one");
    skipList.insert_element(3, "three");
    skipList.insert_element(7, "seven");
    
    // 查找元素
    skipList.search_element(3);  // 输出：Found key: 3, value: three
    
    // 范围查询
    auto results = skipList.range_query(1, 7);
    for (auto& pair : results) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    
    // 删除元素
    skipList.delete_element(3);
    
    // 持久化
    skipList.dump_file();
    
    // 加载
    skipList.load_file();
    
    return 0;
}
```

### 优化版使用

```cpp
#include "skiplist_optimized.h"

int main() {
    // 创建优化版跳表，最大层级 18，16 个分段
    SkipListOptimized<int, std::string> skipList(18, 16);
    
    // 多线程并发插入
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; i++) {
        threads.emplace_back([&skipList, i]() {
            for (int j = 0; j < 1000; j++) {
                skipList.insert_element(i * 1000 + j, "value");
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // 打印内存池统计
    skipList.print_memory_pool_stats();
    
    return 0;
}
```

### MVCC 版使用

```cpp
#include "skiplist_mvcc.h"

int main() {
    // 创建 MVCC 跳表
    SkipListMVCC<int, std::string> skipList(6);
    
    // 事务 1：插入数据
    auto txn1 = skipList.begin_transaction();
    skipList.insert_element(txn1, 1, "value1");
    skipList.insert_element(txn1, 2, "value2");
    skipList.commit_transaction(txn1);
    
    // 事务 2：更新数据
    auto txn2 = skipList.begin_transaction();
    skipList.insert_element(txn2, 1, "updated_value");
    
    // 事务 3：读取数据（读已提交）
    auto txn3 = skipList.begin_transaction();
    std::string value;
    skipList.search_element(txn3, 1, &value);
    // value = "value1"（txn2 未提交，不可见）
    
    skipList.commit_transaction(txn2);
    skipList.commit_transaction(txn3);
    
    // 事务 4：读取最新数据
    auto txn4 = skipList.begin_transaction();
    skipList.search_element(txn4, 1, &value);
    // value = "updated_value"（txn2 已提交，可见）
    skipList.commit_transaction(txn4);
    
    // 垃圾回收
    skipList.gc();
    
    // 打印统计信息
    skipList.print_stats();
    
    return 0;
}
```

---

## 📖 算法复杂度

| 操作 | 时间复杂度 | 空间复杂度 |
|------|-----------|-----------|
| 插入 | O(log n) | O(1) |
| 查找 | O(log n) | O(1) |
| 删除 | O(log n) | O(1) |
| 范围查询 | O(log n + m) | O(m) |

其中 n 为跳表元素总数，m 为范围查询结果数量。

---

## 🎯 适用场景

### 基础版
- 单线程或低并发场景
- 需要有序键值存储
- 需要范围查询功能

### 优化版
- 高并发读写场景
- 需要高性能键值存储
- 内存敏感的应用

### MVCC 版
- 需要事务支持的场景
- 数据库系统
- 分布式系统
- 需要快照隔离的应用

---

