/* ************************************************************************
> File Name:     memory_pool.h
> Description:   内存池实现 - 减少频繁new/delete操作
>                通过对象复用减少内存分配开销和内存碎片
>                提升高并发场景下的性能
 ************************************************************************/

#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <vector>
#include <mutex>
#include <cstring>

/**
 * @brief 跳表节点内存池
 * 
 * 管理跳表节点的内存分配和回收
 * 通过对象池模式减少频繁的new/delete操作
 * 
 * @tparam K 键的类型
 * @tparam V 值的类型
 */
template<typename K, typename V>
class NodeOpt;  // 前向声明

template<typename K, typename V>
class NodeMemoryPool {
public:
    /**
     * @brief 构造函数
     * @param initial_capacity 初始容量（预分配节点数量）
     */
    explicit NodeMemoryPool(int initial_capacity = 100) 
        : _allocated_count(0), _reused_count(0) {
        _free_list.reserve(initial_capacity);
    }
    
    /**
     * @brief 析构函数 - 释放所有缓存的节点
     */
    ~NodeMemoryPool() {
        std::lock_guard<std::mutex> lock(_pool_mutex);
        for (auto* node : _free_list) {
            // 真正释放内存
            delete[] node->forward;
            delete node;
        }
        _free_list.clear();
    }
    
    /**
     * @brief 从内存池分配一个节点
     * @param key 键
     * @param value 值
     * @param level 节点层级
     * @return 分配的节点指针
     */
    NodeOpt<K, V>* allocate(const K& key, const V& value, int level) {
        std::lock_guard<std::mutex> lock(_pool_mutex);
        
        NodeOpt<K, V>* node = nullptr;
        
        // 尝试从空闲列表中获取可复用的节点
        if (!_free_list.empty()) {
            node = _free_list.back();
            _free_list.pop_back();
            
            // 重新初始化节点
            reinitialize_node(node, key, value, level);
            _reused_count++;
        } else {
            // 空闲列表为空，创建新节点
            node = new NodeOpt<K, V>(key, value, level);
            _allocated_count++;
        }
        
        return node;
    }
    
    /**
     * @brief 将节点归还到内存池
     * @param node 要回收的节点指针
     */
    void deallocate(NodeOpt<K, V>* node) {
        if (node == nullptr) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(_pool_mutex);
        
        // 将节点放回空闲列表，而不是直接delete
        // 注意：不释放forward数组，保留以便复用
        _free_list.push_back(node);
    }
    
    /**
     * @brief 获取统计信息 - 总分配次数
     */
    int get_allocated_count() const {
        return _allocated_count;
    }
    
    /**
     * @brief 获取统计信息 - 复用次数
     */
    int get_reused_count() const {
        return _reused_count;
    }
    
    /**
     * @brief 获取当前空闲列表大小
     */
    size_t get_free_list_size() const {
        std::lock_guard<std::mutex> lock(_pool_mutex);
        return _free_list.size();
    }
    
    /**
     * @brief 清空内存池（释放所有缓存节点）
     */
    void clear() {
        std::lock_guard<std::mutex> lock(_pool_mutex);
        for (auto* node : _free_list) {
            delete[] node->forward;
            delete node;
        }
        _free_list.clear();
    }
    
private:
    std::vector<NodeOpt<K, V>*> _free_list;      // 空闲节点列表
    mutable std::mutex _pool_mutex;              // 保护内存池的互斥锁
    int _allocated_count;                         // 总分配次数统计
    int _reused_count;                            // 复用次数统计
    
    /**
     * @brief 重新初始化节点
     * @param node 要初始化的节点
     * @param key 新的键
     * @param value 新的值
     * @param level 新的层级
     */
    void reinitialize_node(NodeOpt<K, V>* node, const K& key, const V& value, int level) {
        // 如果新层级与旧层级不同，需要重新分配forward数组
        if (node->node_level != level) {
            delete[] node->forward;
            node->forward = new NodeOpt<K, V>*[level + 1];
            node->node_level = level;
        }
        
        // 重置forward数组
        memset(node->forward, 0, sizeof(NodeOpt<K, V>*) * (level + 1));
        
        // 设置新的键值（通过友元或公共方法）
        // 注意：这里需要NodeOpt类提供设置key的方法
        node->set_key_value(key, value);
    }
    
    // 禁止拷贝和赋值
    NodeMemoryPool(const NodeMemoryPool&) = delete;
    NodeMemoryPool& operator=(const NodeMemoryPool&) = delete;
};

#endif // MEMORY_POOL_H
