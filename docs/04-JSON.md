# 04 — nlohmann/json 使用指南

## 1. 核心概念

[nlohmann/json](https://github.com/nlohmann/json) 是C++最流行的JSON库，header-only，零依赖。它的设计哲学是"像写Python一样操作JSON"——用直观的语法完成序列化/反序列化。

**特性：**符合JSON标准、支持STL容器互转、BSON/MessagePack/CBOR等二进制格式、JSON Pointer / JSON Patch、编译期类型安全。

---

## 2. 快速上手

### CMake 引入

```cmake
# 方式一：系统已安装
find_package(nlohmann_json 3.11 REQUIRED)
target_link_libraries(myapp PRIVATE nlohmann_json::nlohmann_json)

# 方式二：FetchContent
include(FetchContent)
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)
target_link_libraries(myapp PRIVATE nlohmann_json::nlohmann_json)
```

### 最小示例

```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// ---- 构建JSON ----
json j = {
    {"name", "张三"},
    {"age", 28},
    {"skills", {"C++", "Python", "CMake"}},
    {"active", true},
    {"address", {{"city", "北京"}, {"zip", "100000"}}}
};

// ---- 序列化为字符串 ----
std::string s = j.dump();       // 紧凑格式
std::string s2 = j.dump(4);     // 4空格缩进

// ---- 解析字符串 ----
json parsed = json::parse(R"({"name":"李四","age":30})");
std::cout << parsed["name"] << "\n";   // "李四"
```

---

## 3. 常用API / 写法速查

### 3.1 基本读写

```cpp
json j;

// 赋值
j["name"] = "王五";
j["score"] = 95.5;
j["tags"] = {"math", "physics"};
j["nested"]["key"] = "value";

// 读取（带异常保护）
try {
    std::string name = j.at("name").get<std::string>(); // 不存在抛异常
    int score = j.value("score", 0);                     // 不存在返回默认
    std::string city = j.value("/address/city"_json_pointer, "未知");
} catch (const json::exception& e) {
    std::cerr << e.what() << "\n";
}

// 类型判断
if (j["score"].is_number()) { /* ... */ }
if (j.contains("name")) {       // C++20风格
    // j 中有 "name" 键
}
```

### 3.2 JSON ↔ 结构体互转

```cpp
struct Person {
    std::string name;
    int age;
    std::vector<std::string> hobbies;
};

// C++ → JSON（使用 NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE）
// NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Person, name, age, hobbies)

// 或者手动实现 to_json / from_json
void to_json(json& j, const Person& p) {
    j = json{{"name", p.name}, {"age", p.age}, {"hobbies", p.hobbies}};
}

void from_json(const json& j, Person& p) {
    j.at("name").get_to(p.name);
    j.at("age").get_to(p.age);
    j.at("hobbies").get_to(p.hobbies);
}

// 使用
Person me{"张三", 28, {"编程", "跑步"}};
json j = me;                        // 自动调用 to_json
auto str = j.dump();

Person you = j.get<Person>();       // 自动调用 from_json
```

### 3.3 文件读写

```cpp
#include <fstream>

// 写文件
json config = {{"host", "0.0.0.0"}, {"port", 8080}};
std::ofstream out("config.json");
out << std::setw(4) << config << std::endl;

// 读文件
std::ifstream in("config.json");
json loaded = json::parse(in);
// 或者
// json loaded;
// in >> loaded;
```

### 3.4 数组遍历与操作

```cpp
json arr = json::array();       // 或者 json arr = json::array({1,2,3});
arr.push_back(42);
arr.emplace_back("hello");

for (auto& item : arr) {
    std::cout << item << " ";
}

// 类型混合的JSON数组
json mixed = {1, "two", 3.0, true, nullptr};
for (const auto& el : mixed) {
    if (el.is_number_integer()) std::cout << "整数: " << el << "\n";
    else if (el.is_string())   std::cout << "字符串: " << el << "\n";
}
```

### 3.5 JSON Pointer（路径访问）

```cpp
json doc = R"({
    "users": [
        {"name": "Alice", "email": "alice@example.com"},
        {"name": "Bob",   "email": "bob@example.com"}
    ]
})"_json;

// 用JSON Pointer访问
std::string email = doc["/users/1/email"_json_pointer]; // "bob@example.com"

// 修改
doc["/users/0/name"_json_pointer] = "Alicia";

// 检查路径是否存在
if (!doc["/config/debug"_json_pointer].is_null()) { /* ... */ }
```

### 3.6 JSON Patch（RFC 6902）

```cpp
json doc = {{"title", "Goodbye!"}};
json patch = R"([
    { "op": "replace", "path": "/title", "value": "Hello!" },
    { "op": "add", "path": "/author", "value": "John" }
])"_json;

doc = doc.patch(patch);
// doc = {"title":"Hello!", "author":"John"}
```

---

## 4. 常见陷阱 & 最佳实践

| 陷阱 | 说明 | 正确做法 |
|------|------|----------|
| **operator[]隐式创建** | `j["key"]` 访问不存在的键时会创建null值 | 只读时用 `j.at("key")` 或 `j.value("key", def)` |
| **类型不匹配** | `j["age"].get<std::string>()` 当age是数字时抛异常 | 先检查 `j["age"].is_number()` |
| **隐式类型转换** | `int x = j["count"]` 当值是true时x=1，容易埋雷 | 显式 `.get<int>()` |
| **parse异常未捕获** | 解析非法JSON直接抛异常 | 用 `try-catch` 包裹 `json::parse()` |
| **大文件一次性加载** | 大JSON文件内存爆炸 | 流式解析或用二进制格式(BSON) |
| **结构体宏陷阱** | `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE` 必须有默认构造函数 | 确保结构体有 `= default` 构造函数 |

**最佳实践：**

- 对外接口始终用 `json::parse()` 的 try-catch 保护
- `dump()` 参数为空时紧凑，参数为4时适合日志和配置文件
- 用 `_json` 字面量后缀可直接构造：`auto j = "{\"a\":1}"_json;`
- 结构体互转优先手动实现 to_json/from_json，给予完全控制

---

## 5. 生产项目典型用法

### 场景：HTTP API 客户端

```cpp
#include <nlohmann/json.hpp>
#include <fstream>
#include <optional>
#include <string>

using json = nlohmann::json;

// API响应解析
struct ApiResponse {
    int code;
    std::string message;
    json data;

    static std::optional<ApiResponse> parse(const std::string& raw) {
        try {
            json j = json::parse(raw);
            return ApiResponse{
                j.at("code").get<int>(),
                j.value("message", ""),
                j.value("data", json::object())
            };
        } catch (const json::exception& e) {
            std::cerr << "[JSON Parse Error] " << e.what() << "\n";
            return std::nullopt;
        }
    }
};

// 配置文件管理器
class ConfigManager {
public:
    bool load(const std::string& path) {
        try {
            std::ifstream f(path);
            if (!f) return false;
            data_ = json::parse(f);
            path_ = path;
            return true;
        } catch (...) { return false; }
    }

    bool save() {
        std::ofstream f(path_);
        if (!f) return false;
        f << std::setw(4) << data_ << std::endl;
        return true;
    }

    template<typename T>
    T get(const std::string& key, T default_val) const {
        return data_.value(key, default_val);
    }

    json& operator[](const std::string& key) { return data_[key]; }

private:
    json data_;
    std::string path_;
};

// 使用
int main() {
    // 模拟API响应
    std::string raw_response = R"({
        "code": 200,
        "message": "ok",
        "data": {
            "users": [
                {"id": 1, "name": "Alice"},
                {"id": 2, "name": "Bob"}
            ],
            "total": 2
        }
    })";

    auto resp = ApiResponse::parse(raw_response);
    if (resp && resp->code == 200) {
        int total = resp->data["total"];
        for (const auto& user : resp->data["users"]) {
            std::cout << "用户: " << user["name"] << "\n";
        }
    }

    return 0;
}
```

---

## 参考链接

- [nlohmann/json GitHub](https://github.com/nlohmann/json)
- [JSON for Modern C++ 文档](https://json.nlohmann.me/)
