/********************************************************************************
** Form generated from reading UI file 'vtkdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.11
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_VTKDIALOG_H
#define UI_VTKDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_VTKDialog
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLineEdit *TxtOutPutFileName;
    QPushButton *ChooseOutputFile;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_3;
    QLineEdit *txtModelScriptFile;
    QPushButton *BtmModelScript;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_2;
    QComboBox *VariablecomboBox;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_4;
    QSpacerItem *horizontalSpacer;
    QLineEdit *lineEditInterval;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *VTKDialog)
    {
        if (VTKDialog->objectName().isEmpty())
            VTKDialog->setObjectName(QString::fromUtf8("VTKDialog"));
        VTKDialog->resize(595, 300);
        verticalLayout = new QVBoxLayout(VTKDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label = new QLabel(VTKDialog);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout->addWidget(label);

        TxtOutPutFileName = new QLineEdit(VTKDialog);
        TxtOutPutFileName->setObjectName(QString::fromUtf8("TxtOutPutFileName"));

        horizontalLayout->addWidget(TxtOutPutFileName);

        ChooseOutputFile = new QPushButton(VTKDialog);
        ChooseOutputFile->setObjectName(QString::fromUtf8("ChooseOutputFile"));

        horizontalLayout->addWidget(ChooseOutputFile);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        label_3 = new QLabel(VTKDialog);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        horizontalLayout_3->addWidget(label_3);

        txtModelScriptFile = new QLineEdit(VTKDialog);
        txtModelScriptFile->setObjectName(QString::fromUtf8("txtModelScriptFile"));

        horizontalLayout_3->addWidget(txtModelScriptFile);

        BtmModelScript = new QPushButton(VTKDialog);
        BtmModelScript->setObjectName(QString::fromUtf8("BtmModelScript"));

        horizontalLayout_3->addWidget(BtmModelScript);


        verticalLayout->addLayout(horizontalLayout_3);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        label_2 = new QLabel(VTKDialog);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        horizontalLayout_2->addWidget(label_2);

        VariablecomboBox = new QComboBox(VTKDialog);
        VariablecomboBox->setObjectName(QString::fromUtf8("VariablecomboBox"));

        horizontalLayout_2->addWidget(VariablecomboBox);


        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        label_4 = new QLabel(VTKDialog);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        horizontalLayout_4->addWidget(label_4);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer);

        lineEditInterval = new QLineEdit(VTKDialog);
        lineEditInterval->setObjectName(QString::fromUtf8("lineEditInterval"));

        horizontalLayout_4->addWidget(lineEditInterval);


        verticalLayout->addLayout(horizontalLayout_4);

        buttonBox = new QDialogButtonBox(VTKDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(VTKDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), VTKDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), VTKDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(VTKDialog);
    } // setupUi

    void retranslateUi(QDialog *VTKDialog)
    {
        VTKDialog->setWindowTitle(QApplication::translate("VTKDialog", "Dialog", nullptr));
        label->setText(QApplication::translate("VTKDialog", "Model Output file: ", nullptr));
        ChooseOutputFile->setText(QApplication::translate("VTKDialog", "Choose Output File", nullptr));
        label_3->setText(QApplication::translate("VTKDialog", "Model Script File: ", nullptr));
        BtmModelScript->setText(QApplication::translate("VTKDialog", "Choose Model Script File", nullptr));
        label_2->setText(QApplication::translate("VTKDialog", "Variable", nullptr));
        label_4->setText(QApplication::translate("VTKDialog", "Interval: ", nullptr));
        lineEditInterval->setText(QApplication::translate("VTKDialog", "1", nullptr));
    } // retranslateUi

};

namespace Ui {
    class VTKDialog: public Ui_VTKDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_VTKDIALOG_H
