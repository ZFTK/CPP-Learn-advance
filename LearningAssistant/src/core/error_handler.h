/*
 * error_handler.h —— 统一错误处理
 *
 * 知识点：
 *   1. std::exception 继承体系
 *   2. std::optional 表示"可能失败"的返回值
 *   3. std::variant 统一返回类型
 *   4. std::function 回调式错误处理
 */
#pragma once

#include <string>
#include <optional>
#include <variant>
#include <functional>
#include <stdexcept>

namespace la {

/// 自定义异常基类
class LaException : public std::runtime_error {
public:
    explicit LaException(const std::string& msg) : std::runtime_error(msg) {}
};

class DatabaseException : public LaException { using LaException::LaException; };
class NetworkException  : public LaException { using LaException::LaException; };
class PluginException   : public LaException { using LaException::LaException; };
class ConfigException   : public LaException { using LaException::LaException; };

/// @brief Result<T> —— 类似 Rust 的 Result 类型
/// 要么包含成功的值 T，要么包含错误信息
template<typename T>
class Result {
public:
    // 成功构造
    static Result<T> ok(T value) {
        Result<T> r;
        r.m_data = std::move(value);
        return r;
    }
    // 失败构造
    static Result<T> err(std::string error) {
        Result<T> r;
        r.m_error = std::move(error);
        return r;
    }

    bool isOk() const { return m_data.has_value(); }
    bool isErr() const { return !isOk(); }

    // 获取值（调用者应先检查 isOk()）
    T& value() { return m_data.value(); }
    const T& value() const { return m_data.value(); }

    // 获取错误信息
    const std::string& error() const { return m_error; }

    // 链式处理：如果成功则执行函数
    template<typename F>
    auto map(F&& func) -> Result<decltype(func(std::declval<T>()))> {
        using U = decltype(func(std::declval<T>()));
        if (isOk()) return Result<U>::ok(func(value()));
        return Result<U>::err(error());
    }

private:
    std::optional<T> m_data;
    std::string m_error;
};

// 无返回值的 Result 特化
template<>
class Result<void> {
public:
    static Result<void> ok() { return {}; }
    static Result<void> err(std::string error) {
        Result<void> r;
        r.m_error = std::move(error);
        return r;
    }
    bool isOk() const { return m_error.empty(); }
    bool isErr() const { return !isOk(); }
    const std::string& error() const { return m_error; }
private:
    std::string m_error;
};

} // namespace la
