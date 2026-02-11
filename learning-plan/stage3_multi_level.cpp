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
    Node<K, V>** forward;  // 指针数组
    int node_level;
    
    Node(K k, V v, int level) {
        key = k;
        value = v;
        node_level = level;
        
        // 分配指针数组
        forward = new Node<K, V>*[level + 1];
        
        // 初始化为 nullptr
        for (int i = 0; i <= level; i++) {
            forward[i] = nullptr;
        }
    }
    
    ~Node() {
        delete[] forward;
    }
};

// 多层跳表类
template<typename K, typename V>
class SkipList {
private:
    int max_level;           // 最大层数
    int current_level;       // 当前层数
    Node<K, V>* header;      // 头节点
    int element_count;       // 元素个数
    
public:
    // 构造函数
    SkipList(int max_lvl) : max_level(max_lvl), current_level(0), element_count(0) {
        srand(time(nullptr));
        
        // 创建头节点（哨兵节点）
        K k;
        V v;
        header = new Node<K, V>(k, v, max_level);
        
        cout << "跳表已创建，最大层数: " << max_level << endl;
    }
    
    // 析构函数
    ~SkipList() {
        Node<K, V>* current = header->forward[0];
        
        // 释放所有数据节点
        while (current != nullptr) {
            Node<K, V>* temp = current;
            current = current->forward[0];
            delete temp;
        }
        
        // 释放头节点
        delete header;
        
        cout << "跳表已销毁，内存已释放" << endl;
    }
    
    // 生成随机层数
    int get_random_level() {
        int level = 0;
        
        // 每次有 50% 的概率增加一层
        while (rand() % 2 && level < max_level) {
            level++;
        }
        
        return level;
    }
    
    // 插入元素
    void insert(K key, V value) {
        Node<K, V>* current = header;
        Node<K, V>* update[max_level + 1];
        
        // 初始化 update 数组
        for (int i = 0; i <= max_level; i++) {
            update[i] = nullptr;
        }
        
        // 从最高层开始向下查找
        for (int i = current_level; i >= 0; i--) {
            while (current->forward[i] != nullptr && 
                   current->forward[i]->key < key) {
                current = current->forward[i];
            }
            update[i] = current;
        }
        
        // 到达 Level 0
        current = current->forward[0];
        
        // 检查 key 是否已存在
        if (current != nullptr && current->key == key) {
            cout << "key=" << key << " 已存在" << endl;
            return;
        }
        
        // 生成随机层数
        int random_level = get_random_level();
        
        // 如果随机层数大于当前层数
        if (random_level > current_level) {
            for (int i = current_level + 1; i <= random_level; i++) {
                update[i] = header;
            }
            current_level = random_level;
        }
        
        // 创建新节点
        Node<K, V>* new_node = new Node<K, V>(key, value, random_level);
        
        // 在每一层插入新节点
        for (int i = 0; i <= random_level; i++) {
            new_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = new_node;
        }
        
        element_count++;
        cout << "插入成功: key=" << key << ", value=" << value 
             << ", level=" << random_level << endl;
    }
    
    // 查找元素
    bool search(K key) {
        cout << "\n--- 查找 key=" << key << " ---" << endl;
        Node<K, V>* current = header;
        int comparisons = 0;
        
        // 从最高层开始向下查找
        for (int i = current_level; i >= 0; i--) {
            while (current->forward[i] != nullptr && 
                   current->forward[i]->key < key) {
                current = current->forward[i];
                comparisons++;
            }
        }
        
        // 到达 Level 0
        current = current->forward[0];
        
        // 检查是否找到
        if (current != nullptr && current->key == key) {
            comparisons++;
            cout << "✓ 找到 key=" << key << ", value=" << current->value << endl;
            cout << "比较次数: " << comparisons << endl;
            return true;
        }
        
        cout << "✗ 未找到 key=" << key << endl;
        cout << "比较次数: " << comparisons << endl;
        return false;
    }
    
    // 删除元素
    void remove(K key) {
        cout << "\n--- 删除 key=" << key << " ---" << endl;
        Node<K, V>* current = header;
        Node<K, V>* update[max_level + 1];
        
        // 初始化 update 数组
        for (int i = 0; i <= max_level; i++) {
            update[i] = nullptr;
        }
        
        // 从最高层开始向下查找
        for (int i = current_level; i >= 0; i--) {
            while (current->forward[i] != nullptr && 
                   current->forward[i]->key < key) {
                current = current->forward[i];
            }
            update[i] = current;
        }
        
        // 到达 Level 0
        current = current->forward[0];
        
        // 检查是否找到
        if (current == nullptr || current->key != key) {
            cout << "✗ 未找到 key=" << key << endl;
            return;
        }
        
        // 在每一层删除节点
        for (int i = 0; i <= current_level; i++) {
            if (update[i]->forward[i] != current) {
                break;
            }
            update[i]->forward[i] = current->forward[i];
        }
        
        // 释放节点
        delete current;
        element_count--;
        
        // 更新当前层数（删除空层）
        while (current_level > 0 && 
               header->forward[current_level] == nullptr) {
            current_level--;
        }
        
        cout << "✓ 删除成功: key=" << key << endl;
    }
    
    // 显示跳表
    void display() {
        cout << "\n========== 跳表结构 ==========" << endl;
        
        for (int i = current_level; i >= 0; i--) {
            Node<K, V>* node = header->forward[i];
            cout << "Level " << i << ": ";
            
            while (node != nullptr) {
                cout << "[" << node->key << ":" << node->value << "]";
                if (node->forward[i] != nullptr) {
                    cout << " -> ";
                }
                node = node->forward[i];
            }
            cout << " -> NULL" << endl;
        }
        
        cout << "当前层数: " << current_level << endl;
        cout << "元素个数: " << element_count << endl;
        cout << "==============================" << endl;
    }
    
    // 获取大小
    int size() {
        return element_count;
    }
};

// 主函数：演示程序
int main() {
    cout << "========================================" << endl;
    cout << "  阶段3：多层跳表实现演示" << endl;
    cout << "========================================\n" << endl;
    
    // 创建最大层数为 6 的跳表
    SkipList<int, string> skipList(6);
    
    cout << "\n【测试1：插入元素】" << endl;
    cout << "-------------------" << endl;
    skipList.insert(3, "三");
    skipList.insert(6, "六");
    skipList.insert(7, "七");
    skipList.insert(9, "九");
    skipList.insert(12, "十二");
    skipList.insert(19, "十九");
    skipList.insert(17, "十七");
    skipList.insert(26, "二十六");
    skipList.insert(21, "二十一");
    skipList.insert(25, "二十五");
    
    skipList.display();
    
    cout << "\n【测试2：查找元素】" << endl;
    cout << "-------------------" << endl;
    skipList.search(19);  // 应该找到
    skipList.search(15);  // 应该找不到
    skipList.search(3);   // 测试头节点
    skipList.search(26);  // 测试尾节点
    
    cout << "\n【测试3：删除元素】" << endl;
    cout << "-------------------" << endl;
    skipList.remove(19);
    skipList.display();
    
    skipList.remove(7);
    skipList.display();
    
    skipList.remove(3);
    skipList.display();
    
    cout << "\n【测试4：删除不存在的元素】" << endl;
    cout << "-------------------" << endl;
    skipList.remove(100);
    
    cout << "\n【测试5：插入更多元素测试性能】" << endl;
    cout << "-------------------" << endl;
    
    SkipList<int, string> bigList(16);
    
    cout << "插入 100 个元素..." << endl;
    for (int i = 1; i <= 100; i++) {
        bigList.insert(i, "值" + to_string(i));
    }
    
    cout << "\n查找测试：" << endl;
    bigList.search(50);
    bigList.search(99);
    bigList.search(1);
    
    cout << "\n跳表大小: " << bigList.size() << endl;
    
    cout << "\n========================================" << endl;
    cout << "  演示结束" << endl;
    cout << "========================================" << endl;
    
    return 0;
}

