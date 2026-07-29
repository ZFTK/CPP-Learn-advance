/*
 * plugin_manager.cpp —— 动态库插件管理器实现
 *
 * 核心知识点：
 *   1. Windows: LoadLibraryW / GetProcAddress / FreeLibrary
 *   2. Linux:   dlopen / dlsym / dlclose
 *   3. extern "C" 导出的函数名不会被C++ name mangling修改
 *   4. 跨平台条件编译
 */
#include "plugins/plugin_manager.h"
#include "core/logger.h"
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace la {

// ==================== 跨平台包装 ====================

#ifdef _WIN32
static void* loadLibrary(const std::string& path) {
    // LoadLibraryW: 加载 DLL 到进程地址空间
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    std::wstring wpath(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wpath[0], wlen);
    return LoadLibraryW(wpath.c_str());
}

static void* getSymbol(void* handle, const std::string& name) {
    // GetProcAddress: 从 DLL 中获取函数地址
    return GetProcAddress((HMODULE)handle, name.c_str());
}

static void freeLibrary(void* handle) {
    // FreeLibrary: 卸载 DLL
    FreeLibrary((HMODULE)handle);
}
#else
static void* loadLibrary(const std::string& path) {
    // dlopen: 加载共享库
    return dlopen(path.c_str(), RTLD_NOW);
}

static void* getSymbol(void* handle, const std::string& name) {
    // dlsym: 获取符号地址
    return dlsym(handle, name.c_str());
}

static void freeLibrary(void* handle) {
    // dlclose: 卸载共享库
    dlclose(handle);
}
#endif

// ==================== PluginManager 实现 ====================

PluginManager::~PluginManager() {
    unloadAll();
}

const char* PluginManager::extension() {
#ifdef _WIN32
    return ".dll";
#else
    return ".so";
#endif
}

void* PluginManager::resolveSymbol(void* handle, const std::string& name) {
    return getSymbol(handle, name);
}

bool PluginManager::loadPlugin(const std::string& filepath) {
    // 1. 加载动态库
    void* handle = loadLibrary(filepath);
    if (!handle) {
        LA_LOG_ERROR("Failed to load plugin: " + filepath);
        return false;
    }

    // 2. 获取创建函数 —— createPlugin
    // extern "C" 确保导出符号名不变
    auto createFunc = reinterpret_cast<CreatePluginFunc>(resolveSymbol(handle, "createPlugin"));
    if (!createFunc) {
        LA_LOG_ERROR("Plugin missing 'createPlugin': " + filepath);
        freeLibrary(handle);
        return false;
    }

    // 3. 获取销毁函数 —— destroyPlugin
    auto destroyFunc = reinterpret_cast<DestroyPluginFunc>(resolveSymbol(handle, "destroyPlugin"));
    if (!destroyFunc) {
        LA_LOG_ERROR("Plugin missing 'destroyPlugin': " + filepath);
        freeLibrary(handle);
        return false;
    }

    // 4. 创建插件实例
    IPlugin* plugin = createFunc();
    if (!plugin) {
        LA_LOG_ERROR("createPlugin returned null: " + filepath);
        freeLibrary(handle);
        return false;
    }

    // 5. 初始化插件
    if (!plugin->initialize()) {
        LA_LOG_ERROR("Plugin init failed: " + filepath);
        destroyFunc(plugin);
        freeLibrary(handle);
        return false;
    }

    // 6. 记录插件信息
    PluginInfo info;
    info.name = plugin->name();
    info.version = plugin->version();
    info.filepath = filepath;
    info.instance = plugin;
    m_plugins.push_back(info);

    m_loaded.push_back({handle, plugin, destroyFunc});

    LA_LOG_INFO("Plugin loaded: " + info.name + " v" + info.version);
    return true;
}

size_t PluginManager::loadPluginsFrom(const std::string& directory) {
    size_t count = 0;
    // 简化版：扫描目录中的 .dll/.so 文件
    // 生产项目应使用 std::filesystem (C++17)
#ifdef _WIN32
    std::string searchPath = directory + "\\*" + extension();
    WIN32_FIND_DATAW findData;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, searchPath.c_str(), -1, nullptr, 0);
    std::wstring wsearch(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, searchPath.c_str(), -1, &wsearch[0], wlen);

    HANDLE hFind = FindFirstFileW(wsearch.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            int len = WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, -1, nullptr, 0, nullptr, nullptr);
            std::string filename(len, 0);
            WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, -1, &filename[0], len, nullptr, nullptr);

            std::string fullPath = directory + "\\" + filename;
            if (loadPlugin(fullPath)) ++count;
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }
#endif
    return count;
}

void PluginManager::unloadAll() {
    // 逆序卸载（后加载先卸载）
    for (auto it = m_loaded.rbegin(); it != m_loaded.rend(); ++it) {
        if (it->instance) {
            it->instance->shutdown();
        }
        if (it->destroyFunc && it->instance) {
            it->destroyFunc(it->instance);
        }
        if (it->handle) {
            freeLibrary(it->handle);
        }
    }
    m_loaded.clear();
    m_plugins.clear();
    LA_LOG_INFO("All plugins unloaded");
}

IPlugin* PluginManager::findByName(const std::string& name) {
    // STL: find_if 算法
    auto it = std::find_if(m_plugins.begin(), m_plugins.end(),
        [&name](const PluginInfo& info) { return info.name == name; });
    return (it != m_plugins.end()) ? it->instance : nullptr;
}

void PluginManager::forEach(const std::function<void(IPlugin*)>& func) {
    for (auto& info : m_plugins) {
        if (info.instance) {
            func(info.instance);
        }
    }
}

} // namespace la
