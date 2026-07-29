/*
 * settings_dialog.h —— 设置对话框（Qt桩文件）
 */
#ifdef HAS_QT
#pragma once
#include <QDialog>
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr) : QDialog(parent) {}
};
#endif
