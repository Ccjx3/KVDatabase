/* ************************************************************************
> File Name:     segment_lock.h
> Description:   细粒度锁实现 - 分段锁机制
>                类似ConcurrentHashMap的设计，将跳表分成多个段
>                每个段有独立的读写锁，提升并发性能
 ************************************************************************/

#ifndef SEGMENT_LOCK_H
#define SEGMENT_LOCK_H

#include <mutex>
#include <functional>
#include <vector>

/**
 * @brief 分段锁管理器
 * 
 * 将数据空间分成多个段，每个段有独立的读写锁
 * 不同段的操作可以并发执行，提升并发性能
 * 
 * @tparam K 键的类型
 */
template<typename K>
class SegmentLockManager {
public:
    // 默认分段数量，可根据CPU核心数调整
    static const int DEFAULT_SEGMENT_COUNT = 16;
    
    /**
     * @brief 构造函数
     * @param segment_count 分段数量，建议设置为2的幂次方
     */
    explicit SegmentLockManager(int segment_count = DEFAULT_SEGMENT_COUNT) 
        : _segment_count(segment_count) {
        _segment_locks = new std::mutex[segment_count];
    }
    
    /**
     * @brief 析构函数
     */
    ~SegmentLockManager() {
        delete[] _segment_locks;
    }
    
    /**
     * @brief 根据key计算所属的段索引
     * @param key 键值
     * @return 段索引 [0, segment_count)
     */
    int get_segment_index(const K& key) const {
        // 使用std::hash计算哈希值，然后取模
        size_t hash_value = std::hash<K>{}(key);
        return hash_value % _segment_count;
    }
    
    /**
     * @brief 获取指定段的读锁（共享锁）
     * @param segment_index 段索引
     * @return 独占锁对象（注意：使用mutex代替shared_mutex，读写都是独占）
     */
    std::unique_lock<std::mutex> get_read_lock(int segment_index) {
        return std::unique_lock<std::mutex>(_segment_locks[segment_index]);
    }
    
    /**
     * @brief 获取指定段的写锁（独占锁）
     * @param segment_index 段索引
     * @return 独占锁对象
     */
    std::unique_lock<std::mutex> get_write_lock(int segment_index) {
        return std::unique_lock<std::mutex>(_segment_locks[segment_index]);
    }
    
    /**
     * @brief 获取所有段的写锁（用于全局操作，如dump_file）
     * @return 所有段的独占锁对象向量
     */
    std::vector<std::unique_lock<std::mutex>> get_all_write_locks() {
        std::vector<std::unique_lock<std::mutex>> locks;
        locks.reserve(_segment_count);
        
        // 按顺序获取所有锁，避免死锁
        for (int i = 0; i < _segment_count; i++) {
            locks.emplace_back(_segment_locks[i]);
        }
        return locks;
    }
    
    /**
     * @brief 获取分段数量
     */
    int get_segment_count() const {
        return _segment_count;
    }
    
private:
    int _segment_count;                      // 分段数量
    std::mutex* _segment_locks;              // 分段锁数组
    
    // 禁止拷贝和赋值
    SegmentLockManager(const SegmentLockManager&) = delete;
    SegmentLockManager& operator=(const SegmentLockManager&) = delete;
};

#endif // SEGMENT_LOCK_H

