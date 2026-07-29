# 06 — SQLite C/C++ 实战指南

## 1. 核心概念

SQLite 是嵌入式关系型数据库，整个数据库就是一个文件，零配置、无服务端、事务完整、支持SQL标准。

核心特性：
- 无需安装服务端，链接 `sqlite3.lib` 即可使用
- 单文件存储（`.db` 或 `.sqlite`），方便备份/迁移
- 支持最大 281 TB 的数据库
- ACID事务，崩溃恢复
- 支持内存数据库（`:memory:`）

**为什么用SQLite？**桌面应用本地存储、移动端、嵌入式设备、测试/原型、小规模服务端缓存——不需要MySQL/PostgreSQL的场景首选。

---

## 2. 快速上手

### CMake 引入

```cmake
# 方式一：系统安装（推荐 vcpkg / conan）
find_package(SQLite3 REQUIRED)
target_link_libraries(myapp PRIVATE SQLite::SQLite3)

# 方式二：FetchContent 从源码编译
include(FetchContent)
FetchContent_Declare(sqlite3
    URL https://www.sqlite.org/2024/sqlite-amalgamation-3450000.zip
)
FetchContent_MakeAvailable(sqlite3)
target_link_libraries(myapp PRIVATE sqlite3)
```

### 最小示例

```cpp
#include <sqlite3.h>
#include <iostream>
#include <string>

int main() {
    sqlite3* db = nullptr;

    // 打开（不存在则创建）
    int rc = sqlite3_open("test.db", &db);
    if (rc != SQLITE_OK) {
        std::cerr << "打开失败: " << sqlite3_errmsg(db) << "\n";
        return 1;
    }

    // 创建表
    const char* create_sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id    INTEGER PRIMARY KEY AUTOINCREMENT,
            name  TEXT NOT NULL,
            age   INTEGER
        );
    )";
    rc = sqlite3_exec(db, create_sql, nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "建表失败: " << sqlite3_errmsg(db) << "\n";
    }

    // 插入数据
    const char* insert_sql = "INSERT INTO users (name, age) VALUES ('张三', 28);";
    rc = sqlite3_exec(db, insert_sql, nullptr, nullptr, nullptr);

    // 关闭
    sqlite3_close(db);
    std::cout << "SQLite操作完成!\n";
    return 0;
}
```

---

## 3. 常用API / 写法速查

### 3.1 SQL 执行：sqlite3_exec（简单操作）

```cpp
// 带回调的查询
static int callback(void* data, int argc, char** argv, char** col_name) {
    for (int i = 0; i < argc; ++i) {
        std::cout << col_name[i] << " = " << (argv[i] ? argv[i] : "NULL") << " | ";
    }
    std::cout << "\n";
    return 0;  // 返回0继续，非0则中止
}

// 查询
const char* sql = "SELECT id, name, age FROM users;";
sqlite3_exec(db, sql, callback, nullptr, nullptr);
```

### 3.2 Prepared Statement（防注入 + 高效）

```cpp
// 编译SQL语句
sqlite3_stmt* stmt = nullptr;
const char* sql = "SELECT name, age FROM users WHERE age > ?;";

if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << sqlite3_errmsg(db) << "\n";
    return;
}

// 绑定参数（1-based索引！）
sqlite3_bind_int(stmt, 1, 20);   // age > 20

// 逐行读取
while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char* name = (const char*)sqlite3_column_text(stmt, 0); // 列索引0-based
    int age = sqlite3_column_int(stmt, 1);
    std::cout << name << ", " << age << "\n";
}

sqlite3_finalize(stmt);  // 释放
```

### 3.3 绑定参数类型速查

```cpp
sqlite3_bind_int(stmt, idx, value);         // int
sqlite3_bind_int64(stmt, idx, value);       // int64_t
sqlite3_bind_double(stmt, idx, value);      // double
sqlite3_bind_text(stmt, idx, str, len, SQLITE_TRANSIENT); // 文本
sqlite3_bind_text(stmt, idx, str, -1, SQLITE_STATIC);     // 静态文本
sqlite3_bind_null(stmt, idx);               // NULL
sqlite3_bind_blob(stmt, idx, data, size, SQLITE_TRANSIENT); // 二进制

// SQLITE_TRANSIENT：SQLite 会拷贝数据
// SQLITE_STATIC：  数据生命周期长于stmt（如字符串字面量）
```

### 3.4 事务

```cpp
// 手动事务（大批量写入时性能提升数十倍！）
sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

for (int i = 0; i < 10000; ++i) {
    // 批量插入...
    sqlite3_exec(db, insert_sql, nullptr, nullptr, nullptr);
}

char* err_msg = nullptr;
int rc = sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &err_msg);
if (rc != SQLITE_OK) {
    std::cerr << "提交失败: " << err_msg << "\n";
    sqlite3_free(err_msg);
    sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
}
```

### 3.5 获取受影响行数 / 插入ID

```cpp
// 插入后获取自增ID
sqlite3_int64 last_id = sqlite3_last_insert_rowid(db);

// 受影响行数
int affected = sqlite3_changes(db);
// sqlite3_total_changes(db) 返回连接以来总变更行数
```

### 3.6 C++ RAII 封装

```cpp
class Database {
public:
    explicit Database(const std::string& path) {
        if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db_));
        }
    }

    ~Database() { if (db_) sqlite3_close(db_); }

    // 禁止拷贝
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool execute(const std::string& sql) {
        char* err = nullptr;
        int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
        if (rc != SQLITE_OK) {
            std::cerr << err << "\n";
            sqlite3_free(err);
            return false;
        }
        return true;
    }

    sqlite3* handle() { return db_; }

private:
    sqlite3* db_ = nullptr;
};

// Prepared Statement RAII 封装
class Statement {
public:
    Statement(sqlite3* db, const std::string& sql) {
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db));
        }
    }

    ~Statement() { if (stmt_) sqlite3_finalize(stmt_); }

    void bind_int(int idx, int val)   { sqlite3_bind_int(stmt_, idx, val); }
    void bind_text(int idx, const std::string& val) {
        sqlite3_bind_text(stmt_, idx, val.c_str(), -1, SQLITE_TRANSIENT);
    }

    bool step() { return sqlite3_step(stmt_) == SQLITE_ROW; }

    int column_int(int idx)    { return sqlite3_column_int(stmt_, idx); }
    std::string column_text(int idx) {
        const char* s = (const char*)sqlite3_column_text(stmt_, idx);
        return s ? std::string(s) : "";
    }

    sqlite3_stmt* handle() { return stmt_; }

private:
    sqlite3_stmt* stmt_ = nullptr;
};
```

---

## 4. 常见陷阱 & 最佳实践

| 陷阱 | 说明 | 正确做法 |
|------|------|----------|
| **SQL注入** | 字符串拼接SQL，"name='" + input + "'" | 始终用 prepared statement + 绑定参数 |
| **忘记 finalize** | stmt 不释放导致内存泄漏 | 用 RAII 封装自动释放 |
| **大量写入不开启事务** | 每条INSERT都是隐式事务，极慢 | 用 `BEGIN` / `COMMIT` 包裹批量写入 |
| **绑定索引用错** | 参数索引用0开始的（应从1开始） | 牢记：SQL绑定参数是 1-based |
| **多线程访问同一连接** | SQLite 默认非线程安全 | 每个线程独立连接，或用 WAL 模式 |
| **WAL模式未启用** | 读写互斥影响并发 | `PRAGMA journal_mode=WAL;` |
| **返回码不检查** | 静默失败 | 始终检查 `SQLITE_OK` |

**最佳实践：**

- 生产环境开启 WAL 模式：`PRAGMA journal_mode=WAL;`（读写可并发）
- 用 `PRAGMA foreign_keys = ON;` 启用外键约束
- 数据库连接每次打开后执行 `PRAGMA busy_timeout = 5000;` 防止锁等待挂死
- 大二进制数据（>1MB）存文件系统，SQLite只存路径

---

## 5. 生产项目典型用法

### 场景：本地用户数据存储

```cpp
#include "Database.h"     // 我们的RAII封装
#include "Statement.h"

class UserRepository {
public:
    explicit UserRepository(const std::string& db_path) : db_(db_path) {
        init_schema();
    }

    // 初始化表结构
    void init_schema() {
        db_.execute(R"(
            CREATE TABLE IF NOT EXISTS users (
                id         INTEGER PRIMARY KEY AUTOINCREMENT,
                username   TEXT NOT NULL UNIQUE,
                email      TEXT NOT NULL,
                created_at TEXT NOT NULL DEFAULT (datetime('now')),
                is_active  INTEGER NOT NULL DEFAULT 1
            );
        )");
        db_.execute("PRAGMA journal_mode=WAL;");
        db_.execute("PRAGMA foreign_keys=ON;");
    }

    // 添加用户（防注入）
    bool add_user(const std::string& username, const std::string& email) {
        Statement stmt(db_.handle(),
            "INSERT INTO users (username, email) VALUES (?, ?);");
        stmt.bind_text(1, username);
        stmt.bind_text(2, email);

        while (stmt.step()) {}  // 执行
        return true;
    }

    // 查找用户
    struct User {
        int id;
        std::string username;
        std::string email;
        bool is_active;
    };

    std::optional<User> find_by_username(const std::string& username) {
        Statement stmt(db_.handle(),
            "SELECT id, username, email, is_active FROM users WHERE username = ?;");
        stmt.bind_text(1, username);

        if (stmt.step()) {
            return User{
                stmt.column_int(0),
                stmt.column_text(1),
                stmt.column_text(2),
                stmt.column_int(3) != 0
            };
        }
        return std::nullopt;
    }

    // 更新
    bool deactivate_user(int user_id) {
        Statement stmt(db_.handle(),
            "UPDATE users SET is_active = 0 WHERE id = ?;");
        stmt.bind_int(1, user_id);
        while (stmt.step()) {}
        return sqlite3_changes(db_.handle()) > 0;
    }

    // 删除
    bool delete_user(int user_id) {
        Statement stmt(db_.handle(),
            "DELETE FROM users WHERE id = ?;");
        stmt.bind_int(1, user_id);
        while (stmt.step()) {}
        return sqlite3_changes(db_.handle()) > 0;
    }

private:
    Database db_;
};

int main() {
    UserRepository repo("app_data.db");

    // 添加
    repo.add_user("alice", "alice@example.com");
    repo.add_user("bob", "bob@example.com");

    // 查找
    if (auto user = repo.find_by_username("alice")) {
        std::cout << "找到用户: " << user->email << "\n";
    }

    // 停用
    repo.deactivate_user(1);

    return 0;
}
```

---

## 参考链接

- [SQLite 官方文档](https://www.sqlite.org/docs.html)
- [SQLite C/C++ Interface](https://www.sqlite.org/c3ref/intro.html)
