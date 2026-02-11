#include <iostream>
using namespace std;

// 节点类
template<typename K, typename V>
class Node {
public:
    K key;
    V value;
    Node* next;
    
    Node(K k, V v) : key(k), value(v), next(nullptr) {}
};

// 有序链表类
template<typename K, typename V>
class SortedList {
private:
    Node<K, V>* head;
    int element_count;
    
public:
    // 构造函数
    SortedList() : head(nullptr), element_count(0) {
        cout << "有序链表已创建" << endl;
    }
    
    // 析构函数
    ~SortedList() {
        Node<K, V>* current = head;
        while (current != nullptr) {
            Node<K, V>* temp = current;
            current = current->next;
            delete temp;
        }
        cout << "有序链表已销毁，内存已释放" << endl;
    }
    
    // 插入元素
    void insert(K key, V value) {
        Node<K, V>* newNode = new Node<K, V>(key, value);
        
        // 情况1：链表为空，或新节点应该插在头部
        if (head == nullptr || head->key >= key) {
            if (head != nullptr && head->key == key) {
                cout << "key=" << key << " 已存在" << endl;
                delete newNode;
                return;
            }
            newNode->next = head;
            head = newNode;
            element_count++;
            cout << "插入成功: key=" << key << ", value=" << value << endl;
            return;
        }
        
        // 情况2：在中间或末尾插入
        Node<K, V>* current = head;
        
        while (current->next != nullptr && current->next->key < key) {
            current = current->next;
        }
        
        // 检查是否已存在
        if (current->next != nullptr && current->next->key == key) {
            cout << "key=" << key << " 已存在" << endl;
            delete newNode;
            return;
        }
        
        // 插入新节点
        newNode->next = current->next;
        current->next = newNode;
        element_count++;
        
        cout << "插入成功: key=" << key << ", value=" << value << endl;
    }
    
    // 查找元素
    bool search(K key) {
        cout << "\n--- 查找 key=" << key << " ---" << endl;
        Node<K, V>* current = head;
        
        while (current != nullptr) {
            if (current->key == key) {
                cout << "✓ 找到 key=" << key << ", value=" << current->value << endl;
                return true;
            }
            
            if (current->key > key) {
                break;
            }
            
            current = current->next;
        }
        
        cout << "✗ 未找到 key=" << key << endl;
        return false;
    }
    
    // 删除元素
    void remove(K key) {
        cout << "\n--- 删除 key=" << key << " ---" << endl;
        
        if (head == nullptr) {
            cout << "链表为空" << endl;
            return;
        }
        
        // 情况1：删除头节点
        if (head->key == key) {
            Node<K, V>* temp = head;
            head = head->next;
            delete temp;
            element_count--;
            cout << "✓ 删除成功: key=" << key << endl;
            return;
        }
        
        // 情况2：删除中间或末尾节点
        Node<K, V>* current = head;
        
        while (current->next != nullptr && current->next->key < key) {
            current = current->next;
        }
        
        if (current->next == nullptr || current->next->key != key) {
            cout << "✗ 未找到 key=" << key << endl;
            return;
        }
        
        Node<K, V>* temp = current->next;
        current->next = current->next->next;
        delete temp;
        element_count--;
        
        cout << "✓ 删除成功: key=" << key << endl;
    }
    
    // 显示链表
    void display() {
        cout << "\n========== 链表内容 ==========" << endl;
        Node<K, V>* current = head;
        
        if (current == nullptr) {
            cout << "链表为空" << endl;
            return;
        }
        
        while (current != nullptr) {
            cout << "[" << current->key << ":" << current->value << "]";
            if (current->next != nullptr) {
                cout << " -> ";
            }
            current = current->next;
        }
        cout << " -> NULL" << endl;
        cout << "总元素数: " << element_count << endl;
        cout << "=============================" << endl;
    }
    
    // 获取大小
    int size() {
        return element_count;
    }
};

// 主函数：演示程序
int main() {
    cout << "========================================" << endl;
    cout << "  阶段1：有序链表实现演示" << endl;
    cout << "========================================\n" << endl;
    
    // 创建一个键为int，值为string的有序链表
    SortedList<int, string> list;
    
    cout << "\n【测试1：插入元素】" << endl;
    cout << "-------------------" << endl;
    list.insert(3, "三");
    list.insert(1, "一");
    list.insert(5, "五");
    list.insert(2, "二");
    list.insert(4, "四");
    
    list.display();
    
    cout << "\n【测试2：插入重复元素】" << endl;
    cout << "-------------------" << endl;
    list.insert(3, "重复的三");  // 应该失败
    
    cout << "\n【测试3：查找元素】" << endl;
    cout << "-------------------" << endl;
    list.search(3);  // 应该找到
    list.search(6);  // 应该找不到
    
    cout << "\n【测试4：删除元素】" << endl;
    cout << "-------------------" << endl;
    list.remove(1);  // 删除头节点
    list.display();
    
    list.remove(5);  // 删除尾节点
    list.display();
    
    list.remove(3);  // 删除中间节点
    list.display();
    
    cout << "\n【测试5：删除不存在的元素】" << endl;
    cout << "-------------------" << endl;
    list.remove(10);  // 应该失败
    
    cout << "\n【测试6：获取链表大小】" << endl;
    cout << "-------------------" << endl;
    cout << "当前链表大小: " << list.size() << endl;
    
    cout << "\n========================================" << endl;
    cout << "  演示结束" << endl;
    cout << "========================================" << endl;
    
    return 0;
}

