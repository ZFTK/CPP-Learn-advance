/*
 * app_config.h —— 应用配置管理
 *
 * 知识点：
 *   1. STL容器 (std::map, std::string, std::optional)
 *   2. 单例模式 (Singleton)
 *   3. JSON序列化 (nlohmann/json)
 *   4. 文件I/O
 */
#pragma once

#include <string>
#include <map>
#include <optional>

namespace la {

/// @brief 应用配置管理器（单例）
/// 管理从 JSON 文件加载的键值对配置
class AppConfig {
public:
    // ---------- 单例接口 ----------
    static AppConfig& instance();

    // ---------- 加载/保存 ----------
    /// 从指定 JSON 文件加载配置
    bool loadFromFile(const std::string& filepath);
    /// 将当前配置保存到 JSON 文件
    bool saveToFile(const std::string& filepath) const;

    // ---------- 类型安全的取值 ----------
    std::string  getString(const std::string& key, const std::string& defaultValue = "") const;
    int          getInt(const std::string& key, int defaultValue = 0) const;
    bool         getBool(const std::string& key, bool defaultValue = false) const;
    double       getDouble(const std::string& key, double defaultValue = 0.0) const;

    // ---------- 设值 ----------
    void setString(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setBool(const std::string& key, bool value);

    // ---------- 查询 ----------
    bool hasKey(const std::string& key) const;

private:
    AppConfig() = default;
    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;

    // STL: std::map —— 红黑树实现，键有序
    std::map<std::string, std::string> m_data;
};

} // namespace la
