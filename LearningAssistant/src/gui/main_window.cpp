/*
 * main_window.cpp —— Qt 主窗口实现
 *
 * 知识点：
 *   1. connect() 信号槽连接（编译期类型检查）
 *   2. Lambda 槽函数（C++11起）
 *   3. QWidget 布局嵌套
 *   4. 集成后端 C++ 模块到 Qt UI
 */
#ifdef HAS_QT

#include "gui/main_window.h"
#include "database/sqlite_wrapper.h"
#include "database/note_repository.h"
#include "threading/thread_pool.h"
#include "network/http_client.h"
#include "network/json_parser.h"
#include "plugins/plugin_manager.h"

#include <QMessageBox>
#include <QSplitter>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    initBackend();
    setupUI();
    setupMenuBar();
    setupStatusBar();

    // 初始加载笔记列表
    onRefreshNotes();
}

MainWindow::~MainWindow() = default;

// ==================== 后端初始化 ====================

void MainWindow::initBackend() {
    // 打开数据库
    m_db = std::make_unique<la::SqliteDB>("learning_assistant_gui.db");
    m_repo = std::make_unique<la::NoteRepository>(*m_db);
    m_repo->initTables();

    // 创建线程池（2线程，GUI应用不宜占满CPU）
    m_threadPool = std::make_unique<la::ThreadPool>(2);

    // HTTP 客户端
    m_httpClient = std::make_unique<la::HttpClient>();
    m_httpClient->setTimeout(10);

    // 插件管理器
    m_pluginMgr = std::make_unique<la::PluginManager>();
    m_pluginMgr->loadPluginsFrom("plugins");
}

// ==================== UI 搭建 ====================

void MainWindow::setupUI() {
    m_tabWidget = new QTabWidget(this);

    // ---- Tab 1: 笔记管理 ----
    QWidget* noteTab = new QWidget();
    QVBoxLayout* noteLayout = new QVBoxLayout(noteTab);

    // 顶部：输入区
    QGroupBox* inputGroup = new QGroupBox("新建笔记");
    QFormLayout* formLayout = new QFormLayout(inputGroup);
    m_titleEdit = new QLineEdit();
    m_titleEdit->setPlaceholderText("笔记标题...");
    m_categoryEdit = new QLineEdit();
    m_categoryEdit->setPlaceholderText("分类 (可选)...");
    formLayout->addRow("标题:", m_titleEdit);
    formLayout->addRow("分类:", m_categoryEdit);

    QPushButton* addBtn = new QPushButton("添加笔记");
    formLayout->addRow(addBtn);

    // 搜索栏
    QHBoxLayout* searchLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("搜索笔记 (FTS5全文搜索)...");
    QPushButton* searchBtn = new QPushButton("搜索");
    QPushButton* refreshBtn = new QPushButton("刷新");
    searchLayout->addWidget(m_searchEdit);
    searchLayout->addWidget(searchBtn);
    searchLayout->addWidget(refreshBtn);

    // 中部：分割器（列表 + 内容）
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    m_noteList = new QListWidget();
    m_noteContent = new QTextEdit();
    m_noteContent->setReadOnly(true);
    m_noteContent->setPlaceholderText("选择笔记查看内容...");
    splitter->addWidget(m_noteList);
    splitter->addWidget(m_noteContent);
    splitter->setStretchFactor(0, 1);  // 列表占1份
    splitter->setStretchFactor(1, 2);  // 内容占2份

    // 底部：操作按钮
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* deleteBtn = new QPushButton("删除选中笔记");
    btnLayout->addStretch();
    btnLayout->addWidget(deleteBtn);

    noteLayout->addWidget(inputGroup);
    noteLayout->addLayout(searchLayout);
    noteLayout->addWidget(splitter);
    noteLayout->addLayout(btnLayout);

    m_tabWidget->addTab(noteTab, "📝 笔记管理");

    // ---- Tab 2: HTTP 测试 ----
    QWidget* httpTab = new QWidget();
    QVBoxLayout* httpLayout = new QVBoxLayout(httpTab);

    QHBoxLayout* urlLayout = new QHBoxLayout();
    m_urlEdit = new QLineEdit("http://httpbin.org/get");
    m_getBtn = new QPushButton("GET");
    m_postBtn = new QPushButton("POST");
    urlLayout->addWidget(m_urlEdit);
    urlLayout->addWidget(m_getBtn);
    urlLayout->addWidget(m_postBtn);

    m_responseView = new QTextEdit();
    m_responseView->setReadOnly(true);

    httpLayout->addLayout(urlLayout);
    httpLayout->addWidget(m_responseView);

    m_tabWidget->addTab(httpTab, "🌐 HTTP 测试");

    // ---- Tab 3: JSON 演示 ----
    QWidget* jsonTab = new QWidget();
    QVBoxLayout* jsonLayout = new QVBoxLayout(jsonTab);

    m_jsonInput = new QTextEdit();
    m_jsonInput->setPlainText(R"({"name":"测试","items":[1,2,3]})");
    QPushButton* jsonParseBtn = new QPushButton("解析 JSON");
    m_jsonOutput = new QTextEdit();
    m_jsonOutput->setReadOnly(true);

    jsonLayout->addWidget(new QLabel("输入 JSON:"));
    jsonLayout->addWidget(m_jsonInput);
    jsonLayout->addWidget(jsonParseBtn);
    jsonLayout->addWidget(new QLabel("解析结果:"));
    jsonLayout->addWidget(m_jsonOutput);

    m_tabWidget->addTab(jsonTab, "📋 JSON");

    // ---- Tab 4: 多线程演示 ----
    QWidget* threadTab = new QWidget();
    QVBoxLayout* threadLayout = new QVBoxLayout(threadTab);
    QPushButton* threadDemoBtn = new QPushButton("运行多线程演示");
    QTextEdit* threadOutput = new QTextEdit();
    threadOutput->setReadOnly(true);
    threadLayout->addWidget(threadDemoBtn);
    threadLayout->addWidget(threadOutput);
    m_tabWidget->addTab(threadTab, "⚡ 多线程");

    setCentralWidget(m_tabWidget);

    // ====== 信号槽连接 ======
    // connect() 五要素：发送者, 信号, 接收者, 槽, 连接类型
    // 编译期类型检查：信号和槽的参数必须匹配

    // 传统写法：函数指针
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::onAddNote);
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshNotes);
    connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteNote);
    connect(searchBtn, &QPushButton::clicked, this, &MainWindow::onSearchNote);
    connect(m_noteList, &QListWidget::itemClicked, this, &MainWindow::onNoteSelected);

    // Lambda 槽（C++11起支持）
    // 优点：代码内联，无需声明槽函数
    connect(m_searchEdit, &QLineEdit::returnPressed, this, [this]() {
        onSearchNote();  // 回车搜索
    });

    connect(m_getBtn, &QPushButton::clicked, this, &MainWindow::onHttpTest);
    connect(m_postBtn, &QPushButton::clicked, this, &MainWindow::onHttpTest);
    connect(jsonParseBtn, &QPushButton::clicked, this, &MainWindow::onJsonDemo);

    // 多线程演示（lambda 槽 + 线程池异步）
    connect(threadDemoBtn, &QPushButton::clicked, this, [this, threadOutput]() {
        threadOutput->clear();
        threadOutput->append("提交20个异步任务...");
        std::vector<std::future<int>> futures;
        for (int i = 0; i < 20; ++i) {
            futures.push_back(m_threadPool->enqueue([i]() -> int {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                return i * i;
            }));
        }
        for (size_t i = 0; i < futures.size(); ++i) {
            threadOutput->append(QString("任务 %1 → %2").arg(i).arg(futures[i].get()));
        }
        threadOutput->append("全部完成!");
    });
}

void MainWindow::setupMenuBar() {
    QMenu* fileMenu = menuBar()->addMenu("文件(&F)");
    QAction* exitAction = fileMenu->addAction("退出(&Q)");
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

    QMenu* helpMenu = menuBar()->addMenu("帮助(&H)");
    QAction* aboutAction = helpMenu->addAction("关于(&A)");
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "关于",
            "C++学习助手 v1.0\n\n"
            "涵盖: STL, CMake, SQLite, Qt, HTTP, JSON, 多线程, 插件");
    });

    QAction* pluginAction = helpMenu->addAction("插件信息");
    connect(pluginAction, &QAction::triggered, this, &MainWindow::onPluginInfo);
}

void MainWindow::setupStatusBar() {
    statusBar()->showMessage("就绪 | 插件: " +
        QString::number(m_pluginMgr->plugins().size()) + " 个");
}

// ==================== 槽函数实现 ====================

void MainWindow::onAddNote() {
    QString title = m_titleEdit->text().trimmed();
    if (title.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入标题");
        return;
    }
    la::Note note;
    note.title = title.toStdString();
    note.category = m_categoryEdit->text().trimmed().toStdString();
    note.content = "";

    la::Transaction txn(*m_db);
    int64_t id = m_repo->create(note);
    txn.commit();

    m_titleEdit->clear();
    m_categoryEdit->clear();
    statusBar()->showMessage(QString("笔记已创建，ID: %1").arg(id), 3000);
    onRefreshNotes();
}

void MainWindow::onRefreshNotes() {
    m_noteList->clear();
    auto notes = m_repo->getAll();
    for (const auto& n : notes) {
        QString label = QString("[%1] %2").arg(n.id).arg(QString::fromStdString(n.title));
        QListWidgetItem* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, static_cast<qlonglong>(n.id));  // 存储ID
        m_noteList->addItem(item);
    }
    statusBar()->showMessage(QString("共 %1 条笔记").arg(notes.size()));
}

void MainWindow::onNoteSelected() {
    auto* item = m_noteList->currentItem();
    if (!item) return;
    int64_t id = item->data(Qt::UserRole).toLongLong();
    auto note = m_repo->getById(id);
    m_noteContent->setHtml(
        QString("<h2>%1</h2><p><i>分类: %2 | 更新: %3</i></p><hr>%4")
            .arg(QString::fromStdString(note.title))
            .arg(QString::fromStdString(note.category))
            .arg(QString::fromStdString(note.updatedAt))
            .arg(QString::fromStdString(note.content).replace("\n", "<br>"))
    );
}

void MainWindow::onDeleteNote() {
    auto* item = m_noteList->currentItem();
    if (!item) return;
    int64_t id = item->data(Qt::UserRole).toLongLong();
    la::Transaction txn(*m_db);
    m_repo->remove(id);
    txn.commit();
    onRefreshNotes();
    m_noteContent->clear();
}

void MainWindow::onSearchNote() {
    QString keyword = m_searchEdit->text().trimmed();
    if (keyword.isEmpty()) { onRefreshNotes(); return; }
    m_noteList->clear();
    auto results = m_repo->search(keyword.toStdString());
    for (const auto& n : results) {
        m_noteList->addItem(QString("[%1] %2").arg(n.id).arg(QString::fromStdString(n.title)));
    }
    statusBar()->showMessage(QString("找到 %1 条结果").arg(results.size()));
}

void MainWindow::onHttpTest() {
    // 在后台线程执行HTTP请求，避免阻塞UI
    QString url = m_urlEdit->text().trimmed();
    m_responseView->setPlainText("请求中...");

    // 使用线程池异步执行
    m_threadPool->enqueue([this, url]() {
        HttpResponse resp = m_httpClient->get(url.toStdString());
        // Qt: 跨线程更新UI必须使用信号槽或 QMetaObject::invokeMethod
        QMetaObject::invokeMethod(this, [this, resp]() {
            m_responseView->setPlainText(
                QString("状态码: %1\n\n%2")
                    .arg(resp.statusCode)
                    .arg(QString::fromStdString(resp.body.substr(0, 2000)))
            );
        }, Qt::QueuedConnection);
    });
}

void MainWindow::onJsonDemo() {
    try {
        std::string input = m_jsonInput->toPlainText().toStdString();
        la::JsonValue parsed = la::JsonValue::parse(input);
        m_jsonOutput->setPlainText(QString::fromStdString(parsed.dump(2)));
    } catch (const std::exception& e) {
        m_jsonOutput->setPlainText(QString("解析错误: %1").arg(e.what()));
    }
}

void MainWindow::onPluginInfo() {
    QStringList info;
    for (const auto& p : m_pluginMgr->plugins()) {
        info << QString("%1 v%2").arg(QString::fromStdString(p.name),
                                       QString::fromStdString(p.version));
    }
    QMessageBox::information(this, "已加载插件",
        info.isEmpty() ? "无插件" : info.join("\n"));
}

#endif // HAS_QT
