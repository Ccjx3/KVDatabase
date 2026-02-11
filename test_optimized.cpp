/* ************************************************************************
> File Name:     test_optimized.cpp
> Description:   测试优化版跳表的功能和性能
>                对比原版和优化版的性能差异
 ************************************************************************/

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <sstream>
#include "skiplist.h"
#include "skiplist_optimized.h"

#define NUM_THREADS 8
#define TEST_COUNT 10000

// 用于临时禁用 cout 输出的工具类
class CoutRedirect {
private:
    std::streambuf* old_buf;
    std::ostringstream null_stream;
public:
    CoutRedirect() {
        old_buf = std::cout.rdbuf();
        std::cout.rdbuf(null_stream.rdbuf());
    }
    ~CoutRedirect() {
        std::cout.rdbuf(old_buf);
    }
};

// 测试原版跳表的插入性能
void test_original_insert() {
    std::cout << "\n========== 测试原版跳表单线程插入 ==========" << std::endl;
    SkipList<int, std::string> skipList(18);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    {
        CoutRedirect redirect;  // 禁用输出
        for (int i = 0; i < TEST_COUNT; i++) {
            skipList.insert_element(i, "value_" + std::to_string(i));
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "插入 " << TEST_COUNT << " 个元素耗时: " << duration.count() << " ms" << std::endl;
    std::cout << "QPS: " << (TEST_COUNT * 1000.0 / duration.count()) << std::endl;
}

// 测试优化版跳表的插入性能
void test_optimized_insert() {
    std::cout << "\n========== 测试优化版跳表单线程插入 ==========" << std::endl;
    SkipListOptimized<int, std::string> skipList(18, 16);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    {
        CoutRedirect redirect;  // 禁用输出
        for (int i = 0; i < TEST_COUNT; i++) {
            skipList.insert_element(i, "value_" + std::to_string(i));
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "插入 " << TEST_COUNT << " 个元素耗时: " << duration.count() << " ms" << std::endl;
    std::cout << "QPS: " << (TEST_COUNT * 1000.0 / duration.count()) << std::endl;
    std::cout << "实际元素数量: " << skipList.size() << std::endl;
    
    // 打印内存池统计
    skipList.print_memory_pool_stats();
}

// 多线程插入测试 - 原版
void thread_insert_original(SkipList<int, std::string>* skipList, int thread_id, int count) {
    int start = thread_id * count;
    for (int i = 0; i < count; i++) {
        skipList->insert_element(start + i, "value_" + std::to_string(start + i));
    }
}

void test_original_concurrent_insert() {
    std::cout << "\n========== 原版跳表多线程插入测试 ==========" << std::endl;
    SkipList<int, std::string> skipList(18);
    
    std::vector<std::thread> threads;
    int count_per_thread = TEST_COUNT / NUM_THREADS;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 在作用域内禁用输出
    {
        CoutRedirect redirect;
        for (int i = 0; i < NUM_THREADS; i++) {
            threads.emplace_back(thread_insert_original, &skipList, i, count_per_thread);
        }
        
        for (auto& t : threads) {
            t.join();
        }
    }  // redirect 析构，恢复输出
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "使用 " << NUM_THREADS << " 个线程插入 " << TEST_COUNT << " 个元素" << std::endl;
    std::cout << "总耗时: " << duration.count() << " ms" << std::endl;
    std::cout << "QPS: " << (TEST_COUNT * 1000.0 / duration.count()) << std::endl;
    std::cout << "实际元素数量: " << skipList.size() << std::endl;
}

// 多线程插入测试 - 优化版
void thread_insert_optimized(SkipListOptimized<int, std::string>* skipList, int thread_id, int count) {
    int start = thread_id * count;
    for (int i = 0; i < count; i++) {
        skipList->insert_element(start + i, "value_" + std::to_string(start + i));
    }
}

void test_optimized_concurrent_insert() {
    std::cout << "\n========== 优化版跳表多线程插入测试 ==========" << std::endl;
    SkipListOptimized<int, std::string> skipList(18, 16);
    
    std::vector<std::thread> threads;
    int count_per_thread = TEST_COUNT / NUM_THREADS;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 在作用域内禁用输出
    {
        CoutRedirect redirect;
        for (int i = 0; i < NUM_THREADS; i++) {
            threads.emplace_back(thread_insert_optimized, &skipList, i, count_per_thread);
        }
        
        for (auto& t : threads) {
            t.join();
        }
    }  // redirect 析构，恢复输出
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "使用 " << NUM_THREADS << " 个线程插入 " << TEST_COUNT << " 个元素" << std::endl;
    std::cout << "总耗时: " << duration.count() << " ms" << std::endl;
    std::cout << "QPS: " << (TEST_COUNT * 1000.0 / duration.count()) << std::endl;
    std::cout << "实际元素数量: " << skipList.size() << std::endl;
    
    // 打印内存池统计
    skipList.print_memory_pool_stats();
}

// 多线程读测试 - 优化版
void thread_search_optimized(SkipListOptimized<int, std::string>* skipList, int count) {
    for (int i = 0; i < count; i++) {
        skipList->search_element_silent(rand() % TEST_COUNT);
    }
}

void test_optimized_concurrent_search() {
    std::cout << "\n========== 优化版跳表多线程查询测试 ==========" << std::endl;
    
    // 先插入数据
    SkipListOptimized<int, std::string> skipList(18, 16);
    {
        CoutRedirect redirect;  // 禁用输出
        for (int i = 0; i < TEST_COUNT; i++) {
            skipList.insert_element(i, "value_" + std::to_string(i));
        }
    }
    
    std::vector<std::thread> threads;
    int search_per_thread = TEST_COUNT / NUM_THREADS;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 在作用域内禁用输出
    {
        CoutRedirect redirect;
        for (int i = 0; i < NUM_THREADS; i++) {
            threads.emplace_back(thread_search_optimized, &skipList, search_per_thread);
        }
        
        for (auto& t : threads) {
            t.join();
        }
    }  // redirect 析构，恢复输出
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "使用 " << NUM_THREADS << " 个线程查询 " << TEST_COUNT << " 次" << std::endl;
    std::cout << "总耗时: " << duration.count() << " ms" << std::endl;
    std::cout << "QPS: " << (TEST_COUNT * 1000.0 / duration.count()) << std::endl;
}

// 功能测试
void test_basic_functions() {
    std::cout << "\n========== 基本功能演示 ==========" << std::endl;
    SkipListOptimized<int, std::string> skipList(6, 16);
    
    // 插入测试
    std::cout << "\n[1] 插入测试 - 插入 6 个元素" << std::endl;
    skipList.insert_element(1, "one");
    skipList.insert_element(3, "three");
    skipList.insert_element(7, "seven");
    skipList.insert_element(8, "eight");
    skipList.insert_element(9, "nine");
    skipList.insert_element(19, "nineteen");
    std::cout << "跳表大小: " << skipList.size() << std::endl;
    
    // 查询测试
    std::cout << "\n[2] 查询测试 - 查找存在和不存在的 key" << std::endl;
    skipList.search_element(9);
    skipList.search_element(18);
    
    // 显示跳表
    std::cout << "\n[3] 显示跳表结构" << std::endl;
    skipList.display_list();
    
    // 删除测试
    std::cout << "\n[4] 删除测试 - 删除 key=3 和 key=7" << std::endl;
    skipList.delete_element(3);
    skipList.delete_element(7);
    std::cout << "删除后跳表大小: " << skipList.size() << std::endl;
    skipList.display_list();
    
    // 内存池统计
    std::cout << "\n[5] 内存池统计" << std::endl;
    skipList.print_memory_pool_stats();
}

int main() {
    std::cout << "======================================" << std::endl;
    std::cout << "  跳表优化效果测试程序" << std::endl;
    std::cout << "  1. 细粒度锁优化（分段锁）" << std::endl;
    std::cout << "  2. 内存池优化" << std::endl;
    std::cout << "======================================" << std::endl;
    
    // 基本功能演示
    test_basic_functions();
    
    // 单线程性能对比
    test_original_insert();
    test_optimized_insert();
    
    // 多线程插入性能对比（原版跳表可能不支持多线程，跳过）
    std::cout << "\n========== 原版跳表多线程插入测试 ==========" << std::endl;
    std::cout << "（原版跳表多线程支持有问题，已跳过）" << std::endl;
    
    test_optimized_concurrent_insert();
    
    // 多线程查询性能测试
    test_optimized_concurrent_search();
    
    std::cout << "\n======================================" << std::endl;
    std::cout << "  测试完成！" << std::endl;
    std::cout << "======================================" << std::endl;
    
    return 0;
}

