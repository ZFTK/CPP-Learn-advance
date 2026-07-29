// cmake_demo main.cpp —— CMake 项目管理示例
#include <iostream>
#include "math_utils.h"

int main() {
    std::cout << "=== CMake 项目演示 ===\n";
    std::cout << "add(3, 4) = " << add(3, 4) << "\n";
    std::cout << "multiply(3, 4) = " << multiply(3, 4) << "\n";
    std::cout << "项目版本: " << CMAKE_DEMO_VERSION << "\n";

#ifdef FEATURE_X_ENABLED
    std::cout << "实验功能X已启用\n";
#endif

    return 0;
}
