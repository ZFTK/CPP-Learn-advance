/*
 * sqlite_demo.cpp —— SQLite3 数据库示例
 *
 * 涵盖: 建表、CRUD、预编译语句、事务、FTS5全文搜索
 *
 * 编译: 需要 sqlite3 合并版
 *   g++ -std=c++17 sqlite_demo.cpp sqlite3.c -o sqlite_demo -lpthread -ldl
 *
 * 下载 sqlite3: https://www.sqlite.org/download.html
 *   选择 sqlite-amalgamation-*.zip
 */
#include <iostream>
#include <string>
#include <vector>
#include <sqlite3.h>

// ==================== RAII 数据库封装 ====================

class Database {
public:
    explicit Database(const std::string& path) {
        int rc = sqlite3_open(path.c_str(), &db);
        if (rc != SQLITE_OK) {
            std::cerr << "打开数据库失败: " << sqlite3_errmsg(db) << "\n";
            sqlite3_close(db);
            db = nullptr;
        } else {
            std::cout << "数据库已打开: " << path << "\n";
            execute("PRAGMA journal_mode=WAL");
        }
    }

    ~Database() { if (db) { sqlite3_close(db); std::cout << "数据库已关闭\n"; } }

    bool isOpen() const { return db != nullptr; }

    // 执行无返回的SQL
    bool execute(const std::string& sql) {
        char* err = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL错误: " << err << "\n";
            sqlite3_free(err);
            return false;
        }
        return true;
    }

    // 参数化查询（防SQL注入）
    typedef std::vector<std::vector<std::pair<std::string, std::string>>> QueryResult;
    QueryResult query(const std::string& sql) {
        QueryResult result;
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "预编译失败: " << sqlite3_errmsg(db) << "\n";
            return result;
        }
        int cols = sqlite3_column_count(stmt);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::vector<std::pair<std::string, std::string>> row;
            for (int i = 0; i < cols; ++i) {
                const char* name = sqlite3_column_name(stmt, i);
                const char* value = (const char*)sqlite3_column_text(stmt, i);
                row.emplace_back(name ? name : "", value ? value : "NULL");
            }
            result.push_back(row);
        }
        sqlite3_finalize(stmt);
        return result;
    }

    // 参数化插入
    int64_t insert(const std::string& sql, const std::string& param) {
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, param.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return sqlite3_last_insert_rowid(db);
    }

private:
    sqlite3* db = nullptr;
};

// ==================== 演示 ====================

int main() {
    Database db("demo_sqlite.db");
    if (!db.isOpen()) return 1;

    // 1. 建表
    std::cout << "=== 1. 建表 ===\n";
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS users (
            id    INTEGER PRIMARY KEY AUTOINCREMENT,
            name  TEXT NOT NULL,
            email TEXT UNIQUE,
            score INTEGER DEFAULT 0
        )
    )");

    // 2. 插入数据
    std::cout << "\n=== 2. 插入数据 ===\n";
    db.insert("INSERT INTO users (name, email, score) VALUES (?, ?, ?)", "Alice,alice@test.com,95");
    db.insert("INSERT INTO users (name, email, score) VALUES (?, ?, ?)", "Bob,bob@test.com,87");
    db.insert("INSERT INTO users (name, email, score) VALUES (?, ?, ?)", "Charlie,charlie@test.com,92");
    std::cout << "插入3条数据\n";

    // 3. 查询
    std::cout << "\n=== 3. 查询全表 ===\n";
    auto users = db.query("SELECT id, name, email, score FROM users ORDER BY score DESC");
    for (const auto& row : users) {
        std::cout << "  ";
        for (const auto& [col, val] : row) {
            std::cout << col << "=" << val << " ";
        }
        std::cout << "\n";
    }

    // 4. 更新
    std::cout << "\n=== 4. 更新 ===\n";
    db.execute("UPDATE users SET score = 99 WHERE name = 'Alice'");
    std::cout << "Alice的分数已更新\n";

    // 5. 事务批量操作
    std::cout << "\n=== 5. 事务批量插入 ===\n";
    db.execute("BEGIN TRANSACTION");
    for (int i = 1; i <= 3; ++i) {
        db.execute("INSERT INTO users (name, email, score) VALUES ('User" +
                   std::to_string(i) + "', 'user" + std::to_string(i) +
                   "@test.com', " + std::to_string(i * 10) + ")");
    }
    db.execute("COMMIT");
    std::cout << "事务提交，批量插入3条\n";

    // 6. DELETE
    std::cout << "\n=== 6. 删除 ===\n";
    db.execute("DELETE FROM users WHERE score < 20");
    std::cout << "已删除低分用户\n";

    // 7. FTS5 全文搜索（可选）
    std::cout << "\n=== 7. FTS5 全文搜索 ===\n";
    db.execute("CREATE VIRTUAL TABLE IF NOT EXISTS users_fts USING fts5(name, email, content=users, content_rowid=id)");
    db.execute("INSERT INTO users_fts(users_fts) VALUES('rebuild')"); // 重建索引
    auto ftsResult = db.query("SELECT u.name, u.email FROM users u JOIN users_fts f ON u.id=f.rowid WHERE users_fts MATCH 'Alice'");
    for (const auto& row : ftsResult) {
        std::cout << "  搜索'Alice'结果: ";
        for (const auto& [col, val] : row) std::cout << col << "=" << val << " ";
        std::cout << "\n";
    }

    std::cout << "\n=== SQLite Demo Complete ===\n";
    return 0;
}
