# cmake_demo —— CMake 项目结构示例
#
# 知识点：
#   1. cmake_minimum_required / project
#   2. set(CMAKE_CXX_STANDARD ...)
#   3. add_executable / add_library
#   4. target_include_directories
#   5. target_link_libraries
#   6. find_package
#   7. option (条件编译)
#   8. 子目录管理
#
# 构建：
#   mkdir build && cd build
#   cmake .. -G "MinGW Makefiles"  (或其他生成器)
#   cmake --build .
cmake_minimum_required(VERSION 3.16)
project(CmakeDemo VERSION 1.0 LANGUAGES CXX)

# === C++ 标准 ===
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === 静态库：数学工具 ===
add_library(math_utils STATIC math_utils.cpp)
target_include_directories(math_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# === 主程序 ===
add_executable(cmake_demo main.cpp)
target_link_libraries(cmake_demo PRIVATE math_utils)

# === 条件编译示例 ===
option(ENABLE_FEATURE_X "启用实验功能X" OFF)
if(ENABLE_FEATURE_X)
    target_compile_definitions(cmake_demo PRIVATE FEATURE_X_ENABLED)
    message(STATUS "Feature X enabled")
endif()

# === 查找外部依赖 ===
# find_package(OpenCV QUIET)
# if(OpenCV_FOUND)
#     target_link_libraries(cmake_demo PRIVATE ${OpenCV_LIBS})
#     target_compile_definitions(cmake_demo PRIVATE HAS_OPENCV)
# endif()

message(STATUS "CMake Demo configured successfully")
