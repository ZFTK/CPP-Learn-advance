/*
 * plugin_manager.h —— 动态库插件管理器
 *
 * 知识点：
 *   1. 动态库加载 (Windows: LoadLibrary / Linux: dlopen)
 *   2. 导出 C 接口 —— extern "C" 避免 name mangling
 *   3. 工厂模式 —— 插件导出创建函数
 *   4. 函数指针 —— 运行时绑定
 *   5. 插件发现 —— 扫描目录加载所有 .dll/.so
 */
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace la {

// ============================================================
// 插件接口定义
// 所有插件必须实现此接口
// ============================================================

/// @brief 插件基类 —— 所有插件继承此接口
class IPlugin {
public:
    virtual ~IPlugin() = default;

    /// 插件名称
    virtual const char* name() const = 0;
    /// 插件版本
    virtual const char* version() const = 0;
    /// 初始化（加载时调用）
    virtual bool initialize() = 0;
    /// 清理（卸载时调用）
    virtual void shutdown() = 0;
};

// ============================================================
// 插件元信息
// ============================================================
struct PluginInfo {
    std::string name;
    std::string version;
    std::string filepath;
    IPlugin* instance = nullptr;
};

// ============================================================
// 插件管理器
// ============================================================

/// 插件工厂函数类型（C链接，避免name mangling）
/// 每个插件 DLL/SO 必须导出此函数
using CreatePluginFunc = IPlugin* (*)();
using DestroyPluginFunc = void (*)(IPlugin*);

/// @brief 插件管理器
/// 负责发现、加载、管理所有插件
class PluginManager {
public:
    PluginManager() = default;
    ~PluginManager();

    // 禁止拷贝
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    /// 从指定目录加载所有插件
    size_t loadPluginsFrom(const std::string& directory);

    /// 加载单个插件
    bool loadPlugin(const std::string& filepath);

    /// 卸载所有插件
    void unloadAll();

    /// 获取已加载的插件列表
    const std::vector<PluginInfo>& plugins() const { return m_plugins; }

    /// 按名称查找插件
    IPlugin* findByName(const std::string& name);

    /// 对所有插件执行操作
    void forEach(const std::function<void(IPlugin*)>& func);

private:
    struct LoadedPlugin {
        void* handle = nullptr;          // HMODULE / void*
        IPlugin* instance = nullptr;
        DestroyPluginFunc destroyFunc = nullptr;
    };

    // 从动态库中解析符号
    void* resolveSymbol(void* handle, const std::string& name);

    // 获取动态库文件扩展名
    static const char* extension();

    std::vector<PluginInfo> m_plugins;
    std::vector<LoadedPlugin> m_loaded;
};

} // namespace la
