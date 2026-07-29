/*
 * plugin_demo_main.cpp —— 插件加载器（主程序）
 *
 * 演示：加载动态库，获取函数指针，调用插件
 *
 * 编译 (Windows):
 *   g++ -std=c++17 plugin_demo_main.cpp -o plugin_loader
 *   g++ -std=c++17 -shared plugin_a.cpp -o plugin_a.dll
 *
 * 编译 (Linux):
 *   g++ -std=c++17 plugin_demo_main.cpp -o plugin_loader -ldl
 *   g++ -std=c++17 -shared -fPIC plugin_a.cpp -o libplugin_a.so
 */
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#define LOAD_LIB(path)   LoadLibraryW(path)
#define GET_FUNC(h, name) GetProcAddress((HMODULE)h, name)
#define FREE_LIB(h)      FreeLibrary((HMODULE)h)
static std::wstring toWide(const std::string& s) {
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring ws(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], len);
    return ws;
}
#else
#include <dlfcn.h>
#define LOAD_LIB(path)   dlopen(path, RTLD_NOW)
#define GET_FUNC(h, name) dlsym(h, name)
#define FREE_LIB(h)      dlclose(h)
#endif

// ==================== 插件接口 ====================
// 插件必须实现的函数签名（C链接，无name mangling）
using PluginInitFunc   = void (*)();
using PluginRunFunc    = int  (*)(int, int);
using PluginNameFunc   = const char* (*)();

// ==================== 主程序 ====================
int main() {
#ifdef _WIN32
    const char* pluginPath = "plugin_a.dll";
#else
    const char* pluginPath = "./libplugin_a.so";
#endif

    std::cout << "=== 动态库插件演示 ===\n";
    std::cout << "加载插件: " << pluginPath << "\n";

    // 1. 加载动态库
#ifdef _WIN32
    void* handle = LOAD_LIB(toWide(pluginPath).c_str());
#else
    void* handle = LOAD_LIB(pluginPath);
#endif

    if (!handle) {
        std::cerr << "加载失败！请先编译插件: g++ -shared plugin_a.cpp -o plugin_a.dll\n";
        return 1;
    }
    std::cout << "动态库加载成功\n";

    // 2. 获取函数指针 —— extern "C" 确保函数名不变
    auto initFunc = (PluginInitFunc)GET_FUNC(handle, "plugin_init");
    auto runFunc  = (PluginRunFunc)GET_FUNC(handle, "plugin_run");
    auto nameFunc = (PluginNameFunc)GET_FUNC(handle, "plugin_name");

    if (!initFunc || !runFunc || !nameFunc) {
        std::cerr << "找不到导出函数\n";
        FREE_LIB(handle);
        return 1;
    }

    // 3. 调用插件函数
    std::cout << "插件名称: " << nameFunc() << "\n";
    initFunc();
    int result = runFunc(10, 20);
    std::cout << "plugin_run(10, 20) = " << result << "\n";

    // 4. 卸载动态库
    FREE_LIB(handle);
    std::cout << "插件已卸载\n";

    /*
     * ===== 关键知识点 =====
     *
     * 1. extern "C" { ... } 阻止 C++ name mangling
     *    - C++ 会把函数名"修饰"成类似 _Z11plugin_runii 的名字
     *    - extern "C" 保持原名 plugin_run，便于动态查找
     *
     * 2. __declspec(dllexport) (Windows) 或 visibility("default") (Linux)
     *    标记需要导出的函数
     *
     * 3. 工厂模式：
     *    - 插件导出 createPlugin() 返回接口指针
     *    - 主程序通过虚函数调用插件
     *    - 这种方式支持更复杂的接口（见 LearningAssistant 项目）
     *
     * 4. 安全的插件卸载顺序：
     *    - 先调用插件的清理函数
     *    - 再释放插件对象
     *    - 最后卸载动态库
     */
    return 0;
}
