/* ************************************************************************
> File Name:     test_mvcc.cpp
> Description:   MVCC功能测试程序（优化输出版本）
>                测试多版本并发控制的各种场景
 ************************************************************************/

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cassert>
#include "skiplist_mvcc.h"

using namespace std;
using namespace chrono;

// 测试1：基本事务操作
void test_basic_transaction() {
    cout << "\n========== Test 1: Basic Transaction ==========" << endl;
    auto start = high_resolution_clock::now();
    
    SkipListMVCC<int, string> skiplist(6, true);  // 静默模式
    
    auto txn1 = skiplist.begin_transaction();
    skiplist.insert_element(txn1, 1, "value1");
    skiplist.insert_element(txn1, 2, "value2");
    skiplist.insert_element(txn1, 3, "value3");
    
    string value;
    assert(skiplist.search_element(txn1, 1, &value) == true);
    assert(value == "value1");
    skiplist.commit_transaction(txn1);
    
    auto txn2 = skiplist.begin_transaction();
    assert(skiplist.search_element(txn2, 1, &value) == true);
    assert(value == "value1");
    skiplist.commit_transaction(txn2);
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    cout << "✓ Basic transaction test passed! (耗时: " << duration.count() << "ms)" << endl;
}

// 测试2：事务隔离性 - 读已提交
void test_read_committed() {
    cout << "\n========== Test 2: Read Committed Isolation ==========" << endl;
    auto start = high_resolution_clock::now();
    
    SkipListMVCC<int, string> skiplist(6, true);
    
    auto txn1 = skiplist.begin_transaction();
    skiplist.insert_element(txn1, 10, "initial");
    skiplist.commit_transaction(txn1);
    
    auto txn2 = skiplist.begin_transaction();
    skiplist.insert_element(txn2, 10, "updated_by_txn2");
    
    auto txn3 = skiplist.begin_transaction();
    string value;
    skiplist.search_element(txn3, 10, &value);
    assert(value == "initial");
    
    skiplist.commit_transaction(txn2);
    
    auto txn4 = skiplist.begin_transaction();
    skiplist.search_element(txn4, 10, &value);
    assert(value == "updated_by_txn2");
    
    skiplist.commit_transaction(txn3);
    skiplist.commit_transaction(txn4);
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    cout << "✓ Read committed isolation test passed! (耗时: " << duration.count() << "ms)" << endl;
}

// 测试3：多版本管理
void test_multi_version() {
    cout << "\n========== Test 3: Multi-Version Management ==========" << endl;
    auto start = high_resolution_clock::now();
    
    SkipListMVCC<int, string> skiplist(6, true);
    
    for (int i = 1; i <= 3; i++) {
        auto txn = skiplist.begin_transaction();
        skiplist.insert_element(txn, 100, "v" + to_string(i));
        skiplist.commit_transaction(txn);
    }
    
    auto txn4 = skiplist.begin_transaction();
    string value;
    skiplist.search_element(txn4, 100, &value);
    assert(value == "v3");
    skiplist.commit_transaction(txn4);
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    skiplist.print_stats();
    cout << "✓ Multi-version management test passed! (耗时: " << duration.count() << "ms)" << endl;
}

// 测试4：事务回滚
void test_transaction_abort() {
    cout << "\n========== Test 4: Transaction Abort ==========" << endl;
    auto start = high_resolution_clock::now();
    
    SkipListMVCC<int, string> skiplist(6, true);
    
    auto txn1 = skiplist.begin_transaction();
    skiplist.insert_element(txn1, 50, "committed_value");
    skiplist.commit_transaction(txn1);
    
    auto txn2 = skiplist.begin_transaction();
    skiplist.insert_element(txn2, 50, "aborted_value");
    skiplist.abort_transaction(txn2);
    
    auto txn3 = skiplist.begin_transaction();
    string value;
    skiplist.search_element(txn3, 50, &value);
    assert(value == "committed_value");
    skiplist.commit_transaction(txn3);
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    skiplist.print_stats();
    cout << "✓ Transaction abort test passed! (耗时: " << duration.count() << "ms)" << endl;
}

// 测试5：并发事务
void test_concurrent_transactions() {
    cout << "\n========== Test 5: Concurrent Transactions ==========" << endl;
    auto start = high_resolution_clock::now();
    
    SkipListMVCC<int, string> skiplist(6, true);
    
    auto init_txn = skiplist.begin_transaction();
    for (int i = 0; i < 10; i++) {
        skiplist.insert_element(init_txn, i, "init_" + to_string(i));
    }
    skiplist.commit_transaction(init_txn);
    
    vector<thread> threads;
    
    threads.emplace_back([&skiplist]() {
        auto txn = skiplist.begin_transaction();
        for (int i = 0; i < 10; i++) {
            string value;
            skiplist.search_element(txn, i, &value);
            this_thread::sleep_for(chrono::milliseconds(10));
        }
        skiplist.commit_transaction(txn);
    });
    
    threads.emplace_back([&skiplist]() {
        auto txn = skiplist.begin_transaction();
        for (int i = 0; i < 5; i++) {
            skiplist.insert_element(txn, i, "updated_" + to_string(i));
            this_thread::sleep_for(chrono::milliseconds(15));
        }
        skiplist.commit_transaction(txn);
    });
    
    threads.emplace_back([&skiplist]() {
        auto txn = skiplist.begin_transaction();
        for (int i = 10; i < 15; i++) {
            skiplist.insert_element(txn, i, "new_" + to_string(i));
            this_thread::sleep_for(chrono::milliseconds(12));
        }
        skiplist.commit_transaction(txn);
    });
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    skiplist.print_stats();
    cout << "✓ Concurrent transactions test passed! (耗时: " << duration.count() << "ms)" << endl;
}

// 测试6：范围查询
void test_range_query() {
    cout << "\n========== Test 6: Range Query ==========" << endl;
    auto start = high_resolution_clock::now();
    
    SkipListMVCC<int, string> skiplist(6, true);
    
    auto txn1 = skiplist.begin_transaction();
    for (int i = 0; i < 20; i += 2) {
        skiplist.insert_element(txn1, i, "value_" + to_string(i));
    }
    skiplist.commit_transaction(txn1);
    
    auto txn2 = skiplist.begin_transaction();
    auto results = skiplist.range_query(txn2, 5, 15);
    
    cout << "Range query [5, 15] found " << results.size() << " elements" << endl;
    assert(results.size() == 5);
    skiplist.commit_transaction(txn2);
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    cout << "✓ Range query test passed! (耗时: " << duration.count() << "ms)" << endl;
}

// 测试7：删除操作
void test_delete_operation() {
    cout << "\n========== Test 7: Delete Operation ==========" << endl;
    auto start = high_resolution_clock::now();
    
    SkipListMVCC<int, string> skiplist(6, true);
    
    auto txn1 = skiplist.begin_transaction();
    skiplist.insert_element(txn1, 30, "to_be_deleted");
    skiplist.commit_transaction(txn1);
    
    auto txn2 = skiplist.begin_transaction();
    skiplist.delete_element(txn2, 30);
    skiplist.commit_transaction(txn2);
    
    auto txn3 = skiplist.begin_transaction();
    string value;
    bool found = skiplist.search_element(txn3, 30, &value);
    assert(found == false);
    skiplist.commit_transaction(txn3);
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    cout << "✓ Delete operation test passed! (耗时: " << duration.count() << "ms)" << endl;
}

// 测试8：垃圾回收
void test_garbage_collection() {
    cout << "\n========== Test 8: Garbage Collection ==========" << endl;
    auto start = high_resolution_clock::now();
    
    SkipListMVCC<int, string> skiplist(6, true);
    
    for (int i = 0; i < 10; i++) {
        auto txn = skiplist.begin_transaction();
        skiplist.insert_element(txn, 1, "version_" + to_string(i));
        skiplist.commit_transaction(txn);
    }
    
    cout << "Before GC:" << endl;
    skiplist.print_stats();
    
    skiplist.gc();
    
    cout << "After GC:" << endl;
    skiplist.print_stats();
    
    auto txn = skiplist.begin_transaction();
    string value;
    skiplist.search_element(txn, 1, &value);
    assert(value == "version_9");
    skiplist.commit_transaction(txn);
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    cout << "✓ Garbage collection test passed! (耗时: " << duration.count() << "ms)" << endl;
}

// 测试9：持久化和恢复
void test_persistence() {
    cout << "\n========== Test 9: Persistence ==========" << endl;
    auto start = high_resolution_clock::now();
    
    {
        SkipListMVCC<int, string> skiplist(6, true);
        
        auto txn = skiplist.begin_transaction();
        for (int i = 0; i < 10; i++) {
            skiplist.insert_element(txn, i * 10, "persistent_" + to_string(i));
        }
        skiplist.commit_transaction(txn);
        
        skiplist.dump_file();
    }
    
    {
        SkipListMVCC<int, string> skiplist(6, true);
        skiplist.load_file();
        
        auto txn = skiplist.begin_transaction();
        string value;
        skiplist.search_element(txn, 0, &value);
        assert(value == "persistent_0");
        skiplist.search_element(txn, 90, &value);
        assert(value == "persistent_9");
        skiplist.commit_transaction(txn);
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    cout << "✓ Persistence test passed! (耗时: " << duration.count() << "ms)" << endl;
}

// 测试10：压力测试
void test_stress() {
    cout << "\n========== Test 10: Stress Test ==========" << endl;
    auto start_time = high_resolution_clock::now();
    
    SkipListMVCC<int, string> skiplist(18, true);
    
    vector<thread> threads;
    const int num_threads = 4;
    const int ops_per_thread = 1000;
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&skiplist, t, ops_per_thread]() {
            for (int i = 0; i < ops_per_thread; i++) {
                auto txn = skiplist.begin_transaction();
                
                int key = t * ops_per_thread + i;
                skiplist.insert_element(txn, key, "stress_" + to_string(key));
                
                if (i % 3 == 0) {
                    string value;
                    skiplist.search_element(txn, key, &value);
                }
                
                skiplist.commit_transaction(txn);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end_time = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end_time - start_time);
    
    cout << "Completed " << (num_threads * ops_per_thread) << " operations" << endl;
    cout << "Throughput: " << (num_threads * ops_per_thread * 1000.0 / duration.count()) 
         << " ops/sec" << endl;
    
    skiplist.print_stats();
    cout << "✓ Stress test passed! (耗时: " << duration.count() << "ms)" << endl;
}

// 主函数
int main() {
    cout << "\n";
    cout << "╔════════════════════════════════════════════════════════╗" << endl;
    cout << "║         MVCC Skip List Test Suite                      ║" << endl;
    cout << "║         多版本并发控制跳表测试套件                         ║" << endl;
    cout << "╚════════════════════════════════════════════════════════╝" << endl;
    
    auto total_start = high_resolution_clock::now();
    
    try {
        test_basic_transaction();
        test_read_committed();
        test_multi_version();
        test_transaction_abort();
        test_concurrent_transactions();
        test_range_query();
        test_delete_operation();
        test_garbage_collection();
        test_persistence();
        test_stress();
        
        auto total_end = high_resolution_clock::now();
        auto total_duration = duration_cast<milliseconds>(total_end - total_start);
        
        cout << "\n";
        cout << "╔════════════════════════════════════════════════════════╗" << endl;
        cout << "║         All Tests Passed! ✓                            ║" << endl;
        cout << "║         所有测试通过！                                   ║" << endl;
        cout << "║         总耗时: " << total_duration.count() << " ms" << string(37 - to_string(total_duration.count()).length(), ' ') << "║" << endl;
        cout << "╚════════════════════════════════════════════════════════╝" << endl;
        cout << "\n";
        
    } catch (const exception& e) {
        cerr << "Test failed with exception: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}
