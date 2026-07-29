/*
 * hello_plugin.cpp —— 示例插件
 *
 * 知识点：
 *   1. extern "C" —— 避免 C++ name mangling，确保导出符号名不变
 *   2. __declspec(dllexport) —— Windows DLL 导出
 *   3. 实现 IPlugin 接口
 *   4. 编译为独立的 .dll/.so
 *
 * 构建：会被 CMake 编译为 plugins/hello_plugin.dll
 * 主程序通过 PluginManager 加载此 DLL
 */
#include "plugins/plugin_manager.h"
#include <iostream>

// ============================================================
// 插件实现
// ============================================================
class HelloPlugin : public la::IPlugin {
public:
    const char* name() const override { return "HelloPlugin"; }
    const char* version() const override { return "1.0.0"; }

    bool initialize() override {
        std::cout << "[HelloPlugin] 初始化完成！\n";
        return true;
    }

    void shutdown() override {
        std::cout << "[HelloPlugin] 正在关闭...\n";
    }
};

// ============================================================
// 导出函数 —— 必须使用 extern "C" 避免 name mangling
// ============================================================

#ifdef _WIN32
// __declspec(dllexport): Windows 需要在 DLL 导出函数前声明
#define PLUGIN_API __declspec(dllexport)
#else
// Linux/macOS: 不需要特殊声明（或使用 __attribute__((visibility("default")))）
#define PLUGIN_API
#endif

extern "C" {
    /// 插件工厂函数 —— 创建插件实例
    /// 主程序通过 GetProcAddress("createPlugin") 获取此函数
    PLUGIN_API la::IPlugin* createPlugin() {
        return new HelloPlugin();
    }

    /// 插件销毁函数 —— 销毁插件实例
    PLUGIN_API void destroyPlugin(la::IPlugin* plugin) {
        delete plugin;
    }
}

/*
 * ===== 测试方法 =====
 * 1. 编译整个项目: mkdir build && cd build && cmake .. && cmake --build .
 * 2. 运行主程序: ./LearningAssistant
 * 3. 主程序自动从 plugins/ 目录加载此插件
 * 4. 在菜单选择 8 查看插件状态
 *
 * ===== 如何添加新插件 =====
 * 1. 在 plugins/ 下创建新目录
 * 2. 实现 IPlugin 接口
 * 3. 导出 createPlugin / destroyPlugin
 * 4. 在 plugins/CMakeLists.txt 中添加 add_library
 * 5. 编译，DLL 自动放到 build/plugins/
 */
