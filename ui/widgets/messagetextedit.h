#ifndef MESSAGETEXTEDIT_H
#define MESSAGETEXTEDIT_H

#include <QTextEdit>
#include <QObject>
#include <QWidget>

namespace Ui
{
class MessageTextEdit;
}

class MessageTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit MessageTextEdit(QWidget *parent = nullptr) : QTextEdit(parent) {}
    explicit MessageTextEdit(const QString &text, QWidget *parent = nullptr) : QTextEdit(text, parent) {}

signals:
    void imageInserted(const QImage& image);
    void fileInserted(const QString& localPath);

protected:
    bool canInsertFromMimeData(const QMimeData* source) const override;
    void insertFromMimeData(const QMimeData* source) override;

};

#endif // MESSAGETEXTEDIT_H
