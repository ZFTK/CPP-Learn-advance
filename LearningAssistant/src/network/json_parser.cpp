/*
 * json_parser.cpp —— 简易 JSON 解析器实现
 *
 * 这是一个教学用的递归下降 JSON 解析器。
 * 生产项目请使用 nlohmann/json (https://github.com/nlohmann/json)
 * 用法: #include <nlohmann/json.hpp>  →  nlohmann::json j = nlohmann::json::parse(str);
 */
#include "network/json_parser.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace la {

// ==================== JsonValue 操作实现 ====================

JsonValue& JsonValue::operator[](size_t index) {
    return std::get<JsonArray>(m_data).at(index);
}

const JsonValue& JsonValue::operator[](size_t index) const {
    return std::get<JsonArray>(m_data).at(index);
}

void JsonValue::push_back(const JsonValue& val) {
    if (!isArray()) { m_type = JsonType::Array; m_data = JsonArray{}; }
    std::get<JsonArray>(m_data).push_back(val);
}

JsonValue& JsonValue::operator[](const std::string& key) {
    if (!isObject()) { m_type = JsonType::Object; m_data = JsonObject{}; }
    return std::get<JsonObject>(m_data)[key];
}

bool JsonValue::contains(const std::string& key) const {
    if (!isObject()) return false;
    return std::get<JsonObject>(m_data).find(key) != std::get<JsonObject>(m_data).end();
}

std::optional<JsonValue> JsonValue::get(const std::string& key) const {
    if (!isObject()) return std::nullopt;
    const auto& obj = std::get<JsonObject>(m_data);
    auto it = obj.find(key);
    if (it != obj.end()) return it->second;
    return std::nullopt;
}

// ==================== 序列化为 JSON 字符串 ====================

std::string JsonValue::dump(int indent) const {
    std::ostringstream oss;
    dumpInternal(oss, indent, 0);
    return oss.str();
}

void JsonValue::dumpInternal(std::ostringstream& oss, int indent, int level) const {
    auto writeIndent = [&](int lv) {
        for (int i = 0; i < lv * indent; ++i) oss << ' ';
    };

    switch (m_type) {
    case JsonType::Null:   oss << "null"; break;
    case JsonType::Bool:   oss << (std::get<bool>(m_data) ? "true" : "false"); break;
    case JsonType::Int:    oss << std::get<int64_t>(m_data); break;
    case JsonType::Double: oss << std::get<double>(m_data); break;
    case JsonType::String:
        oss << '"' << std::get<std::string>(m_data) << '"';
        break;
    case JsonType::Array: {
        const auto& arr = std::get<JsonArray>(m_data);
        oss << '[';
        if (indent > 0 && !arr.empty()) oss << '\n';
        for (size_t i = 0; i < arr.size(); ++i) {
            if (indent > 0) writeIndent(level + 1);
            arr[i].dumpInternal(oss, indent, level + 1);
            if (i + 1 < arr.size()) oss << ',';
            if (indent > 0) oss << '\n';
        }
        if (indent > 0 && !arr.empty()) writeIndent(level);
        oss << ']';
        break;
    }
    case JsonType::Object: {
        const auto& obj = std::get<JsonObject>(m_data);
        oss << '{';
        if (indent > 0 && !obj.empty()) oss << '\n';
        size_t count = 0;
        for (const auto& [key, val] : obj) {
            if (indent > 0) writeIndent(level + 1);
            oss << '"' << key << '"' << ':';
            if (indent > 0) oss << ' ';
            val.dumpInternal(oss, indent, level + 1);
            if (++count < obj.size()) oss << ',';
            if (indent > 0) oss << '\n';
        }
        if (indent > 0 && !obj.empty()) writeIndent(level);
        oss << '}';
        break;
    }
    }
}

// ==================== 递归下降 JSON 解析器 ====================

void JsonValue::Parser::skipWhitespace() {
    while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t' ||
           src[pos] == '\n' || src[pos] == '\r')) {
        ++pos;
    }
}

char JsonValue::Parser::peek() {
    skipWhitespace();
    return pos < src.size() ? src[pos] : '\0';
}

char JsonValue::Parser::next() {
    skipWhitespace();
    return pos < src.size() ? src[pos++] : '\0';
}

JsonValue JsonValue::Parser::parseValue() {
    char c = peek();
    switch (c) {
    case '{': return JsonValue(parseObject());
    case '[': return JsonValue(parseArray());
    case '"': return JsonValue(parseString());
    case 't': case 'f': {
        // true / false
        if (src.substr(pos, 4) == "true")  { pos += 4; return JsonValue(true); }
        if (src.substr(pos, 5) == "false") { pos += 5; return JsonValue(false); }
        throw std::runtime_error("Invalid boolean");
    }
    case 'n': {
        if (src.substr(pos, 4) == "null") { pos += 4; return JsonValue(); }
        throw std::runtime_error("Invalid null");
    }
    default:
        if (c == '-' || (c >= '0' && c <= '9')) {
            double num = parseNumber();
            // 判断是整数还是浮点
            if (num == static_cast<int64_t>(num)) return JsonValue(static_cast<int64_t>(num));
            return JsonValue(num);
        }
        throw std::runtime_error("Unexpected character: " + std::string(1, c));
    }
}

JsonObject JsonValue::Parser::parseObject() {
    JsonObject obj;
    next(); // 跳过 '{'
    if (peek() == '}') { next(); return obj; }

    while (true) {
        std::string key = parseString();
        char colon = next();
        if (colon != ':') throw std::runtime_error("Expected ':'");
        obj[key] = parseValue();

        char delim = next();
        if (delim == '}') break;
        if (delim != ',') throw std::runtime_error("Expected ',' or '}'");
    }
    return obj;
}

JsonArray JsonValue::Parser::parseArray() {
    JsonArray arr;
    next(); // 跳过 '['
    if (peek() == ']') { next(); return arr; }

    while (true) {
        arr.push_back(parseValue());
        char delim = next();
        if (delim == ']') break;
        if (delim != ',') throw std::runtime_error("Expected ',' or ']'");
    }
    return arr;
}

std::string JsonValue::Parser::parseString() {
    next(); // 跳过开头的 '"'
    std::string result;
    while (pos < src.size()) {
        char c = src[pos++];
        if (c == '"') return result;
        if (c == '\\') {
            if (pos >= src.size()) break;
            char escaped = src[pos++];
            switch (escaped) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case '/':  result += '/';  break;
                case 'b':  result += '\b'; break;
                case 'f':  result += '\f'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                default:   result += escaped; break;
            }
        } else {
            result += c;
        }
    }
    throw std::runtime_error("Unterminated string");
}

double JsonValue::Parser::parseNumber() {
    size_t start = pos;
    if (src[pos] == '-') ++pos;
    while (pos < src.size() && src[pos] >= '0' && src[pos] <= '9') ++pos;
    if (pos < src.size() && src[pos] == '.') {
        ++pos;
        while (pos < src.size() && src[pos] >= '0' && src[pos] <= '9') ++pos;
    }
    if (pos < src.size() && (src[pos] == 'e' || src[pos] == 'E')) {
        ++pos;
        if (pos < src.size() && (src[pos] == '+' || src[pos] == '-')) ++pos;
        while (pos < src.size() && src[pos] >= '0' && src[pos] <= '9') ++pos;
    }
    return std::stod(src.substr(start, pos - start));
}

JsonValue JsonValue::parse(const std::string& json) {
    Parser parser(json);
    return parser.parseValue();
}

} // namespace la
