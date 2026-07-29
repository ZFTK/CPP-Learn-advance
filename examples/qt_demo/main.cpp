/*
 * qt_demo_main.cpp —— Qt 信号槽 + 控件速成示例
 *
 * 演示: QApplication, QMainWindow, QPushButton, QLabel,
 *       QLayout, 信号与槽, lambda 槽
 *
 * 编译 (需要 Qt 安装):
 *   CMakeLists.txt 或:
 *   qmake -project && qmake && make
 *
 * 安装 Qt:
 *   https://www.qt.io/download-open-source
 *   推荐 Qt 6.x + Qt Creator
 */
#ifdef HAS_QT

#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QString>

int main(int argc, char* argv[]) {
    // QApplication: 管理 GUI 应用的控制流
    QApplication app(argc, argv);

    // 创建主窗口
    QMainWindow window;
    window.setWindowTitle("Qt 速成示例");
    window.resize(400, 300);

    // 中心部件 + 布局
    QWidget* central = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(central);

    // 标签
    QLabel* label = new QLabel("请输入你的名字:");
    layout->addWidget(label);

    // 输入框
    QLineEdit* lineEdit = new QLineEdit();
    lineEdit->setPlaceholderText("名字...");
    layout->addWidget(lineEdit);

    // 结果显示标签
    QLabel* resultLabel = new QLabel("");
    resultLabel->setStyleSheet("font-size: 18px; color: blue;");
    layout->addWidget(resultLabel);

    // 按钮
    QPushButton* greetBtn = new QPushButton("打招呼");
    layout->addWidget(greetBtn);

    QPushButton* clearBtn = new QPushButton("清除");
    layout->addWidget(clearBtn);

    // ====== 信号与槽 ======

    // 方式1: 函数指针连接（推荐，编译期类型检查）
    QObject::connect(greetBtn, &QPushButton::clicked, [&]() {
        QString name = lineEdit->text().trimmed();
        if (name.isEmpty()) {
            QMessageBox::warning(&window, "提示", "请输入名字！");
        } else {
            resultLabel->setText("你好，" + name + "！欢迎学习Qt！");
        }
    });

    // 方式2: Lambda 槽（C++11起）
    QObject::connect(clearBtn, &QPushButton::clicked, [&]() {
        lineEdit->clear();
        resultLabel->clear();
    });

    // 方式3: 信号连接信号
    QObject::connect(lineEdit, &QLineEdit::returnPressed, greetBtn, &QPushButton::click);

    window.setCentralWidget(central);
    window.show();

    // app.exec(): 进入事件循环
    return app.exec();
}

#else
#include <iostream>
int main() {
    std::cout << "Qt 未安装，无法运行 GUI 示例。\n";
    std::cout << "请安装 Qt: https://www.qt.io/download-open-source\n";
    std::cout << "编译: 在 Qt Creator 中打开项目或使用 CMake\n";
    return 0;
}
#endif
