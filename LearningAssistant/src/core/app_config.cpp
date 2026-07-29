/*
 * app_config.cpp —— 应用配置管理实现
 *
 * 知识点：
 *   1. std::map 的 insert/find/迭代
 *   2. 文件流读写 (std::ifstream / std::ofstream)
 *   3. 简单的 JSON 手动解析（无外部依赖时）
 *   4. 字符串处理 (std::stoi, std::stod)
 */
#include "core/app_config.h"
#include "core/logger.h"
#include <fstream>
#include <algorithm>

namespace la {

AppConfig& AppConfig::instance() {
    // C++11保证局部静态变量的线程安全初始化
    static AppConfig inst;
    return inst;
}

// ---- 手动 JSON 解析（不依赖 nlohmann/json 时的简易实现） ----
// 生产项目请使用 nlohmann/json，详见 src/network/json_parser.cpp

bool AppConfig::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LA_LOG_WARNING("Cannot open config file: " + filepath);
        return false;
    }

    m_data.clear();
    std::string line;
    while (std::getline(file, line)) {
        // 跳过注释和空行
        // STL: std::string::find_first_not_of 查找第一个非空白字符
        auto pos = line.find_first_not_of(" \t\r\n");
        if (pos == std::string::npos || line[pos] == '#' || line[pos] == '/') continue;

        // 查找 key = value 格式
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(pos, eq - pos);
        std::string value = line.substr(eq + 1);

        // STL: 去除首尾空白
        auto trim = [](std::string& s) {
            // std::find_if_not: 查找第一个不满足条件的元素
            s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), ::isspace));
            s.erase(std::find_if_not(s.rbegin(), s.rend(), ::isspace).base(), s.end());
        };
        trim(key);
        trim(value);

        // STL: map::emplace —— 原地构造，避免临时对象
        m_data.emplace(std::move(key), std::move(value));
    }

    LA_LOG_INFO("Loaded " + std::to_string(m_data.size()) + " config entries");
    return true;
}

bool AppConfig::saveToFile(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    // STL: range-based for + structured binding (C++17) 遍历 map
    for (const auto& [key, value] : m_data) {
        file << key << " = " << value << "\n";
    }
    return true;
}

std::string AppConfig::getString(const std::string& key, const std::string& defaultValue) const {
    // STL: map::find —— O(log n) 查找
    auto it = m_data.find(key);
    return (it != m_data.end()) ? it->second : defaultValue;
}

int AppConfig::getInt(const std::string& key, int defaultValue) const {
    auto it = m_data.find(key);
    if (it == m_data.end()) return defaultValue;
    try { return std::stoi(it->second); }
    catch (...) { return defaultValue; }
}

bool AppConfig::getBool(const std::string& key, bool defaultValue) const {
    auto it = m_data.find(key);
    if (it == m_data.end()) return defaultValue;
    std::string lower = it->second;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return (lower == "true" || lower == "1" || lower == "yes");
}

double AppConfig::getDouble(const std::string& key, double defaultValue) const {
    auto it = m_data.find(key);
    if (it == m_data.end()) return defaultValue;
    try { return std::stod(it->second); }
    catch (...) { return defaultValue; }
}

void AppConfig::setString(const std::string& key, const std::string& value) {
    // STL: map::insert_or_assign (C++17)
    // 若 key 不存在则插入，存在则更新值
    m_data.insert_or_assign(key, value);
}

void AppConfig::setInt(const std::string& key, int value) {
    m_data.insert_or_assign(key, std::to_string(value));
}

void AppConfig::setBool(const std::string& key, bool value) {
    m_data.insert_or_assign(key, value ? "true" : "false");
}

bool AppConfig::hasKey(const std::string& key) const {
    return m_data.find(key) != m_data.end();
}

} // namespace la
