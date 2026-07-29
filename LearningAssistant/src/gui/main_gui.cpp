/*
 * main_gui.cpp —— Qt GUI 入口
 *
 * 知识点：
 *   1. QApplication —— Qt 事件循环
 *   2. QMainWindow —— 主窗口
 *   3. 信号与槽 (Signals & Slots) —— Qt 核心通信机制
 *   4. QWidget 层级结构
 *
 * 编译条件：cmake .. -DENABLE_QT=ON（需安装 Qt5/Qt6）
 */
#ifdef HAS_QT

#include <QApplication>
#include "gui/main_window.h"

int main(int argc, char* argv[]) {
    // QApplication: 管理 GUI 应用的控制流和主要设置
    QApplication app(argc, argv);
    app.setApplicationName("C++学习助手");
    app.setApplicationVersion("1.0.0");

    // 创建并显示主窗口
    MainWindow window;
    window.setWindowTitle("C++学习助手 - LearningAssistant");
    window.resize(1024, 768);
    window.show();

    // app.exec(): 进入 Qt 事件循环，阻塞直到窗口关闭
    return app.exec();
}

#endif
