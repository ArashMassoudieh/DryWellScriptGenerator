#include "scripteditordialog.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

ScriptEditorDialog::ScriptEditorDialog(QWidget *parent)
    : QDialog(parent), editor(new QTextEdit(this))
{
    setWindowTitle(tr("Review / Edit OHQ Script"));
    resize(1000, 700);

    auto *layout = new QVBoxLayout(this);
    editor->setLineWrapMode(QTextEdit::NoWrap);
    layout->addWidget(editor, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void ScriptEditorDialog::setScriptText(const QString &text)
{
    editor->setPlainText(text);
}

QString ScriptEditorDialog::scriptText() const
{
    return editor->toPlainText();
}
