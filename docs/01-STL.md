# 01 — C++ STL 核心组件速查

## 1. 核心概念

STL（Standard Template Library）是C++标准库的核心，提供了容器、算法、迭代器三大支柱。

- **容器**：管理数据集合（vector、map、set等），自动处理内存
- **算法**：操作数据的通用函数（sort、find、transform），与容器解耦
- **迭代器**：连接容器和算法的桥梁，提供统一遍历方式

**为什么用STL？**手写动态数组/链表/排序既容易出错又浪费时间。STL经过数十年的打磨，性能极致、bug极少，是生产环境的基石。

---

## 2. 快速上手

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

int main() {
    // ---- vector：动态数组 ----
    std::vector<int> v = {3, 1, 4, 1, 5, 9};

    // ---- algorithm：排序 & 查找 ----
    std::sort(v.begin(), v.end());                     // 升序：{1,1,3,4,5,9}
    auto it = std::find(v.begin(), v.end(), 4);
    if (it != v.end()) std::cout << "找到: " << *it << "\n";

    // ---- 范围for + 迭代器 ----
    for (int x : v) std::cout << x << " ";            // 1 1 3 4 5 9
    std::cout << "\n";

    // ---- transform：映射 ----
    std::vector<int> squared(v.size());
    std::transform(v.begin(), v.end(), squared.begin(),
                   [](int x) { return x * x; });
    // squared = {1, 1, 9, 16, 25, 81}
    return 0;
}
```

---

## 3. 常用API / 写法速查

### 3.1 容器选择指南

| 容器 | 底层结构 | 适用场景 |
|------|----------|----------|
| `vector` | 连续数组 | 随机访问、尾部增删、默认首选 |
| `list` | 双向链表 | 任意位置频繁插入/删除 |
| `map` | 红黑树 | 有序键值对，按键遍历 |
| `unordered_map` | 哈希表 | O(1)查找，无序 |
| `set` | 红黑树 | 去重 + 有序集合 |
| `deque` | 分段数组 | 双端快速增删 |

### 3.2 vector 常用操作

```cpp
std::vector<int> v;
v.push_back(10);          // 尾部追加
v.emplace_back(20);       // 原地构造，更高效
v.pop_back();             // 删除末尾
v.reserve(100);           // 预分配容量，避免反复扩容
v.resize(5, 0);           // 调整为5个元素，新元素填0
v.clear();                // 清空
v.insert(v.begin(), 42);  // 头部插入
v.erase(v.begin());       // 删除头部
```

### 3.3 map / unordered_map

```cpp
std::map<std::string, int> m;
m["apple"] = 5;
m.insert({"banana", 3});

// 安全查找：find 返回迭代器
if (auto it = m.find("cherry"); it != m.end()) {
    std::cout << it->second << "\n";
}

// C++17: insert_or_assign
m.insert_or_assign("apple", 10);

// 遍历
for (const auto& [key, val] : m) {  // C++17 结构化绑定
    std::cout << key << " = " << val << "\n";
}
```

### 3.4 algorithm 高频函数

```cpp
#include <algorithm>
#include <numeric>

std::vector<int> data = {5, 2, 8, 1, 9};

std::sort(data.begin(), data.end());                          // 升序
std::sort(data.begin(), data.end(), std::greater<>());        // 降序

auto it = std::find(data.begin(), data.end(), 8);             // 查找
int cnt = std::count(data.begin(), data.end(), 2);            // 计数
auto maxit = std::max_element(data.begin(), data.end());      // 最大值
auto minit = std::min_element(data.begin(), data.end());      // 最小值

std::reverse(data.begin(), data.end());                       // 反转
bool found = std::binary_search(data.begin(), data.end(), 5); // 二分（需有序）

// 复制满足条件的元素
std::vector<int> evens;
std::copy_if(data.begin(), data.end(), std::back_inserter(evens),
             [](int x) { return x % 2 == 0; });

// 累加
int sum = std::accumulate(data.begin(), data.end(), 0);

// 移除（移到末尾，返回新end；配合erase真删除）
auto newEnd = std::remove_if(data.begin(), data.end(),
                             [](int x) { return x % 2 == 0; });
data.erase(newEnd, data.end());  // erase-remove 惯用法
```

### 3.5 智能指针

```cpp
#include <memory>

// unique_ptr：独占所有权，不能拷贝，只能移动
auto p1 = std::make_unique<int>(42);       // 推荐：C++14起
std::unique_ptr<Foo> p2(new Foo());

auto p3 = std::move(p1);                   // 转移所有权
// p1 现在为空

// shared_ptr：共享所有权，引用计数
auto s1 = std::make_shared<int>(100);
auto s2 = s1;                              // 引用计数 = 2

// weak_ptr：不增加引用计数，用于打破循环引用
std::weak_ptr<int> w = s1;
if (auto locked = w.lock()) {              // 使用时需 lock() 提升为 shared_ptr
    std::cout << *locked << "\n";
}
```

### 3.6 string / string_view

```cpp
#include <string>
#include <string_view>

std::string s = "hello world";
s += "!";
s.find("world");                  // 返回位置
s.substr(0, 5);                   // "hello"
bool b = s.starts_with("he");     // C++20

// string_view：不拥有数据，零拷贝的只读视图
std::string_view sv = "hello world";
sv = sv.substr(6);                // "world" —— 零拷贝！只移动指针
// 注意：sv 不拥有数据，原数据销毁后 sv 悬空！
```

### 3.7 optional / span (C++17/20)

```cpp
// optional：可能不存在的值
#include <optional>
std::optional<int> divide(int a, int b) {
    if (b == 0) return std::nullopt;
    return a / b;
}
auto result = divide(10, 2);
if (result) std::cout << *result << "\n";     // 解引用
std::cout << result.value_or(-1) << "\n";     // 有则返回，无则默认-1

// span：不拥有数据的连续内存视图（C++20）
#include <span>
void process(std::span<int> data) {
    for (int x : data) std::cout << x << " ";
}
std::vector<int> v = {1, 2, 3};
process(v);       // vector 可隐式转 span
int arr[] = {4, 5, 6};
process(arr);     // 原生数组也可
```

---

## 4. 常见陷阱 & 最佳实践

| 陷阱 | 说明 | 正确做法 |
|------|------|----------|
| **迭代器失效** | vector扩容/insert/erase使迭代器失效 | erase后使用返回值：`it = v.erase(it)` |
| **map的[]副作用** | `m["key"]` 不存在时会插入默认值 | 只读时用`find()`或`at()` |
| **vector<bool>** | 特化版本，`operator[]`返回代理，非`bool&` | 避免使用，用`vector<char>`或`deque<bool>` |
| **string_view悬空** | 指向临时对象或已销毁的string | 确保源数据生命周期长于string_view |
| **循环中的shared_ptr** | A持有B的shared_ptr，B持有A的shared_ptr | 一方改用weak_ptr打破循环 |
| **未reserve** | 已知大小时反复push_back导致多次扩容 | 先用`reserve()`预分配 |

**最佳实践速记：**

- 默认选 `vector`，需要键值查找选 `unordered_map`
- 优先 `emplace_back` 而非 `push_back`
- 优先 `make_unique / make_shared` 而非 `new`
- 传参：只读参数用 `const T&` 或 `string_view`；转移所有权用 `unique_ptr`
- 结构体绑定成员用值类型，避免 `shared_ptr` 满天飞

---

## 5. 生产项目典型用法

### 场景：配置文件解析器

```cpp
#include <unordered_map>
#include <string>
#include <string_view>
#include <optional>
#include <fstream>
#include <sstream>

class ConfigParser {
public:
    // 加载 key=value 格式的配置文件
    bool load(std::string_view path) {
        std::ifstream file(path.data());
        if (!file) return false;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue; // 跳过空行和注释
            auto pos = line.find('=');
            if (pos == std::string::npos) continue;
            std::string key = line.substr(0, pos);
            std::string val = line.substr(pos + 1);
            // 去除首尾空格（省略实现）
            config_[std::move(key)] = std::move(val);
        }
        return true;
    }

    // 安全获取配置值
    std::optional<std::string> get(std::string_view key) const {
        auto it = config_.find(key.data());
        if (it != config_.end()) return it->second;
        return std::nullopt;
    }

    // 获取或使用默认值
    std::string get_or(std::string_view key, std::string_view def) const {
        return get(key).value_or(std::string(def));
    }

    // 遍历所有配置
    void for_each(const std::function<void(const std::string&, const std::string&)>& fn) const {
        for (const auto& [k, v] : config_) fn(k, v);
    }

    size_t size() const { return config_.size(); }

private:
    std::unordered_map<std::string, std::string> config_;
    // 为什么用unordered_map？配置项无序，但需要O(1)查找
};

// 使用示例
int main() {
    ConfigParser cfg;
    if (!cfg.load("app.conf")) {
        std::cerr << "加载配置失败\n";
        return 1;
    }

    int port = std::stoi(cfg.get_or("port", "8080"));
    std::string host = cfg.get_or("host", "0.0.0.0");
    bool debug = cfg.get_or("debug", "false") == "true";

    std::cout << "服务启动: " << host << ":" << port
              << (debug ? " [调试模式]" : "") << "\n";
    return 0;
}
```

---

## 参考链接

- [cppreference — C++ 标准库参考](https://zh.cppreference.com/)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
