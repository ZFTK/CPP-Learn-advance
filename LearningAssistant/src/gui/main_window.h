/*
 * main_window.h —— Qt 主窗口
 *
 * 知识点：
 *   1. QMainWindow —— 菜单栏/工具栏/状态栏/中心部件
 *   2. 信号与槽 —— Qt 对象间通信
 *   3. QVBoxLayout / QHBoxLayout —— 布局管理
 *   4. QTabWidget —— 多页切换
 *   5. QMenuBar / QStatusBar —— 菜单与状态栏
 *   6. connect() 的五种重载形式
 */
#ifdef HAS_QT

#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QTextEdit>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QVBoxLayout>
#include <QHBoxLayout>

// 前向声明核心模块
namespace la {
    class SqliteDB;
    class NoteRepository;
    class ThreadPool;
    class HttpClient;
    class PluginManager;
}

/// @brief 主窗口
/// 整合 SQLite 笔记管理 + HTTP 请求 + 多线程 + JSON
class MainWindow : public QMainWindow {
    Q_OBJECT  // Q_OBJECT 宏：启用信号槽、元对象系统

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:  // Qt 槽函数（事件处理）
    void onAddNote();           // 添加笔记
    void onRefreshNotes();      // 刷新笔记列表
    void onNoteSelected();      // 笔记选中 → 显示内容
    void onDeleteNote();        // 删除笔记
    void onSearchNote();        // 搜索笔记
    void onHttpTest();          // HTTP 请求测试
    void onJsonDemo();          // JSON 解析演示
    void onThreadDemo();        // 多线程演示
    void onPluginInfo();        // 插件信息

private:
    void setupUI();             // 初始化界面
    void setupMenuBar();        // 初始化菜单
    void setupStatusBar();      // 初始化状态栏
    void initBackend();         // 初始化后端模块

    // ---- 后端模块 ----
    std::unique_ptr<la::SqliteDB> m_db;
    std::unique_ptr<la::NoteRepository> m_repo;
    std::unique_ptr<la::ThreadPool> m_threadPool;
    std::unique_ptr<la::HttpClient> m_httpClient;
    std::unique_ptr<la::PluginManager> m_pluginMgr;

    // ---- UI 组件 ----
    QTabWidget* m_tabWidget;

    // 笔记管理页
    QListWidget* m_noteList;
    QTextEdit* m_noteContent;
    QLineEdit* m_titleEdit;
    QLineEdit* m_categoryEdit;
    QLineEdit* m_searchEdit;

    // HTTP 测试页
    QLineEdit* m_urlEdit;
    QTextEdit* m_responseView;
    QPushButton* m_getBtn;
    QPushButton* m_postBtn;

    // JSON 演示页
    QTextEdit* m_jsonInput;
    QTextEdit* m_jsonOutput;
};

#endif // HAS_QT
