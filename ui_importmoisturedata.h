/********************************************************************************
** Form generated from reading UI file 'importmoisturedata.ui'
**
** Created by: Qt User Interface Compiler version 5.12.11
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_IMPORTMOISTUREDATA_H
#define UI_IMPORTMOISTUREDATA_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_ImportMoistureData
{
public:
    QVBoxLayout *verticalLayout;
    QPushButton *ChooseFolder;
    QPushButton *pushButtonExport;
    QHBoxLayout *horizontalLayout;
    QVBoxLayout *verticalLayout_2;
    QLabel *label;
    QLineEdit *center_X;
    QVBoxLayout *verticalLayout_3;
    QLabel *label_2;
    QLineEdit *center_y;
    QPushButton *Export_Radial_coordinate;

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

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        label = new QLabel(ImportMoistureData);
        label->setObjectName(QString::fromUtf8("label"));

        verticalLayout_2->addWidget(label);

        center_X = new QLineEdit(ImportMoistureData);
        center_X->setObjectName(QString::fromUtf8("center_X"));

        verticalLayout_2->addWidget(center_X);


        horizontalLayout->addLayout(verticalLayout_2);

        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        label_2 = new QLabel(ImportMoistureData);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        verticalLayout_3->addWidget(label_2);

        center_y = new QLineEdit(ImportMoistureData);
        center_y->setObjectName(QString::fromUtf8("center_y"));

        verticalLayout_3->addWidget(center_y);


        horizontalLayout->addLayout(verticalLayout_3);


        verticalLayout->addLayout(horizontalLayout);

        Export_Radial_coordinate = new QPushButton(ImportMoistureData);
        Export_Radial_coordinate->setObjectName(QString::fromUtf8("Export_Radial_coordinate"));

        verticalLayout->addWidget(Export_Radial_coordinate);


        retranslateUi(ImportMoistureData);

        QMetaObject::connectSlotsByName(ImportMoistureData);
    } // setupUi

    void retranslateUi(QDialog *ImportMoistureData)
    {
        ImportMoistureData->setWindowTitle(QApplication::translate("ImportMoistureData", "Dialog", nullptr));
        ChooseFolder->setText(QApplication::translate("ImportMoistureData", "ChooseFolder", nullptr));
        pushButtonExport->setText(QApplication::translate("ImportMoistureData", "Export", nullptr));
        label->setText(QApplication::translate("ImportMoistureData", "X", nullptr));
        label_2->setText(QApplication::translate("ImportMoistureData", "Y", nullptr));
        Export_Radial_coordinate->setText(QApplication::translate("ImportMoistureData", "Export Radial", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ImportMoistureData: public Ui_ImportMoistureData {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_IMPORTMOISTUREDATA_H
