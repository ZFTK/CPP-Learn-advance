#include "database/note_repository.h"
#include "core/logger.h"
#include <sstream>

namespace la {

NoteRepository::NoteRepository(SqliteDB& db) : m_db(db) {}

bool NoteRepository::initTables() {
    // SQLite CREATE TABLE IF NOT EXISTS —— 幂等建表
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS notes (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            title      TEXT NOT NULL,
            content    TEXT DEFAULT '',
            category   TEXT DEFAULT '默认',
            created_at TEXT DEFAULT (datetime('now','localtime')),
            updated_at TEXT DEFAULT (datetime('now','localtime'))
        );
        -- FTS5 全文搜索：提升搜索性能
        CREATE VIRTUAL TABLE IF NOT EXISTS notes_fts USING fts5(
            title, content, content=notes, content_rowid=id
        );
        -- 触发器：自动同步 FTS 索引
        CREATE TRIGGER IF NOT EXISTS notes_ai AFTER INSERT ON notes BEGIN
            INSERT INTO notes_fts(rowid, title, content) VALUES (new.id, new.title, new.content);
        END;
        CREATE TRIGGER IF NOT EXISTS notes_ad AFTER DELETE ON notes BEGIN
            INSERT INTO notes_fts(notes_fts, rowid, title, content) VALUES('delete', old.id, old.title, old.content);
        END;
        CREATE TRIGGER IF NOT EXISTS notes_au AFTER UPDATE ON notes BEGIN
            INSERT INTO notes_fts(notes_fts, rowid, title, content) VALUES('delete', old.id, old.title, old.content);
            INSERT INTO notes_fts(rowid, title, content) VALUES (new.id, new.title, new.content);
        END;
    )";
    return m_db.execute(sql);
}

int64_t NoteRepository::create(const Note& note) {
    // 使用参数化查询防 SQL 注入
    auto rows = m_db.query(
        "INSERT INTO notes (title, content, category) VALUES (?, ?, ?)",
        note.title, note.content, note.category
    );
    return m_db.lastInsertRowId();
}

Note NoteRepository::getById(int64_t id) {
    Note note;
    auto rows = m_db.query("SELECT * FROM notes WHERE id = ?", id);
    if (!rows.empty()) {
        note = rowToNote(rows[0]);
    }
    return note;
}

std::vector<Note> NoteRepository::getAll() {
    std::vector<Note> result;
    auto rows = m_db.query("SELECT * FROM notes ORDER BY updated_at DESC");
    for (const auto& row : rows) {
        result.push_back(rowToNote(row));
    }
    return result;
}

std::vector<Note> NoteRepository::getByCategory(const std::string& category) {
    std::vector<Note> result;
    auto rows = m_db.query("SELECT * FROM notes WHERE category = ? ORDER BY updated_at DESC", category);
    for (const auto& row : rows) result.push_back(rowToNote(row));
    return result;
}

std::vector<Note> NoteRepository::search(const std::string& keyword) {
    std::vector<Note> result;
    // 使用 FTS5 全文搜索
    auto rows = m_db.query(
        "SELECT n.* FROM notes n "
        "JOIN notes_fts f ON n.id = f.rowid "
        "WHERE notes_fts MATCH ? ORDER BY rank",
        keyword
    );
    for (const auto& row : rows) result.push_back(rowToNote(row));
    return result;
}

bool NoteRepository::update(const Note& note) {
    return m_db.execute(
        "UPDATE notes SET title=?, content=?, category=?, updated_at=datetime('now','localtime') WHERE id=?",
        note.title, note.content, note.category, note.id
    );
}

bool NoteRepository::remove(int64_t id) {
    return m_db.execute("DELETE FROM notes WHERE id = ?", id);
}

Note NoteRepository::rowToNote(const Row& row) {
    Note note;
    // STL: 通过列名查找值
    for (const auto& [colName, colValue] : row) {  // C++17 结构化绑定
        if (colName == "id")         note.id        = std::stoll(colValue);
        if (colName == "title")      note.title     = colValue;
        if (colName == "content")    note.content   = colValue;
        if (colName == "category")   note.category  = colValue;
        if (colName == "created_at") note.createdAt = colValue;
        if (colName == "updated_at") note.updatedAt = colValue;
    }
    return note;
}

} // namespace la
