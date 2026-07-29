/*
 * sqlite_wrapper.cpp —— SQLite3 封装实现
 *
 * 核心知识点：
 *   1. sqlite3_open / sqlite3_close    —— 打开/关闭数据库
 *   2. sqlite3_exec                    —— 执行无返回值的SQL
 *   3. sqlite3_prepare_v2 / sqlite3_step —— 预编译+执行查询
 *   4. sqlite3_bind_*                  —— 参数绑定（防注入）
 *   5. sqlite3_column_*               —— 读取结果列
 */
#include "database/sqlite_wrapper.h"
#include "core/logger.h"
#include <sqlite3.h>

namespace la {

// ==================== SqliteDB ====================

SqliteDB::SqliteDB(const std::string& dbPath) {
    // sqlite3_open: 打开数据库连接，不存在则创建
    int rc = sqlite3_open(dbPath.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        LA_LOG_ERROR("Failed to open database: " + std::string(sqlite3_errmsg(m_db)));
        sqlite3_close(m_db);
        m_db = nullptr;
    } else {
        // 启用 WAL 模式提升并发性能
        execute("PRAGMA journal_mode=WAL");
        // 启用外键约束
        execute("PRAGMA foreign_keys=ON");
        LA_LOG_INFO("Database opened: " + dbPath);
    }
}

SqliteDB::~SqliteDB() {
    if (m_db) {
        sqlite3_close(m_db);
        LA_LOG_INFO("Database closed");
    }
}

SqliteDB::SqliteDB(SqliteDB&& other) noexcept : m_db(other.m_db) {
    other.m_db = nullptr;
}

SqliteDB& SqliteDB::operator=(SqliteDB&& other) noexcept {
    if (this != &other) {
        if (m_db) sqlite3_close(m_db);
        m_db = other.m_db;
        other.m_db = nullptr;
    }
    return *this;
}

bool SqliteDB::isOpen() const {
    return m_db != nullptr;
}

bool SqliteDB::execute(const std::string& sql) {
    char* errMsg = nullptr;
    // sqlite3_exec: 执行一条或多条 SQL，适合 CREATE/INSERT/UPDATE/DELETE
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        LA_LOG_ERROR("SQL error: " + std::string(errMsg));
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

sqlite3_stmt* SqliteDB::prepare(const std::string& sql) {
    sqlite3_stmt* stmt = nullptr;
    // sqlite3_prepare_v2: 预编译 SQL，支持参数占位符 ?
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        LA_LOG_ERROR("Prepare failed: " + std::string(sqlite3_errmsg(m_db)));
        return nullptr;
    }
    return stmt;
}

std::vector<Row> SqliteDB::fetchAll(sqlite3_stmt* stmt) {
    std::vector<Row> result;
    int colCount = sqlite3_column_count(stmt);

    // sqlite3_step: 逐行遍历结果集
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Row row;
        row.reserve(colCount);
        for (int i = 0; i < colCount; ++i) {
            // sqlite3_column_name: 列名
            const char* name = sqlite3_column_name(stmt, i);
            // sqlite3_column_text: 列值（以文本形式）
            const char* value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
            row.emplace_back(name ? name : "", value ? value : "NULL");
        }
        result.push_back(std::move(row));
    }

    // sqlite3_finalize: 释放 prepared statement
    sqlite3_finalize(stmt);
    return result;
}

void SqliteDB::beginTransaction() {
    execute("BEGIN TRANSACTION");
}

void SqliteDB::commit() {
    execute("COMMIT");
}

void SqliteDB::rollback() {
    execute("ROLLBACK");
}

int64_t SqliteDB::lastInsertRowId() {
    return sqlite3_last_insert_rowid(m_db);
}

// ==================== Transaction (RAII) ====================

Transaction::Transaction(SqliteDB& db) : m_db(db) {
    m_db.beginTransaction();
}

Transaction::~Transaction() {
    if (!m_committed) {
        m_db.rollback();  // 异常安全：未提交则回滚
    }
}

void Transaction::commit() {
    m_db.commit();
    m_committed = true;
}

} // namespace la
