/*
 * note_editor.h —— 笔记编辑器窗口（Qt桩文件）
 *
 * 在完整版本中：
 *   1. 提供富文本编辑器编辑笔记内容
 *   2. 支持 Markdown 实时预览
 *   3. 图像拖拽上传
 */
#ifdef HAS_QT
#pragma once
#include <QDialog>
class NoteEditor : public QDialog {
    Q_OBJECT
public:
    explicit NoteEditor(QWidget* parent = nullptr) : QDialog(parent) {}
};
#endif
