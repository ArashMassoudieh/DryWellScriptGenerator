// NOTE: This file is part of the DryWellSuite/OpenHydroQual codebase.
#ifndef SCRIPTEDITORDIALOG_H
#define SCRIPTEDITORDIALOG_H

#include <QDialog>

class QTextEdit;
class QComboBox;
class QPushButton;

class ScriptEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScriptEditorDialog(QWidget *parent = nullptr);

    void setScriptText(const QString &text);
    QString scriptText() const;

private:
    void insertSelectedSnippet();

    QTextEdit *editor;
    QComboBox *snippetCombo;
    QPushButton *insertSnippetButton;
};

#endif // SCRIPTEDITORDIALOG_H
