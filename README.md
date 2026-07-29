# C++ 核心技能速成项目

> 面向有编程基础（LeetCode 500题），需在数天内快速上手 C++ 工程开发的综合教学项目。

## 📁 项目结构

```
E:\Cpro\C++学习进阶\
├── README.md                          # 本文件
├── docs/                              # 11篇知识文档 (详细讲解每个模块)
│   ├── 00-概览.md
│   ├── 01-STL.md
│   ├── 02-CMake.md
│   ├── 03-Git.md
│   ├── 04-JSON.md
│   ├── 05-多线程.md
│   ├── 06-SQLite.md
│   ├── 07-动态库插件.md
│   ├── 08-libcurl.md
│   ├── 09-Qt.md
│   ├── 10-OpenCV.md
│   └── 11-ONNX_Runtime.md
│
├── LearningAssistant/                 # 综合教学应用 (涵盖所有模块)
│   ├── CMakeLists.txt                 # 顶层构建文件
│   ├── third_party/                   # 第三方依赖 (sqlite3, nlohmann/json)
│   ├── plugins/                       # 示例动态库插件
│   └── src/
│       ├── main.cpp                   # 控制台入口 (整合所有模块)
│       ├── core/                      # 配置管理、日志、错误处理
│       ├── database/                  # SQLite 封装 + 笔记仓库
│       ├── threading/                 # 线程池 + 任务管理
│       ├── network/                   # HTTP客户端 + JSON解析
│       ├── plugins/                   # 动态库插件管理器
│       ├── gui/                       # Qt GUI 界面 (条件编译)
│       ├── ocr/                       # OpenCV 截图 + 图像处理
│       └── ai/                        # ONNX Runtime 推理引擎
│
└── examples/                          # 11个模块的独立可运行示例
    ├── stl_demo/      → STL 数据结构与算法
    ├── cmake_demo/    → CMake 项目构建
    ├── sqlite_demo/   → SQLite CRUD + 全文搜索
    ├── json_demo/     → JSON 解析+生成+文件读写
    ├── curl_demo/     → HTTP GET/POST + 多线程下载
    ├── thread_demo/   → 多线程 + 线程池 + async
    ├── plugin_demo/   → 动态库加载 + C接口导出
    ├── qt_demo/       → Qt 信号槽 + 控件
    ├── opencv_demo/   → 图像处理 + 边缘检测
    ├── onnx_demo/     → ONNX 模型推理
    └── git_demo/      → Git 协作指南
```

## 🚀 快速开始

### 1. 阅读文档（1-2天）

按优先级顺序阅读 `docs/` 目录下的文档：

| 优先级 | 文档 | 时间 |
|--------|------|------|
| P0 | 01-STL → 02-CMake → 03-Git | 上午 |
| P1 | 04-JSON → 05-多线程 → 06-SQLite → 07-插件 | 下午 |
| P2 | 08-libcurl → 09-Qt → 10-OpenCV → 11-ONNX | 次日上午 |

### 2. 运行独立示例

每个示例都是自包含的单文件（或最小项目），可直接编译运行：

```bash
# 编译 STL 示例（无需任何依赖）
cd examples/stl_demo
g++ -std=c++17 main.cpp -o stl_demo
./stl_demo

# 编译线程示例
cd examples/thread_demo
g++ -std=c++17 main.cpp -o thread_demo -lpthread
./thread_demo

# 编译 JSON 示例（无依赖）
cd examples/json_demo
g++ -std=c++17 main.cpp -o json_demo
./json_demo

# 编译 SQLite 示例（需 sqlite3 合并版）
# 下载 https://www.sqlite.org/download.html 中的 sqlite-amalgamation
cd examples/sqlite_demo
g++ -std=c++17 main.cpp sqlite3.c -o sqlite_demo -lpthread
./sqlite_demo
```

### 3. 构建综合应用

```bash
cd LearningAssistant
mkdir build && cd build

# 基础构建（STL + SQLite + 多线程 + JSON + 插件）
cmake .. -G "MinGW Makefiles"
cmake --build .

# 运行
./LearningAssistant

# 完整构建（需要安装 Qt/OpenCV/ONNX Runtime/libcurl）
cmake .. -DENABLE_QT=ON -DENABLE_OPENCV=ON -DENABLE_ONNX=ON
```

## 📚 核心知识点速查

### STL — 数据结构与算法
| 容器 | 底层 | 查找 | 插入 | 场景 |
|------|------|------|------|------|
| `vector` | 动态数组 | O(n) | O(1)尾 | 序列存储 |
| `map` | 红黑树 | O(log n) | O(log n) | 有序键值对 |
| `unordered_map` | 哈希表 | O(1)均 | O(1)均 | 快速查找 |
| `set` | 红黑树 | O(log n) | O(log n) | 唯一集合 |
| `deque` | 双端队列 | O(n) | O(1)两端 | 队列/栈 |

### CMake — 工程管理
```cmake
cmake_minimum_required(VERSION 3.16)    # 最低版本
project(MyApp LANGUAGES CXX)            # 项目定义
add_executable(app main.cpp)            # 可执行文件
add_library(lib STATIC src.cpp)         # 静态/动态库
target_link_libraries(app PRIVATE lib)  # 链接依赖
target_include_directories(lib PUBLIC include/)  # 头文件路径
find_package(Qt6 REQUIRED COMPONENTS Widgets)    # 外部依赖
```

### 多线程 — 四种并发模式
```cpp
// 1. 基础线程
std::thread t([] { doWork(); });  t.join();

// 2. 互斥锁
std::mutex mtx;
std::lock_guard<std::mutex> lock(mtx);  // RAII自动解锁

// 3. 条件变量（生产者-消费者）
std::condition_variable cv;
cv.wait(lock, []{ return ready; });

// 4. 异步任务
auto future = std::async(std::launch::async, heavyWork);
auto result = future.get();
```

### SQLite — 数据库操作
```cpp
sqlite3* db;
sqlite3_open("data.db", &db);
sqlite3_exec(db, "CREATE TABLE t(id INT, name TEXT)", ...);
// 预编译语句防注入
sqlite3_prepare_v2(db, "INSERT INTO t VALUES(?,?)", -1, &stmt, 0);
sqlite3_bind_int(stmt, 1, 42);
sqlite3_step(stmt);
sqlite3_finalize(stmt);
sqlite3_close(db);
```

### JSON — nlohmann/json
```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

json j = json::parse(R"({"name":"Alice","age":25})");
std::string name = j["name"];

json out = {{"status","ok"}, {"data",{1,2,3}}};
std::string s = out.dump(2);  // 美化输出

// 或使用本项目内置的简易解析器（la::JsonValue）
```

### 动态库插件
```cpp
// 导出函数 (C链接避免 name mangling)
extern "C" {
    __declspec(dllexport) IPlugin* createPlugin() { return new MyPlugin(); }
}
// 加载
HMODULE h = LoadLibrary("plugin.dll");           // Windows
void* h = dlopen("plugin.so", RTLD_NOW);          // Linux
auto create = (CreateFunc*)GetProcAddress(h, "createPlugin"); // 获取符号
```

## ⚙️ 开发环境要求

| 工具 | 最低版本 | 安装方式 |
|------|---------|---------|
| GCC | 7.0+ | MinGW-w64 / Linux apt |
| CMake | 3.16+ | https://cmake.org |
| Git | 2.30+ | https://git-scm.com |
| Qt (可选) | 5.15+ | https://www.qt.io |
| OpenCV (可选) | 4.5+ | vcpkg / apt |
| ONNX Runtime (可选) | 1.12+ | GitHub Releases |
| libcurl (可选) | 7.60+ | vcpkg / apt |

## 📖 学习路径建议

```
第1天上午: STL文档 → 运行 stl_demo → 看 LearningAssistant/src/core/
第1天下午: CMake文档 → 运行 cmake_demo → 多线程文档 → 运行 thread_demo
第1天晚上: Git文档 → 尝试 git 操作

第2天上午: SQLite文档 → 运行 sqlite_demo → 看 LearningAssistant/src/database/
第2天下午: JSON文档 → libcurl文档 → 看 LearningAssistant/src/network/
第2天晚上: 插件文档 → 运行 plugin_demo → 看 LearningAssistant/src/plugins/

第3天上午: Qt文档 → 运行 qt_demo (需安装Qt)
第3天下午: OpenCV文档 → ONNX文档 → 浏览对应示例
第3天晚上: 通读 LearningAssistant/main.cpp 整合理解

入职后:   将 LearningAssistant 项目作为参考模板，遇到问题回来查阅
```

## 🎯 项目特色

- **教学注释驱动**: 每个文件都标注了对应的知识点，代码即教程
- **渐进式学习**: 独立示例 → 模块源码 → 综合应用，三层递进
- **生产级代码风格**: 展示真实的 C++ 工程结构和命名规范
- **条件编译**: 缺少依赖也不影响其他模块，渐进安装
- **RAII 贯穿始终**: 展示现代 C++ 资源管理最佳实践
