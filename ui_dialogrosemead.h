/********************************************************************************
** Form generated from reading UI file 'dialogrosemead.ui'
**
** Created by: Qt User Interface Compiler version 5.12.11
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DIALOGROSEMEAD_H
#define UI_DIALOGROSEMEAD_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_DialogRoseMead
{
public:
    QVBoxLayout *verticalLayout;
    QFormLayout *formLayout;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_4;
    QLabel *label_5;
    QLabel *label_6;
    QLineEdit *lineEditBioSwaleWidth;
    QLineEdit *SystemWidth;
    QLineEdit *lineEditBioSwaleDepth;
    QHBoxLayout *horizontalLayout;
    QLineEdit *SoilFileName;
    QPushButton *pushButtonSoilProps;
    QSpinBox *spinBoxLateralCells;
    QLineEdit *lineEditLenght;
    QTableWidget *tableWidget;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *DialogRoseMead)
    {
        if (DialogRoseMead->objectName().isEmpty())
            DialogRoseMead->setObjectName(QString::fromUtf8("DialogRoseMead"));
        DialogRoseMead->resize(591, 501);
        verticalLayout = new QVBoxLayout(DialogRoseMead);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        formLayout = new QFormLayout();
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        label = new QLabel(DialogRoseMead);
        label->setObjectName(QString::fromUtf8("label"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        label_2 = new QLabel(DialogRoseMead);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_2);

        label_3 = new QLabel(DialogRoseMead);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        formLayout->setWidget(2, QFormLayout::LabelRole, label_3);

        label_4 = new QLabel(DialogRoseMead);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        formLayout->setWidget(3, QFormLayout::LabelRole, label_4);

        label_5 = new QLabel(DialogRoseMead);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        formLayout->setWidget(4, QFormLayout::LabelRole, label_5);

        label_6 = new QLabel(DialogRoseMead);
        label_6->setObjectName(QString::fromUtf8("label_6"));

        formLayout->setWidget(5, QFormLayout::LabelRole, label_6);

        lineEditBioSwaleWidth = new QLineEdit(DialogRoseMead);
        lineEditBioSwaleWidth->setObjectName(QString::fromUtf8("lineEditBioSwaleWidth"));

        formLayout->setWidget(0, QFormLayout::FieldRole, lineEditBioSwaleWidth);

        SystemWidth = new QLineEdit(DialogRoseMead);
        SystemWidth->setObjectName(QString::fromUtf8("SystemWidth"));

        formLayout->setWidget(1, QFormLayout::FieldRole, SystemWidth);

        lineEditBioSwaleDepth = new QLineEdit(DialogRoseMead);
        lineEditBioSwaleDepth->setObjectName(QString::fromUtf8("lineEditBioSwaleDepth"));

        formLayout->setWidget(2, QFormLayout::FieldRole, lineEditBioSwaleDepth);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        SoilFileName = new QLineEdit(DialogRoseMead);
        SoilFileName->setObjectName(QString::fromUtf8("SoilFileName"));

        horizontalLayout->addWidget(SoilFileName);

        pushButtonSoilProps = new QPushButton(DialogRoseMead);
        pushButtonSoilProps->setObjectName(QString::fromUtf8("pushButtonSoilProps"));

        horizontalLayout->addWidget(pushButtonSoilProps);


        formLayout->setLayout(3, QFormLayout::FieldRole, horizontalLayout);

        spinBoxLateralCells = new QSpinBox(DialogRoseMead);
        spinBoxLateralCells->setObjectName(QString::fromUtf8("spinBoxLateralCells"));
        spinBoxLateralCells->setMinimum(1);

        formLayout->setWidget(4, QFormLayout::FieldRole, spinBoxLateralCells);

        lineEditLenght = new QLineEdit(DialogRoseMead);
        lineEditLenght->setObjectName(QString::fromUtf8("lineEditLenght"));

        formLayout->setWidget(5, QFormLayout::FieldRole, lineEditLenght);


        verticalLayout->addLayout(formLayout);

        tableWidget = new QTableWidget(DialogRoseMead);
        tableWidget->setObjectName(QString::fromUtf8("tableWidget"));

        verticalLayout->addWidget(tableWidget);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(DialogRoseMead);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(DialogRoseMead);
        QObject::connect(buttonBox, SIGNAL(accepted()), DialogRoseMead, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), DialogRoseMead, SLOT(reject()));

        QMetaObject::connectSlotsByName(DialogRoseMead);
    } // setupUi

    void retranslateUi(QDialog *DialogRoseMead)
    {
        DialogRoseMead->setWindowTitle(QApplication::translate("DialogRoseMead", "Dialog", nullptr));
        label->setText(QApplication::translate("DialogRoseMead", "Bioswale Width", nullptr));
        label_2->setText(QApplication::translate("DialogRoseMead", "System Width", nullptr));
        label_3->setText(QApplication::translate("DialogRoseMead", "Bioswale Depth", nullptr));
        label_4->setText(QApplication::translate("DialogRoseMead", "Soil Properties file", nullptr));
        label_5->setText(QApplication::translate("DialogRoseMead", "Number of cells in lateral direction", nullptr));
        label_6->setText(QApplication::translate("DialogRoseMead", "Bioswale Length (m)", nullptr));
        pushButtonSoilProps->setText(QApplication::translate("DialogRoseMead", "...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class DialogRoseMead: public Ui_DialogRoseMead {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DIALOGROSEMEAD_H
