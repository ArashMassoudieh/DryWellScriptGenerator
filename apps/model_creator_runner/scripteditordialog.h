#ifndef SCRIPTEDITORDIALOG_H
#define SCRIPTEDITORDIALOG_H

#include <QDialog>

class QTextEdit;

class ScriptEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScriptEditorDialog(QWidget *parent = nullptr);

    void setScriptText(const QString &text);
    QString scriptText() const;

private:
    QTextEdit *editor;
};

#endif // SCRIPTEDITORDIALOG_H
