# 02 — 现代 CMake 实战指南

## 1. 核心概念

CMake 是C/C++生态的事实标准构建系统生成器。它不直接编译代码，而是生成平台相关的构建文件（Makefile、Visual Studio解决方案、Ninja等）。

- **"现代CMake"** = CMake 3.12+，核心理念：**一切围绕target**（可执行文件/库），通过target的属性传递编译选项和依赖，告别全局变量污染。

**为什么用CMake？**跨平台、IDE友好、包管理（find_package / FetchContent）、条件编译，是几乎所有C++开源项目的标配。

---

## 2. 快速上手

### 最小项目结构

```
my_project/
├── CMakeLists.txt          # 顶层
├── src/
│   ├── CMakeLists.txt      # 子目录
│   └── main.cpp
└── include/
    └── mylib/
        └── mylib.h
```

### 顶层 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyApp VERSION 1.0.0 LANGUAGES CXX)

# C++17标准，不是仅编译期，而是强制的
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src)
```

### src/CMakeLists.txt

```cmake
# 创建库
add_library(mylib STATIC
    mylib.cpp
)

# 指定头文件搜索路径（PUBLIC：使用者也需要）
target_include_directories(mylib PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# 创建可执行文件
add_executable(myapp main.cpp)

# 链接库
target_link_libraries(myapp PRIVATE mylib)
```

### 构建 & 运行

```bash
# 配置
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build build --config Release --parallel 8

# 运行
./build/src/myapp
```

---

## 3. 常用命令速查

### 3.1 target_* 系列（现代CMake核心）

```cmake
add_library(foo STATIC foo.cpp)       # 静态库
add_library(bar SHARED bar.cpp)       # 动态库
add_library(baz INTERFACE)            # 纯头文件库（header-only）
add_executable(myapp main.cpp)        # 可执行文件

target_include_directories(foo PUBLIC include/)   # 头文件路径
target_compile_definitions(foo PRIVATE DEBUG_MODE) # 宏定义
target_compile_options(foo PRIVATE -Wall -Wextra)  # 编译选项
target_link_libraries(myapp PRIVATE foo)           # 链接依赖

# 传播级别：
# PRIVATE   — 只有自己用
# INTERFACE — 只有使用者需要（header-only库常用）
# PUBLIC    — 自己 + 使用者都需要
```

### 3.2 find_package — 引入第三方库

```cmake
# Qt
find_package(Qt6 REQUIRED COMPONENTS Widgets)
target_link_libraries(myapp PRIVATE Qt6::Widgets)

# OpenCV
find_package(OpenCV REQUIRED)
target_link_libraries(myapp PRIVATE ${OpenCV_LIBS})

# nlohmann_json (header-only)
find_package(nlohmann_json 3.11 REQUIRED)
target_link_libraries(myapp PRIVATE nlohmann_json::nlohmann_json)
```

### 3.3 FetchContent — 源码级引入第三方

```cmake
include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.13.0
)
FetchContent_MakeAvailable(spdlog)

target_link_libraries(myapp PRIVATE spdlog::spdlog)
```

### 3.4 条件编译

```cmake
option(ENABLE_TESTS "Build unit tests" ON)

if(ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 平台判断
if(WIN32)
    target_compile_definitions(myapp PRIVATE PLATFORM_WINDOWS)
elseif(APPLE)
    target_compile_definitions(myapp PRIVATE PLATFORM_MACOS)
elseif(UNIX)
    target_compile_definitions(myapp PRIVATE PLATFORM_LINUX)
endif()
# 更好的方式：if(CMAKE_SYSTEM_NAME STREQUAL "Windows") ...
```

### 3.5 安装规则

```cmake
include(GNUInstallDirs)

install(TARGETS mylib
    EXPORT  mylib-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 导出以便其他项目 find_package
install(EXPORT mylib-targets
    FILE mylibConfig.cmake
    NAMESPACE mylib::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mylib
)
```

### 3.6 常用变量

```cmake
${PROJECT_SOURCE_DIR}     # 顶层 CMakeLists.txt 所在目录
${PROJECT_BINARY_DIR}     # 构建目录
${CMAKE_CURRENT_SOURCE_DIR} # 当前 CMakeLists.txt 所在目录
${CMAKE_INSTALL_PREFIX}   # 安装路径（默认 /usr/local 或 C:/Program Files）
```

---

## 4. 常见陷阱 & 最佳实践

| 陷阱 | 说明 | 正确做法 |
|------|------|----------|
| **全局 include_directories** | 污染所有target | 改用`target_include_directories` |
| **全局 link_libraries** | 同上，不可控 | 改用`target_link_libraries` |
| **未设 CXX_STANDARD_REQUIRED** | 编译器不支持时静默降级 | 始终设置`ON` |
| **硬编码路径** | 不可移植 | 使用`${PROJECT_SOURCE_DIR}`等变量 |
| **add_subdirectory 顺序错误** | target未定义就link | 先`add_subdirectory`库，再link |
| **find_package 不检查 REQUIRED** | 库不存在时继续配置，后面才报错 | 加上`REQUIRED` |
| **在CMake中使用 file(GLOB)** | 新增文件不会自动重新配置 | 显式列出源文件，或只在开发期用`CONFIGURE_DEPENDS` |

**最佳实践速记：**

- CMake版本 ≥ 3.16，目标平台都能装
- `target_*` 命令为主，尽量不用全局命令
- 库的公开头文件路径设为 `PUBLIC`，内部依赖设为 `PRIVATE`
- 用 `FetchContent` 管理第三方依赖比 git submodule 省心
- `cmake -B build` 和 `cmake --build build` 统一脚本

---

## 5. 生产项目典型用法

### 完整项目结构

```
my_project/
├── CMakeLists.txt
├── cmake/
│   └── CompilerWarnings.cmake       # 公共编译选项模块
├── src/
│   ├── CMakeLists.txt
│   ├── core/
│   │   ├── CMakeLists.txt
│   │   └── engine.cpp
│   ├── ui/
│   │   ├── CMakeLists.txt
│   │   └── widget.cpp
│   └── main.cpp
├── tests/
│   ├── CMakeLists.txt
│   └── test_engine.cpp
├── include/
│   └── myproject/
│       ├── core/engine.h
│       └── ui/widget.h
└── third_party/
    └── CMakeLists.txt               # FetchContent 集中管理
```

### 顶层 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyProject VERSION 2.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 全局编译选项
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)

# 选项
option(BUILD_TESTS "构建测试" ON)
option(BUILD_UI    "构建UI模块" ON)

# 第三方依赖
add_subdirectory(third_party)

# 子模块
add_subdirectory(src/core)
if(BUILD_UI)
    add_subdirectory(src/ui)
endif()
add_subdirectory(src)

# 测试
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

### cmake/CompilerWarnings.cmake

```cmake
function(set_compiler_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()
```

### third_party/CMakeLists.txt

```cmake
include(FetchContent)

# spdlog 日志库
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.13.0
)
FetchContent_MakeAvailable(spdlog)

# catch2 测试框架（仅测试需要）
if(BUILD_TESTS)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.0
    )
    FetchContent_MakeAvailable(Catch2)
endif()
```

---

## 参考链接

- [CMake 官方文档](https://cmake.org/documentation/)
- [Modern CMake 介绍](https://cliutils.gitlab.io/modern-cmake/)
- [Effective Modern CMake](https://gist.github.com/mbinna/c61dbb39bca0e4fb7d1f73b0d66a4fd1)
