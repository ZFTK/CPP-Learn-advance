/*
 * sqlite_wrapper.h —— SQLite3 C++ 封装
 *
 * 知识点：
 *   1. SQLite3 C API 封装为 RAII 风格的 C++ 类
 *   2. std::unique_ptr 管理资源生命周期
 *   3. Prepared Statement（预编译语句）防 SQL 注入
 *   4. 事务 (Transaction) RAII 封装
 *   5. std::vector 存储查询结果
 */
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

#include <sqlite3.h>
// SQLite3 类型



namespace la {

/// @brief 数据库单行记录：列名 -> 值
using Row = std::vector<std::pair<std::string, std::string>>;

/// @brief SQLite3 数据库封装（RAII）
/// 构造时打开数据库，析构时自动关闭
class SqliteDB {
public:
    explicit SqliteDB(const std::string& dbPath);
    ~SqliteDB();

    // 禁止拷贝，允许移动
    SqliteDB(const SqliteDB&) = delete;
    SqliteDB& operator=(const SqliteDB&) = delete;
    SqliteDB(SqliteDB&& other) noexcept;
    SqliteDB& operator=(SqliteDB&& other) noexcept;

    bool isOpen() const;

    // ---- 执行 SQL ----
    /// 执行无返回的 SQL（CREATE/INSERT/UPDATE/DELETE）
    bool execute(const std::string& sql);

    /// 执行参数化 SQL（无返回值）
    template<typename... Args>
    bool execute(const std::string& sql, Args&&... args) {
        sqlite3_stmt* stmt = prepare(sql);
        if (!stmt) return false;
        bindParams(stmt, 1, std::forward<Args>(args)...);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return rc == SQLITE_DONE || rc == SQLITE_ROW;
    }

    /// 执行查询并返回所有行
    /// 参数化查询示例：query("SELECT * FROM t WHERE id=?", 42)
    template<typename... Args>
    std::vector<Row> query(const std::string& sql, Args&&... args) {
        sqlite3_stmt* stmt = prepare(sql);
        if (!stmt) return {};
        bindParams(stmt, 1, std::forward<Args>(args)...);
        return fetchAll(stmt);
    }

    // ---- 事务 ----
    void beginTransaction();
    void commit();
    void rollback();

    // ---- 工具 ----
    int64_t lastInsertRowId();

private:
    sqlite3_stmt* prepare(const std::string& sql);
    std::vector<Row> fetchAll(sqlite3_stmt* stmt);

    // 递归终止
    void bindParams(sqlite3_stmt*, int) {}
    // 递归绑定参数
    template<typename T, typename... Rest>
    void bindParams(sqlite3_stmt* stmt, int index, T&& first, Rest&&... rest);

    sqlite3* m_db = nullptr;
};

/// @brief RAII 事务封装
/// 构造时 BEGIN，析构时 COMMIT（若无异常）或 ROLLBACK（若有异常）
class Transaction {
public:
    explicit Transaction(SqliteDB& db);
    ~Transaction();

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    void commit();  // 手动提交

private:
    SqliteDB& m_db;
    bool m_committed = false;
};

// ---- 模板实现（必须在头文件中） ----
#include <stdexcept>

template<typename T, typename... Rest>
void SqliteDB::bindParams(sqlite3_stmt* stmt, int index, T&& first, Rest&&... rest) {
    // sqlite3_bind_* 系列函数：将 C++ 值绑定到 SQL 参数
    if constexpr (std::is_same_v<std::decay_t<T>, int>) {
        sqlite3_bind_int(stmt, index, first);
    } else if constexpr (std::is_same_v<std::decay_t<T>, int64_t>
        || std::is_same_v<std::decay_t<T>, long long>) {
        sqlite3_bind_int64(stmt, index, first);
    } else if constexpr (std::is_same_v<std::decay_t<T>, double>) {
        sqlite3_bind_double(stmt, index, first);
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        sqlite3_bind_text(stmt, index, first.c_str(), -1, SQLITE_TRANSIENT);
    } else if constexpr (std::is_same_v<std::decay_t<T>, const char*>
        || std::is_same_v<std::decay_t<T>, char*>) {
        sqlite3_bind_text(stmt, index, first, -1, SQLITE_TRANSIENT);
    } else {
        throw std::runtime_error("Unsupported bind type");
    }
    // 递归展开参数包
    bindParams(stmt, index + 1, std::forward<Rest>(rest)...);
}

} // namespace la
