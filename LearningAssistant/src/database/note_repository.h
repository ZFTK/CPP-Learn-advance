/*
 * note_repository.h —— 笔记数据仓库
 *
 * 知识点：
 *   1. Repository 设计模式 —— 隔离数据访问逻辑
 *   2. 结构化绑定 (Structured Bindings)
 *   3. std::chrono 时间戳
 *   4. SQLite 表设计与 CRUD 操作
 */
#pragma once

#include "database/sqlite_wrapper.h"
#include <string>
#include <vector>
#include <chrono>

namespace la {

/// @brief 笔记实体
struct Note {
    int64_t id = 0;
    std::string title;
    std::string content;
    std::string category;
    std::string createdAt;
    std::string updatedAt;
};

/// @brief 笔记仓库 —— 封装所有数据库操作
class NoteRepository {
public:
    explicit NoteRepository(SqliteDB& db);

    // 初始化表结构
    bool initTables();

    // ---- CRUD ----
    // Create: 创建笔记，返回新ID
    int64_t create(const Note& note);
    // Read: 按ID查询
    Note getById(int64_t id);
    // Read: 查询全部
    std::vector<Note> getAll();
    // Read: 按分类查询
    std::vector<Note> getByCategory(const std::string& category);
    // Read: 搜索标题和内容
    std::vector<Note> search(const std::string& keyword);
    // Update: 更新笔记
    bool update(const Note& note);
    // Delete: 删除笔记
    bool remove(int64_t id);

private:
    Note rowToNote(const Row& row);
    SqliteDB& m_db;
};

} // namespace la
