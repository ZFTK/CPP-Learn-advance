/*
 * logger.h —— 简易日志系统
 *
 * 知识点：
 *   1. 变参模板 (Variadic Templates)
 *   2. std::ofstream 文件流
 *   3. std::mutex 线程安全
 *   4. enum class 强类型枚举
 *   5. std::chrono 时间处理
 *   6. std::source_location (C++20) 或用宏替代
 */
#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iostream>

namespace la {

/// 日志级别
enum class LogLevel { Debug, Info, Warning, Error };

/// @brief 线程安全的日志系统
class Logger {
public:
    static Logger& instance();

    /// 设置日志输出文件（不设置则输出到控制台）
    void setOutputFile(const std::string& filepath);
    /// 设置最低日志级别
    void setMinLevel(LogLevel level);

    // ---------- 日志宏风格接口 ----------
    void log(LogLevel level, const std::string& message);

private:
    Logger() = default;
    std::string levelToString(LogLevel level) const;
    std::string currentTime() const;

    LogLevel m_minLevel = LogLevel::Debug;
    std::ofstream m_file;
    std::mutex m_mutex;  // 多线程安全
};

// ---------- 便捷宏 ----------
#define LA_LOG_DEBUG(msg)   la::Logger::instance().log(la::LogLevel::Debug,   msg)
#define LA_LOG_INFO(msg)    la::Logger::instance().log(la::LogLevel::Info,    msg)
#define LA_LOG_WARNING(msg) la::Logger::instance().log(la::LogLevel::Warning, msg)
#define LA_LOG_ERROR(msg)   la::Logger::instance().log(la::LogLevel::Error,   msg)

/// @brief 模板函数：将任意类型转为字符串（用于日志格式化）
template<typename... Args>
std::string formatString(const std::string& fmt, Args&&... args) {
    // 简化版：用 ostringstream 拼接
    std::ostringstream oss;
    ((oss << std::forward<Args>(args) << " "), ...);  // C++17 折叠表达式
    return fmt + ": " + oss.str();
}

} // namespace la
