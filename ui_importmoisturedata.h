/********************************************************************************
** Form generated from reading UI file 'importmoisturedata.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_IMPORTMOISTUREDATA_H
#define UI_IMPORTMOISTUREDATA_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_ImportMoistureData
{
public:
    QVBoxLayout *verticalLayout;
    QPushButton *ChooseFolder;
    QPushButton *pushButtonExport;

    void setupUi(QDialog *ImportMoistureData)
    {
        if (ImportMoistureData->objectName().isEmpty())
            ImportMoistureData->setObjectName(QString::fromUtf8("ImportMoistureData"));
        ImportMoistureData->resize(400, 300);
        verticalLayout = new QVBoxLayout(ImportMoistureData);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        ChooseFolder = new QPushButton(ImportMoistureData);
        ChooseFolder->setObjectName(QString::fromUtf8("ChooseFolder"));

        verticalLayout->addWidget(ChooseFolder);

        pushButtonExport = new QPushButton(ImportMoistureData);
        pushButtonExport->setObjectName(QString::fromUtf8("pushButtonExport"));

        verticalLayout->addWidget(pushButtonExport);


        retranslateUi(ImportMoistureData);

        QMetaObject::connectSlotsByName(ImportMoistureData);
    } // setupUi

    void retranslateUi(QDialog *ImportMoistureData)
    {
        ImportMoistureData->setWindowTitle(QCoreApplication::translate("ImportMoistureData", "Dialog", nullptr));
        ChooseFolder->setText(QCoreApplication::translate("ImportMoistureData", "ChooseFolder", nullptr));
        pushButtonExport->setText(QCoreApplication::translate("ImportMoistureData", "Export", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ImportMoistureData: public Ui_ImportMoistureData {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_IMPORTMOISTUREDATA_H
