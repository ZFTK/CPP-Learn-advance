/*
 * plugin_a.cpp —— 示例插件实现
 *
 * 编译为动态库:
 *   Windows: g++ -std=c++17 -shared plugin_a.cpp -o plugin_a.dll
 *   Linux:   g++ -std=c++17 -shared -fPIC plugin_a.cpp -o libplugin_a.so
 */
#include <iostream>

// Windows DLL 导出宏
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

// ===== extern "C" 阻止 C++ name mangling =====
// 没有extern "C": 函数名变成 _Z11plugin_initv 等
// 有extern "C":   函数名保持 plugin_init
extern "C" {
    EXPORT void plugin_init() {
        std::cout << "  [Plugin A] 初始化完成\n";
    }

    EXPORT int plugin_run(int a, int b) {
        std::cout << "  [Plugin A] 执行: " << a << " + " << b << "\n";
        return a + b;
    }

    EXPORT const char* plugin_name() {
        return "PluginA v1.0";
    }
}
