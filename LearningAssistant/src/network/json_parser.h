/*
 * json_parser.h —— JSON 解析与生成
 *
 * 知识点：
 *   1. nlohmann/json —— 现代C++ JSON库
 *   2. 序列化：C++对象 → JSON字符串
 *   3. 反序列化：JSON字符串 → C++对象
 *   4. JSON文件读写
 *   5. 条件编译：有 nlohmann/json 时使用它，否则使用简易解析
 */
#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <optional>
#include <sstream>
#include <fstream>
#include <stdexcept>

namespace la {

// ============================================================
// 简易 JSON 值类型（教学用，生产项目请使用 nlohmann/json）
// ============================================================

/// 支持的类型
enum class JsonType { Null, Bool, Int, Double, String, Array, Object };

// 前向声明
class JsonValue;
using JsonArray = std::vector<JsonValue>;
using JsonObject = std::map<std::string, JsonValue>;

/// @brief JSON 值类型（类似 nlohmann/json 的简化版）
class JsonValue {
public:
    JsonValue() : m_type(JsonType::Null) {}
    JsonValue(std::nullptr_t) : m_type(JsonType::Null) {}
    JsonValue(bool v) : m_type(JsonType::Bool), m_data(v) {}
    JsonValue(int v) : m_type(JsonType::Int), m_data(static_cast<int64_t>(v)) {}
    JsonValue(int64_t v) : m_type(JsonType::Int), m_data(v) {}
    JsonValue(double v) : m_type(JsonType::Double), m_data(v) {}
    JsonValue(const char* v) : m_type(JsonType::String), m_data(std::string(v)) {}
    JsonValue(const std::string& v) : m_type(JsonType::String), m_data(v) {}
    JsonValue(JsonArray v) : m_type(JsonType::Array), m_data(std::move(v)) {}
    JsonValue(JsonObject v) : m_type(JsonType::Object), m_data(std::move(v)) {}

    // ---- 类型判断 ----
    bool isNull()    const { return m_type == JsonType::Null; }
    bool isBool()    const { return m_type == JsonType::Bool; }
    bool isInt()     const { return m_type == JsonType::Int; }
    bool isDouble()  const { return m_type == JsonType::Double; }
    bool isString()  const { return m_type == JsonType::String; }
    bool isArray()   const { return m_type == JsonType::Array; }
    bool isObject()  const { return m_type == JsonType::Object; }
    bool isNumber()  const { return isInt() || isDouble(); }

    // ---- 取值（带类型检查） ----
    bool getBool() const { return std::get<bool>(m_data); }
    int64_t getInt() const { return std::get<int64_t>(m_data); }
    double getDouble() const { return std::get<double>(m_data); }
    const std::string& getString() const { return std::get<std::string>(m_data); }
    const JsonArray& getArray() const { return std::get<JsonArray>(m_data); }
    const JsonObject& getObject() const { return std::get<JsonObject>(m_data); }

    // ---- 数组操作 ----
    JsonValue& operator[](size_t index);
    const JsonValue& operator[](size_t index) const;
    void push_back(const JsonValue& val);

    // ---- 对象操作 ----
    JsonValue& operator[](const std::string& key);
    bool contains(const std::string& key) const;
    std::optional<JsonValue> get(const std::string& key) const;

    // ---- 序列化 ----
    std::string dump(int indent = 0) const;
    static JsonValue parse(const std::string& json);

private:
    JsonType m_type = JsonType::Null;
    std::variant<std::nullptr_t, bool, int64_t, double, std::string, JsonArray, JsonObject> m_data;

    void dumpInternal(std::ostringstream& oss, int indent, int level) const;

    // 简易 JSON 解析器
    struct Parser {
        const std::string& src;
        size_t pos = 0;
        explicit Parser(const std::string& s) : src(s) {}
        JsonValue parseValue();
        JsonObject parseObject();
        JsonArray  parseArray();
        std::string parseString();
        double     parseNumber();
        void skipWhitespace();
        char peek();
        char next();
    };
};

} // namespace la
