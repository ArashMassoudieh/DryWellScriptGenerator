#include "scripteditordialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextCursor>

ScriptEditorDialog::ScriptEditorDialog(QWidget *parent)
    : QDialog(parent),
      editor(new QTextEdit(this)),
      snippetCombo(new QComboBox(this)),
      insertSnippetButton(new QPushButton(tr("Insert snippet"), this))
{
    setWindowTitle(tr("Review / Edit OHQ Script"));
    resize(1000, 700);

    auto *layout = new QVBoxLayout(this);

    auto *snippetRow = new QHBoxLayout();
    snippetCombo->addItem(tr("set output filename"),
                          "setvalue; object=system, quantity=outputfile, value=OHQ_output.txt");
    snippetCombo->addItem(tr("load standard template set"),
                          "loadtemplate; filename=<template_dir>/main_components.json\n"
                          "addtemplate; filename=<template_dir>/Pond_Plugin.json\n"
                          "addtemplate; filename=<template_dir>/unsaturated_soil.json\n"
                          "addtemplate; filename=<template_dir>/Well.json\n"
                          "addtemplate; filename=<template_dir>/Sewer_system.json\n"
                          "addtemplate; filename=<template_dir>/soil_evapotranspiration_models.json\n"
                          "addtemplate; filename=<template_dir>/evapotranspiration_models.json\n"
                          "addtemplate; filename=<template_dir>/pipe_pump_tank.json");
    snippetCombo->addItem(tr("load main components template"),
                          "loadtemplate; filename=<template_dir>/main_components.json");
    snippetCombo->addItem(tr("add Pond_Plugin template"),
                          "addtemplate; filename=<template_dir>/Pond_Plugin.json");
    snippetCombo->addItem(tr("add unsaturated_soil template"),
                          "addtemplate; filename=<template_dir>/unsaturated_soil.json");
    snippetCombo->addItem(tr("add Well template"),
                          "addtemplate; filename=<template_dir>/Well.json");
    snippetCombo->addItem(tr("add Sewer_system template"),
                          "addtemplate; filename=<template_dir>/Sewer_system.json");
    snippetCombo->addItem(tr("add ET templates"),
                          "addtemplate; filename=<template_dir>/soil_evapotranspiration_models.json\n"
                          "addtemplate; filename=<template_dir>/evapotranspiration_models.json");
    snippetCombo->addItem(tr("add pipe_pump_tank template"),
                          "addtemplate; filename=<template_dir>/pipe_pump_tank.json");
    snippetCombo->addItem(tr("add theta observation"),
                          "create observation;type=Observation,object=Soil (1$1),name=Obs_1,expression=theta,observed_data=obs.csv,error_structure=normal,error_standard_deviation=1");
    snippetCombo->addItem(tr("add groundwater boundary"),
                          "create block;type=fixed_head,name=GW,_width=180,_height=180,x=0,y=-420,head=-3[m],Storage=100000[m~^3]\n"
                          "create link;from=Infiltration_Pond,to=GW,type=soil_to_fixedhead_link,name=Pond_to_GW");
    snippetCombo->addItem(tr("add underdrain link"),
                          "create block;type=Pipe,name=Underdrain,_width=180,_height=180,x=320,y=-320,diameter=0.15[m],length=40[m],slope=0.01\n"
                          "create link;from=Catchment (1),to=Underdrain,type=surfacewater_to_pipe_link,name=Catchment_to_Underdrain");
    snippetCombo->addItem(tr("add user command comment"),
                          "# user_additional_commands");
    snippetRow->addWidget(snippetCombo, 1);
    snippetRow->addWidget(insertSnippetButton);
    layout->addLayout(snippetRow);

    editor->setLineWrapMode(QTextEdit::NoWrap);
    layout->addWidget(editor, 1);
    connect(insertSnippetButton, &QPushButton::clicked, this, [this]() { insertSelectedSnippet(); });

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

void ScriptEditorDialog::insertSelectedSnippet()
{
    const QString snippet = snippetCombo->currentData().toString().trimmed();
    if (snippet.isEmpty()) {
        return;
    }

    QTextCursor cursor = editor->textCursor();
    if (cursor.position() > 0) {
        cursor.insertText("\n");
    }
    cursor.insertText(snippet);
    cursor.insertText("\n");
    editor->setTextCursor(cursor);
    editor->setFocus();
}
