/*
 * json_demo.cpp —— JSON 数据交换示例（GCC 6 兼容版）
 *
 * 涵盖: JSON解析/生成/文件读写
 *
 * 编译: g++ -std=c++14 json_demo.cpp -o json_demo
 *
 * 生产项目请使用 nlohmann/json: https://github.com/nlohmann/json
 */
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cctype>

// ==================== 简易JSON值（兼容旧编译器） ====================
enum class JType { Null, Bool, Int, Double, String, Array, Object };

class JsonVal {
public:
    JType type = JType::Null;

    // 基本类型存储
    bool   boolVal = false;
    long long intVal = 0;
    double doubleVal = 0.0;
    std::string strVal;
    std::vector<JsonVal> arrVal;
    std::map<std::string, JsonVal> objVal;

    // 构造
    JsonVal() {}
    JsonVal(bool v) : type(JType::Bool), boolVal(v) {}
    JsonVal(int v) : type(JType::Int), intVal(v) {}
    JsonVal(long long v) : type(JType::Int), intVal(v) {}
    JsonVal(double v) : type(JType::Double), doubleVal(v) {}
    JsonVal(const std::string& v) : type(JType::String), strVal(v) {}
    JsonVal(const char* v) : type(JType::String), strVal(v) {}

    // 数组/对象操作
    JsonVal& operator[](size_t i) {
        if (type != JType::Array) { type = JType::Array; arrVal.clear(); }
        if (i >= arrVal.size()) arrVal.resize(i + 1);
        return arrVal[i];
    }
    JsonVal& operator[](const std::string& key) {
        if (type != JType::Object) { type = JType::Object; objVal.clear(); }
        return objVal[key];
    }
    void push_back(const JsonVal& v) {
        if (type != JType::Array) { type = JType::Array; arrVal.clear(); }
        arrVal.push_back(v);
    }

    // 类型检查
    bool isNull() const { return type == JType::Null; }
    bool isString() const { return type == JType::String; }
    bool isArray() const { return type == JType::Array; }
    bool isObject() const { return type == JType::Object; }

    // 取值
    const std::string& getString() const { return strVal; }
    long long getInt() const { return intVal; }
    double getDouble() const { return doubleVal; }
    bool getBool() const { return boolVal; }
    const std::vector<JsonVal>& getArray() const { return arrVal; }
    const std::map<std::string, JsonVal>& getObject() const { return objVal; }

    // 序列化
    std::string dump(int indent = 0) const {
        std::ostringstream oss;
        dumpInternal(oss, indent, 0);
        return oss.str();
    }

    // 解析
    static JsonVal parse(const std::string& json);

private:
    void dumpInternal(std::ostringstream& oss, int indent, int level) const;
};

void JsonVal::dumpInternal(std::ostringstream& oss, int indent, int level) const {
    auto writeIndent = [&](int lv) {
        for (int j = 0; j < lv * indent; ++j) oss << ' ';
    };
    switch (type) {
    case JType::Null:   oss << "null"; break;
    case JType::Bool:   oss << (boolVal ? "true" : "false"); break;
    case JType::Int:    oss << intVal; break;
    case JType::Double: oss << doubleVal; break;
    case JType::String: oss << '"' << strVal << '"'; break;
    case JType::Array:
        oss << '['; if (indent && !arrVal.empty()) oss << '\n';
        for (size_t j = 0; j < arrVal.size(); ++j) {
            if (indent) writeIndent(level + 1);
            arrVal[j].dumpInternal(oss, indent, level + 1);
            if (j + 1 < arrVal.size()) oss << ',';
            if (indent) oss << '\n';
        }
        if (indent && !arrVal.empty()) writeIndent(level);
        oss << ']'; break;
    case JType::Object:
        oss << '{'; if (indent && !objVal.empty()) oss << '\n';
        size_t cnt = 0;
        for (const auto& kv : objVal) {
            if (indent) writeIndent(level + 1);
            oss << '"' << kv.first << '"' << ':';
            if (indent) oss << ' ';
            kv.second.dumpInternal(oss, indent, level + 1);
            if (++cnt < objVal.size()) oss << ',';
            if (indent) oss << '\n';
        }
        if (indent && !objVal.empty()) writeIndent(level);
        oss << '}'; break;
    }
}

// 递归下降解析器
struct Parser {
    const std::string& s;
    size_t i = 0;
    Parser(const std::string& src) : s(src) {}
    void skipWS() { while (i < s.size() && std::isspace(s[i])) ++i; }
    char peek() { skipWS(); return i < s.size() ? s[i] : 0; }
    char next() { skipWS(); return i < s.size() ? s[i++] : 0; }

    JsonVal parseVal() {
        char c = peek();
        if (c == '{') return parseObj();
        if (c == '[') return parseArr();
        if (c == '"') return JsonVal(parseStr());
        if (s.substr(i,4)=="true")  { i+=4; return JsonVal(true); }
        if (s.substr(i,5)=="false") { i+=5; return JsonVal(false); }
        if (s.substr(i,4)=="null")  { i+=4; return JsonVal(); }
        return JsonVal(parseNum());
    }

    JsonVal parseObj() {
        JsonVal v; v.type = JType::Object; next(); // skip '{'
        if (peek() == '}') { next(); return v; }
        while (true) {
            std::string key = parseStr(); next(); // skip ':'
            v.objVal[key] = parseVal();
            if (next() == '}') break;
        }
        return v;
    }

    JsonVal parseArr() {
        JsonVal v; v.type = JType::Array; next(); // skip '['
        if (peek() == ']') { next(); return v; }
        while (true) {
            v.arrVal.push_back(parseVal());
            if (next() == ']') break;
        }
        return v;
    }

    std::string parseStr() {
        next(); std::string r;
        while (i < s.size()) {
            if (s[i] == '"') { ++i; return r; }
            if (s[i] == '\\') { ++i;
                switch(s[i++]) {
                    case '"':r+='"';break;case '\\':r+='\\';break;
                    case 'n':r+='\n';break;case 't':r+='\t';break;
                    case 'r':r+='\r';break;default:r+=s[i-1];break;
                }
            } else r += s[i++];
        }
        return r;
    }

    double parseNum() {
        size_t start = i;
        if (s[i]=='-') ++i;
        while (i<s.size() && std::isdigit(s[i])) ++i;
        if (i<s.size() && s[i]=='.') { ++i; while (i<s.size()&&std::isdigit(s[i])) ++i; }
        return std::stod(s.substr(start, i-start));
    }
};

JsonVal JsonVal::parse(const std::string& json) {
    return Parser(json).parseVal();
}

// ==================== 演示 ====================

// 业务数据结构
struct User {
    std::string name;
    int age = 0;
    std::vector<std::string> skills;

    JsonVal toJson() const {
        JsonVal v; v.type = JType::Object;
        v["name"] = JsonVal(name);
        v["age"] = JsonVal((long long)age);
        JsonVal arr; arr.type = JType::Array;
        for (const auto& s : skills) arr.push_back(JsonVal(s));
        v["skills"] = arr;
        return v;
    }

    static User fromJson(const JsonVal& j) {
        User u;
        u.name = j.objVal.at("name").strVal;
        u.age = (int)j.objVal.at("age").intVal;
        for (const auto& s : j.objVal.at("skills").arrVal) {
            u.skills.push_back(s.strVal);
        }
        return u;
    }
};

int main() {
    std::cout << "=== JSON 数据交换演示 ===\n\n";

    // 1. C++对象 → JSON
    User alice{"Alice", 25, {"C++", "Python", "Rust"}};
    JsonVal jsonUser = alice.toJson();
    std::cout << "--- C++对象 → JSON ---\n";
    std::cout << jsonUser.dump(2) << "\n\n";

    // 2. 序列化为紧凑格式
    std::string jsonStr = jsonUser.dump();
    std::cout << "--- 紧凑格式 ---\n" << jsonStr << "\n\n";

    // 3. 解析 JSON
    std::cout << "--- 解析 JSON → C++对象 ---\n";
    JsonVal parsed = JsonVal::parse(jsonStr);
    User alice2 = User::fromJson(parsed);
    std::cout << "name=" << alice2.name << " age=" << alice2.age << "\n";
    std::cout << "skills: ";
    for (const auto& s : alice2.skills) std::cout << s << " ";
    std::cout << "\n\n";

    // 4. 复杂 JSON 解析
    std::string cfgJson = R"({
        "config": {"host":"localhost","port":8080,"ssl":true},
        "endpoints": ["/api/v1","/api/v2"],
        "timeout": 30
    })";
    JsonVal cfg = JsonVal::parse(cfgJson);
    std::cout << "--- 配置解析 ---\n";
    std::cout << "host: " << cfg["config"]["host"].strVal << "\n";
    std::cout << "port: " << cfg["config"]["port"].intVal << "\n";
    std::cout << "ssl: " << (cfg["config"]["ssl"].boolVal?"on":"off") << "\n";
    std::cout << "endpoints[0]: " << cfg["endpoints"][0].strVal << "\n";

    // 5. 文件操作
    std::ofstream out("demo_output.json");
    out << jsonUser.dump(2);
    out.close();
    std::cout << "\n已写入 demo_output.json\n";

    std::cout << "=== JSON Demo Complete ===\n";
    return 0;
}
