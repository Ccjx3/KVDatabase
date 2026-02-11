#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;

// 节点类
template<typename K, typename V>
class Node {
public:
    K key;
    V value;
    Node* next;    // 同层的下一个节点
    Node* down;    // 下一层的对应节点
    
    Node(K k, V v) : key(k), value(v), next(nullptr), down(nullptr) {}
};

// 双层跳表类
template<typename K, typename V>
class TwoLevelSkipList {
private:
    Node<K, V>* level1_head;  // 第1层头节点
    Node<K, V>* level0_head;  // 第0层头节点
    int element_count;
    
public:
    // 构造函数
    TwoLevelSkipList() : level1_head(nullptr), level0_head(nullptr), element_count(0) {
        srand(time(nullptr));  // 初始化随机数种子
        cout << "双层跳表已创建" << endl;
    }
    
    // 析构函数
    ~TwoLevelSkipList() {
        // 释放 Level 1
        Node<K, V>* current = level1_head;
        while (current != nullptr) {
            Node<K, V>* temp = current;
            current = current->next;
            delete temp;
        }
        
        // 释放 Level 0
        current = level0_head;
        while (current != nullptr) {
            Node<K, V>* temp = current;
            current = current->next;
            delete temp;
        }
        
        cout << "双层跳表已销毁，内存已释放" << endl;
    }
    
    // 插入元素
    void insert(K key, V value) {
        cout << "\n--- 插入 key=" << key << " ---" << endl;
        
        // 步骤1：在 Level 0 插入
        Node<K, V>* newNode0 = new Node<K, V>(key, value);
        
        if (level0_head == nullptr || level0_head->key >= key) {
            if (level0_head != nullptr && level0_head->key == key) {
                cout << "key=" << key << " 已存在" << endl;
                delete newNode0;
                return;
            }
            newNode0->next = level0_head;
            level0_head = newNode0;
        } else {
            Node<K, V>* current = level0_head;
            while (current->next != nullptr && current->next->key < key) {
                current = current->next;
            }
            
            if (current->next != nullptr && current->next->key == key) {
                cout << "key=" << key << " 已存在" << endl;
                delete newNode0;
                return;
            }
            
            newNode0->next = current->next;
            current->next = newNode0;
        }
        
        element_count++;
        cout << "✓ 在 Level 0 插入成功" << endl;
        
        // 步骤2：以 50% 概率提升到 Level 1
        if (rand() % 2 == 0) {
            cout << "→ 提升到 Level 1" << endl;
            Node<K, V>* newNode1 = new Node<K, V>(key, value);
            newNode1->down = newNode0;  // 连接到下层节点
            
            if (level1_head == nullptr || level1_head->key >= key) {
                newNode1->next = level1_head;
                level1_head = newNode1;
            } else {
                Node<K, V>* current = level1_head;
                while (current->next != nullptr && current->next->key < key) {
                    current = current->next;
                }
                newNode1->next = current->next;
                current->next = newNode1;
            }
        } else {
            cout << "→ 不提升到 Level 1" << endl;
        }
    }
    
    // 查找元素
    bool search(K key) {
        cout << "\n--- 查找 key=" << key << " ---" << endl;
        int comparisons = 0;
        
        // 步骤1：从 Level 1 开始
        Node<K, V>* current = level1_head;
        
        if (current != nullptr) {
            cout << "从 Level 1 开始查找..." << endl;
            
            // 在 Level 1 向右移动
            while (current != nullptr && current->key < key) {
                comparisons++;
                if (current->next == nullptr || current->next->key > key) {
                    // 下降到 Level 0
                    cout << "从 Level 1 下降到 Level 0，位置在 key=" << current->key << " 之后" << endl;
                    current = current->down;
                    if (current != nullptr) {
                        current = current->next;
                    }
                    break;
                }
                current = current->next;
            }
            
            // 检查是否在 Level 1 就找到了
            if (current != nullptr && current->key == key) {
                comparisons++;
                cout << "✓ 在 Level 1 找到 key=" << key << ", value=" << current->value << endl;
                cout << "总比较次数: " << comparisons << endl;
                return true;
            }
        } else {
            // Level 1 为空，直接从 Level 0 开始
            cout << "Level 1 为空，从 Level 0 开始查找..." << endl;
            current = level0_head;
        }
        
        // 步骤2：在 Level 0 查找
        while (current != nullptr) {
            comparisons++;
            if (current->key == key) {
                cout << "✓ 在 Level 0 找到 key=" << key << ", value=" << current->value << endl;
                cout << "总比较次数: " << comparisons << endl;
                return true;
            }
            if (current->key > key) {
                break;
            }
            current = current->next;
        }
        
        cout << "✗ 未找到 key=" << key << endl;
        cout << "总比较次数: " << comparisons << endl;
        return false;
    }
    
    // 显示跳表
    void display() {
        cout << "\n========== 双层跳表结构 ==========" << endl;
        
        // 显示 Level 1
        cout << "Level 1: ";
        Node<K, V>* current1 = level1_head;
        if (current1 == nullptr) {
            cout << "空" << endl;
        } else {
            while (current1 != nullptr) {
                cout << "[" << current1->key << ":" << current1->value << "]";
                if (current1->next != nullptr) {
                    cout << " -> ";
                }
                current1 = current1->next;
            }
            cout << " -> NULL" << endl;
        }
        
        // 显示 Level 0
        cout << "Level 0: ";
        Node<K, V>* current0 = level0_head;
        if (current0 == nullptr) {
            cout << "空" << endl;
        } else {
            while (current0 != nullptr) {
                cout << "[" << current0->key << ":" << current0->value << "]";
                if (current0->next != nullptr) {
                    cout << " -> ";
                }
                current0 = current0->next;
            }
            cout << " -> NULL" << endl;
        }
        
        cout << "总元素数: " << element_count << endl;
        cout << "===================================" << endl;
    }
    
    // 获取大小
    int size() {
        return element_count;
    }
};

// 主函数：演示程序
int main() {
    cout << "========================================" << endl;
    cout << "  阶段2：双层跳表实现演示" << endl;
    cout << "========================================\n" << endl;
    
    TwoLevelSkipList<int, string> skipList;
    
    cout << "\n【测试1：插入元素】" << endl;
    cout << "-------------------" << endl;
    skipList.insert(1, "一");
    skipList.insert(3, "三");
    skipList.insert(5, "五");
    skipList.insert(7, "七");
    skipList.insert(9, "九");
    skipList.insert(2, "二");
    skipList.insert(4, "四");
    skipList.insert(6, "六");
    skipList.insert(8, "八");
    
    skipList.display();
    
    cout << "\n【测试2：查找元素】" << endl;
    cout << "-------------------" << endl;
    skipList.search(7);  // 应该找到
    skipList.search(10); // 应该找不到
    skipList.search(1);  // 测试头节点
    skipList.search(9);  // 测试尾节点
    
    cout << "\n【测试3：对比查找效率】" << endl;
    cout << "-------------------" << endl;
    cout << "提示：观察'总比较次数'，体会跳表的加速效果！" << endl;
    cout << "如果没有 Level 1，查找 key=9 需要比较 9 次" << endl;
    cout << "有了 Level 1，比较次数会减少！" << endl;
    
    cout << "\n========================================" << endl;
    cout << "  演示结束" << endl;
    cout << "========================================" << endl;
    
    return 0;
}

