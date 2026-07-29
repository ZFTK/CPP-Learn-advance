#include "core/logger.h"
#include <iostream>
#include <iomanip>

namespace la {

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::setOutputFile(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file.is_open()) m_file.close();
    m_file.open(filepath, std::ios::app);
}

void Logger::setMinLevel(LogLevel level) {
    m_minLevel = level;
}

void Logger::log(LogLevel level, const std::string& message) {
    // 级别过滤
    if (level < m_minLevel) return;

    // std::lock_guard: RAII 锁，构造时加锁，析构时解锁
    // 作用域结束时自动释放，即使发生异常也能保证解锁
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string formatted = "[" + currentTime() + "] [" + levelToString(level) + "] " + message;

    if (m_file.is_open()) {
        m_file << formatted << std::endl;
    } else {
        std::cout << formatted << std::endl;
    }
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
    }
    return "???";
}

std::string Logger::currentTime() const {
    // std::chrono: 获取当前时间并格式化
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S")
        << "." << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

} // namespace la
