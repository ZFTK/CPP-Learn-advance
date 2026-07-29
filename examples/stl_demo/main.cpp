/*
 * stl_demo.cpp —— STL 数据结构与算法速成示例
 *
 * 涵盖: vector, map, set, unordered_map, algorithm, 智能指针, string
 * 兼容: GCC 6+ / C++14
 *
 * 编译: g++ -std=c++14 stl_demo.cpp -o stl_demo
 */
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <string>
#include <numeric>

// ==================== 1. vector ====================
void demo_vector() {
    std::cout << "=== vector ===\n";
    std::vector<int> v = {3, 1, 4, 1, 5, 9};
    v.push_back(2);
    v.pop_back();
    v.insert(v.begin(), 0);
    for (auto x : v) std::cout << x << " ";
    std::cout << "\n";

    std::sort(v.begin(), v.end());
    auto it = std::find(v.begin(), v.end(), 5);
    int sum = std::accumulate(v.begin(), v.end(), 0);
    std::cout << "size=" << v.size() << " capacity=" << v.capacity() << "\n";
    std::cout << "front=" << v.front() << " back=" << v.back() << "\n";
    std::cout << "sum=" << sum << "\n";

    std::vector<std::string> names;
    names.reserve(100);  // 预分配
    std::cout << "---\n";
}

// ==================== 2. map / unordered_map ====================
void demo_map() {
    std::cout << "=== map / unordered_map ===\n";
    std::map<std::string, int> scores;
    scores["Alice"] = 95;
    scores["Bob"]   = 87;
    scores.emplace("Charlie", 92);

    auto it = scores.find("Bob");
    if (it != scores.end()) std::cout << "Bob: " << it->second << "\n";

    // 更新值
    scores["Alice"] = 98;
    // 遍历（C++14 兼容写法）
    for (const auto& pair : scores) {
        std::cout << "  " << pair.first << ": " << pair.second << "\n";
    }

    // unordered_map: 哈希表，O(1) 平均
    std::unordered_map<std::string, int> cache;
    cache["key1"] = 100;
    cache["key2"] = 200;
    std::cout << "cache[key1]=" << cache["key1"] << "\n";
    std::cout << "load_factor=" << cache.load_factor() << "\n";
    std::cout << "---\n";
}

// ==================== 3. set ====================
void demo_set() {
    std::cout << "=== set ===\n";
    std::set<int> s = {5, 2, 8, 2, 5};  // 去重+排序
    for (int x : s) std::cout << x << " ";  // 2 5 8
    std::cout << "\n";

    s.insert(3);
    s.erase(8);
    std::cout << "count(3)=" << s.count(3) << "\n";

    // 集合交集
    std::set<int> a = {1, 2, 3, 4};
    std::set<int> b = {3, 4, 5, 6};
    std::vector<int> inter;
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
                          std::back_inserter(inter));
    std::cout << "a n b: ";
    for (int x : inter) std::cout << x << " ";
    std::cout << "\n---\n";
}

// ==================== 4. algorithm ====================
void demo_algorithm() {
    std::cout << "=== algorithm ===\n";
    std::vector<int> v = {3, 7, 2, 9, 1, 5, 8, 4, 6};

    // 排序
    std::sort(v.begin(), v.end());
    std::sort(v.begin(), v.end(), std::greater<int>());

    // 查找
    auto it = std::find_if(v.begin(), v.end(), [](int x) { return x > 5; });
    if (it != v.end()) std::cout << "First > 5: " << *it << "\n";

    // 变换
    std::vector<int> squares(v.size());
    std::transform(v.begin(), v.end(), squares.begin(),
                   [](int x) { return x * x; });

    // 过滤
    std::vector<int> evens;
    std::copy_if(v.begin(), v.end(), std::back_inserter(evens),
                 [](int x) { return x % 2 == 0; });
    std::cout << "evens: ";
    for (int e : evens) std::cout << e << " ";
    std::cout << "\n";

    // 最值
    auto minmax = std::minmax_element(v.begin(), v.end());
    std::cout << "min=" << *minmax.first << " max=" << *minmax.second << "\n";

    // 二分查找
    std::sort(v.begin(), v.end());  // 必须先排序
    bool found = std::binary_search(v.begin(), v.end(), 5);
    std::cout << "binary_search(5)=" << found << "\n";
    std::cout << "---\n";
}

// ==================== 5. 智能指针 ====================
class Resource {
public:
    Resource(int id) : m_id(id) {
        std::cout << "  Resource(" << id << ") created\n";
    }
    ~Resource() { std::cout << "  Resource(" << m_id << ") destroyed\n"; }
    void use() { std::cout << "  Using Resource(" << m_id << ")\n"; }
private:
    int m_id;
};

void demo_smart_pointers() {
    std::cout << "=== 智能指针 ===\n";

    // unique_ptr: 独占所有权
    {
        auto p1 = std::make_unique<Resource>(1);
        p1->use();
        auto p2 = std::move(p1);  // 所有权转移
        if (!p1) std::cout << "  p1 is null after move\n";
    } // 自动析构

    // shared_ptr: 共享所有权，引用计数
    {
        auto sp1 = std::make_shared<Resource>(100);
        std::cout << "  use_count=" << sp1.use_count() << "\n";
        {
            auto sp2 = sp1;
            std::cout << "  use_count=" << sp1.use_count() << "\n";
        }
        std::cout << "  use_count=" << sp1.use_count() << "\n";
    }

    // weak_ptr: 不增加引用计数
    std::weak_ptr<Resource> wp;
    {
        auto sp = std::make_shared<Resource>(200);
        wp = sp;
        if (auto locked = wp.lock()) locked->use();
    }
    std::cout << "  wp.expired()=" << wp.expired() << "\n";
    std::cout << "---\n";
}

// ==================== 6. string ====================
void demo_string() {
    std::cout << "=== string ===\n";
    std::string s = "Hello, C++ World!";

    auto pos = s.find("C++");
    if (pos != std::string::npos)
        std::cout << "'C++' found at position " << pos << "\n";

    std::string sub = s.substr(7, 3);  // "C++"
    std::cout << "substr(7,3)=" << sub << "\n";

    s.replace(7, 3, "Modern C++");
    std::cout << s << "\n";

    // 分割字符串
    std::string csv = "apple,banana,orange";
    std::vector<std::string> parts;
    size_t start = 0, end;
    while ((end = csv.find(',', start)) != std::string::npos) {
        parts.push_back(csv.substr(start, end - start));
        start = end + 1;
    }
    parts.push_back(csv.substr(start));
    for (const auto& p : parts) std::cout << "  [" << p << "] ";
    std::cout << "\n---\n";
}

// ==================== 7. lambda ====================
void demo_lambda() {
    std::cout << "=== lambda ===\n";
    int threshold = 5;
    std::vector<int> v = {1, 3, 5, 7, 9};
    // [=] 按值捕获, [&] 按引用捕获, [threshold] 捕获特定变量
    int count = std::count_if(v.begin(), v.end(),
        [threshold](int x) { return x > threshold; });
    std::cout << "count > " << threshold << " = " << count << "\n";
    std::cout << "---\n";
}

int main() {
    demo_vector();
    demo_map();
    demo_set();
    demo_algorithm();
    demo_smart_pointers();
    demo_string();
    demo_lambda();
    std::cout << "=== STL Demo Complete ===\n";
    return 0;
}
