/*
 * main.cpp —— 学习助手应用入口
 *
 * ============================================================
 * 这个文件整合了所有模块，展示：
 *   1. CMake 管理的多模块链接
 *   2. STL 的广泛使用 (vector, map, string, algorithm...)
 *   3. SQLite 数据库操作 (CRUD + 全文搜索)
 *   4. 多线程后台任务 (线程池 + 异步下载)
 *   5. HTTP 网络请求 (libcurl / WinHTTP)
 *   6. JSON 数据交换 (解析 + 生成)
 *   7. 插件系统动态加载
 * ============================================================
 *
 * 运行时说明：
 *   - 首次运行自动创建 SQLite 数据库
 *   - 从 plugins/ 目录加载插件
 *   - 提供交互式控制台菜单
 */
#include "core/app_config.h"
#include "core/logger.h"
#include "core/error_handler.h"
#include "database/sqlite_wrapper.h"
#include "database/note_repository.h"
#include "threading/thread_pool.h"
#include "threading/task_manager.h"
#include "network/http_client.h"
#include "network/json_parser.h"
#include "plugins/plugin_manager.h"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>

using namespace la;

// ======================= 控制台菜单 =======================

void showMenu() {
    std::cout << "\n"
              << "╔════════════════════════════════════════╗\n"
              << "║       C++ 学习助手 (LearningAssistant) ║\n"
              << "╠════════════════════════════════════════╣\n"
              << "║  1. 创建笔记     (SQLite INSERT)       ║\n"
              << "║  2. 查看全部     (SQLite SELECT)       ║\n"
              << "║  3. 搜索笔记     (FTS5 全文搜索)       ║\n"
              << "║  4. 删除笔记     (SQLite DELETE)       ║\n"
              << "║  5. HTTP请求测试 (libcurl GET/POST)    ║\n"
              << "║  6. JSON演示     (解析/生成)           ║\n"
              << "║  7. 多线程测试   (ThreadPool)          ║\n"
              << "║  8. 插件管理     (动态库加载)          ║\n"
              << "║  9. 配置管理     (文件读写)            ║\n"
              << "║  0. 退出                               ║\n"
              << "╚════════════════════════════════════════╝\n"
              << "请选择: ";
}

// ======================= 各功能演示 =======================

/// 功能1 & 2：SQLite CRUD 演示
void demoCRUD(SqliteDB& db, NoteRepository& repo) {
    std::cout << "\n--- SQLite CRUD 演示 ---\n";
    std::cout << "标题: ";
    std::string title, content, category;
    std::getline(std::cin >> std::ws, title);
    std::cout << "内容: ";
    std::getline(std::cin, content);
    std::cout << "分类: ";
    std::getline(std::cin, category);

    // 使用 RAII 事务确保数据一致性
    Transaction txn(db);

    Note note;
    note.title = title;
    note.content = content;
    note.category = category.empty() ? "默认" : category;

    int64_t id = repo.create(note);
    txn.commit();

    std::cout << "笔记已创建，ID: " << id << "\n";
}

/// 功能2：查看全部笔记
void demoListAll(NoteRepository& repo) {
    std::cout << "\n--- 全部笔记 ---\n";
    // STL: auto + range-for 遍历
    auto notes = repo.getAll();
    if (notes.empty()) {
        std::cout << "（暂无笔记）\n";
        return;
    }
    for (const auto& n : notes) {
        std::cout << "[" << n.id << "] " << n.title
                  << " [" << n.category << "] "
                  << n.updatedAt << "\n";
    }
}

/// 功能3：FTS5 全文搜索
void demoSearch(NoteRepository& repo) {
    std::cout << "\n--- FTS5 全文搜索 ---\n";
    std::cout << "搜索关键词: ";
    std::string keyword;
    std::getline(std::cin >> std::ws, keyword);

    auto results = repo.search(keyword);
    std::cout << "找到 " << results.size() << " 条结果:\n";
    for (const auto& n : results) {
        std::cout << "  [" << n.id << "] " << n.title << "\n";
        // STL: substr 截取内容摘要
        std::string preview = n.content.substr(0, std::min<size_t>(n.content.size(), 80));
        std::cout << "      " << preview << "...\n";
    }
}

/// 功能4：删除笔记
void demoDelete(SqliteDB& db, NoteRepository& repo) {
    demoListAll(repo);
    std::cout << "输入要删除的笔记ID: ";
    int64_t id;
    std::cin >> id;

    Transaction txn(db);
    if (repo.remove(id)) {
        txn.commit();
        std::cout << "笔记 " << id << " 已删除\n";
    } else {
        std::cout << "删除失败\n";
    }
}

/// 功能5：HTTP 请求演示
void demoHTTP(HttpClient& client) {
    std::cout << "\n--- HTTP 请求演示 ---\n";
    std::cout << "1. GET 请求测试\n";
    std::cout << "2. POST 请求测试\n";
    std::cout << "选择: ";
    int choice;
    std::cin >> choice;

    if (choice == 1) {
        std::cout << "正在 GET http://httpbin.org/get ...\n";
        HttpResponse resp = client.get("http://httpbin.org/get");
        std::cout << "状态码: " << resp.statusCode << "\n";
        std::cout << "响应体(前200字符): " << resp.body.substr(0, 200) << "\n";
    } else if (choice == 2) {
        // 使用 JSON 构造请求体
        JsonObject body;
        body["name"] = JsonValue(std::string("LearningAssistant"));
        body["version"] = JsonValue(std::string("1.0"));
        JsonValue jsonBody(body);

        std::cout << "正在 POST ...\n";
        std::cout << "请求体: " << jsonBody.dump(2) << "\n";
        HttpResponse resp = client.post("http://httpbin.org/post", jsonBody.dump());
        std::cout << "状态码: " << resp.statusCode << "\n";
        std::cout << "响应(前200字符): " << resp.body.substr(0, 200) << "\n";
    }
}

/// 功能6：JSON 演示
void demoJSON() {
    std::cout << "\n--- JSON 解析与生成 ---\n";

    // 示例 JSON 字符串
    std::string rawJson = R"({
        "name": "C++学习助手",
        "version": 1,
        "modules": ["STL", "CMake", "SQLite", "Qt", "OpenCV"],
        "settings": {
            "theme": "dark",
            "autosave": true
        }
    })";

    std::cout << "原始 JSON:\n" << rawJson << "\n\n";

    // 解析 JSON
    JsonValue parsed = JsonValue::parse(rawJson);

    // 访问解析后的数据
    std::cout << "解析结果:\n";
    std::cout << "  name: " << parsed["name"].getString() << "\n";
    std::cout << "  version: " << parsed["version"].getInt() << "\n";
    std::cout << "  modules[0]: " << parsed["modules"][0].getString() << "\n";
    std::cout << "  settings.theme: " << (*parsed["settings"].get("theme")).getString() << "\n";

    // 构造并生成 JSON
    JsonObject newObj;
    newObj["status"] = JsonValue(std::string("ok"));
    newObj["count"] = JsonValue(static_cast<int64_t>(42));
    JsonObject inner;
    inner["x"] = JsonValue(10.5);
    inner["y"] = JsonValue(20.3);
    newObj["position"] = JsonValue(inner);

    JsonValue newJson(newObj);
    std::cout << "\n生成的 JSON:\n" << newJson.dump(2) << "\n";
}

/// 功能7：多线程演示
void demoThreading(ThreadPool& pool) {
    std::cout << "\n--- 多线程演示 ---\n";
    std::cout << "创建 20 个异步任务...\n";

    // STL: vector 存储 future
    // std::future: 异步获取任务结果
    std::vector<std::future<int>> futures;

    for (int i = 0; i < 20; ++i) {
        // 向线程池提交任务
        futures.push_back(pool.enqueue([i]() -> int {
            // 模拟耗时操作
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return i * i;  // 返回 i 的平方
        }));
    }

    std::cout << "等待所有任务完成...\n";
    int sum = 0;
    for (size_t i = 0; i < futures.size(); ++i) {
        // future::get: 阻塞等待任务完成并获取结果
        int result = futures[i].get();
        sum += result;
        if (i < 5) {
            std::cout << "  任务 " << i << " 结果: " << result << "\n";
        }
    }
    std::cout << "... 共 " << futures.size() << " 个任务\n";
    std::cout << "平方和: " << sum << "\n";
}

/// 功能8：插件管理演示
void demoPlugins(PluginManager& pluginMgr) {
    std::cout << "\n--- 插件系统演示 ---\n";
    std::cout << "已加载 " << pluginMgr.plugins().size() << " 个插件:\n";

    // STL: for_each 遍历
    for (const auto& info : pluginMgr.plugins()) {
        std::cout << "  - " << info.name << " v" << info.version
                  << " (" << info.filepath << ")\n";
    }

    if (pluginMgr.plugins().empty()) {
        std::cout << "  (无插件，请将示例插件放到 plugins/ 目录)\n";
    }
}

/// 功能9：配置管理演示
void demoConfig() {
    std::cout << "\n--- 配置管理演示 ---\n";
    auto& config = AppConfig::instance();

    config.setString("username", "C++学习者");
    config.setInt("max_notes", 100);
    config.setBool("dark_mode", true);

    config.saveToFile("learning_assistant.conf");

    std::cout << "配置已保存到 learning_assistant.conf\n";
    std::cout << "  username = " << config.getString("username") << "\n";
    std::cout << "  max_notes = " << config.getInt("max_notes") << "\n";
    std::cout << "  dark_mode = " << (config.getBool("dark_mode") ? "true" : "false") << "\n";
}

// ======================= 主函数 =======================

int main() {
    // ===== 1. 初始化日志 =====
    Logger::instance().setMinLevel(LogLevel::Debug);
    LA_LOG_INFO("LearningAssistant starting...");

    // ===== 2. 打开数据库 =====
    // RAII: SqliteDB 构造时打开，析构时自动关闭
    SqliteDB db("learning_assistant.db");
    if (!db.isOpen()) {
        LA_LOG_ERROR("Cannot open database, exiting.");
        return 1;
    }

    // ===== 3. 初始化数据表 =====
    NoteRepository repo(db);
    repo.initTables();

    // ===== 4. 创建线程池 =====
    // 自动检测硬件并发数
    ThreadPool pool(4);
    TaskManager taskMgr(pool);

    // ===== 5. 初始化 HTTP 客户端 =====
    HttpClient httpClient;
    httpClient.setTimeout(10);
    httpClient.setDefaultHeader("User-Agent", "LearningAssistant/1.0");

    // ===== 6. 加载插件 =====
    PluginManager pluginMgr;
    size_t pluginCount = pluginMgr.loadPluginsFrom("plugins");
    LA_LOG_INFO(std::to_string(pluginCount) + " plugins loaded");

    // ===== 7. 主循环 =====
    bool running = true;
    while (running) {
        showMenu();

        int choice = -1;
        std::cin >> choice;
        // STL: cin.clear() 清除错误状态，cin.ignore() 丢弃缓冲区
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "请输入数字\n";
            continue;
        }

        switch (choice) {
        case 1: demoCRUD(db, repo); break;
        case 2: demoListAll(repo); break;
        case 3: demoSearch(repo); break;
        case 4: demoDelete(db, repo); break;
        case 5: demoHTTP(httpClient); break;
        case 6: demoJSON(); break;
        case 7: demoThreading(pool); break;
        case 8: demoPlugins(pluginMgr); break;
        case 9: demoConfig(); break;
        case 0:
            running = false;
            std::cout << "再见！\n";
            break;
        default:
            std::cout << "无效选项\n";
            break;
        }
    }

    // ===== 8. 清理 =====
    // 所有 RAII 对象在离开作用域时自动清理：
    //   - pool 析构 → 停止所有线程
    //   - pluginMgr 析构 → 卸载所有插件
    //   - httpClient 析构 → 释放 curl 句柄
    //   - db 析构 → 关闭数据库连接

    LA_LOG_INFO("LearningAssistant exiting.");
    return 0;
}
