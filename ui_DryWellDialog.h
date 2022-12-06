/********************************************************************************
** Form generated from reading UI file 'DryWellDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DRYWELLDIALOG_H
#define UI_DRYWELLDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DryWellDialog
{
public:
    QAction *actionPost_Process;
    QVBoxLayout *verticalLayout_3;
    QStatusBar *statusbar;
    QMenuBar *menubar;
    QMenu *menuPost_Process;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QHBoxLayout *horizontalLayout_3;
    QVBoxLayout *verticalLayout;
    QFormLayout *formLayout_2;
    QLabel *label;
    QLineEdit *Alpha;
    QLabel *label_2;
    QLineEdit *Beta;
    QLabel *label_3;
    QLineEdit *Pond_radius;
    QLabel *label_4;
    QLineEdit *Well_radius;
    QLabel *label_5;
    QLineEdit *Depth_of_Well;
    QLabel *label_6;
    QLineEdit *Depth_to_GW;
    QLabel *label_7;
    QLineEdit *Pond_ini_depth;
    QLabel *label_8;
    QSpinBox *nr;
    QLabel *label_9;
    QSpinBox *nl_shallow;
    QLabel *label_10;
    QSpinBox *nl_deep;
    QLabel *label_11;
    QHBoxLayout *horizontalLayout;
    QLineEdit *filename_text;
    QPushButton *file_push_bottom;
    QPushButton *Generate_Model;
    QPushButton *pushReadLayers;
    QLabel *label_12;
    QLineEdit *Ks_factor;
    QGridLayout *gridLayout;
    QCheckBox *Logarithmic_Radial_Disc;
    QLabel *label_13;
    QLineEdit *txtIncreaseFactor;
    QLabel *label_14;
    QLineEdit *SettlingChamberDepth;
    QLabel *label_15;
    QLabel *label_16;
    QLineEdit *lineEditTopPipe;
    QLineEdit *lineEditBottomPipe;
    QVBoxLayout *verticalLayout_2;
    QTableWidget *tableWidget;
    QHBoxLayout *horizontalLayout_4;
    QCheckBox *checkCut;
    QPushButton *CreateCadDrawing;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *btnCreateVTKfromOutput;

    void setupUi(QDialog *DryWellDialog)
    {
        if (DryWellDialog->objectName().isEmpty())
            DryWellDialog->setObjectName(QString::fromUtf8("DryWellDialog"));
        DryWellDialog->resize(747, 715);
        actionPost_Process = new QAction(DryWellDialog);
        actionPost_Process->setObjectName(QString::fromUtf8("actionPost_Process"));
        verticalLayout_3 = new QVBoxLayout(DryWellDialog);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        statusbar = new QStatusBar(DryWellDialog);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));

        verticalLayout_3->addWidget(statusbar);

        menubar = new QMenuBar(DryWellDialog);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menuPost_Process = new QMenu(menubar);
        menuPost_Process->setObjectName(QString::fromUtf8("menuPost_Process"));

        verticalLayout_3->addWidget(menubar);

        scrollArea = new QScrollArea(DryWellDialog);
        scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 713, 760));
        horizontalLayout_3 = new QHBoxLayout(scrollAreaWidgetContents);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        formLayout_2 = new QFormLayout();
        formLayout_2->setObjectName(QString::fromUtf8("formLayout_2"));
        label = new QLabel(scrollAreaWidgetContents);
        label->setObjectName(QString::fromUtf8("label"));

        formLayout_2->setWidget(0, QFormLayout::LabelRole, label);

        Alpha = new QLineEdit(scrollAreaWidgetContents);
        Alpha->setObjectName(QString::fromUtf8("Alpha"));

        formLayout_2->setWidget(0, QFormLayout::FieldRole, Alpha);

        label_2 = new QLabel(scrollAreaWidgetContents);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        formLayout_2->setWidget(1, QFormLayout::LabelRole, label_2);

        Beta = new QLineEdit(scrollAreaWidgetContents);
        Beta->setObjectName(QString::fromUtf8("Beta"));

        formLayout_2->setWidget(1, QFormLayout::FieldRole, Beta);

        label_3 = new QLabel(scrollAreaWidgetContents);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        formLayout_2->setWidget(3, QFormLayout::LabelRole, label_3);

        Pond_radius = new QLineEdit(scrollAreaWidgetContents);
        Pond_radius->setObjectName(QString::fromUtf8("Pond_radius"));

        formLayout_2->setWidget(3, QFormLayout::FieldRole, Pond_radius);

        label_4 = new QLabel(scrollAreaWidgetContents);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        formLayout_2->setWidget(4, QFormLayout::LabelRole, label_4);

        Well_radius = new QLineEdit(scrollAreaWidgetContents);
        Well_radius->setObjectName(QString::fromUtf8("Well_radius"));

        formLayout_2->setWidget(4, QFormLayout::FieldRole, Well_radius);

        label_5 = new QLabel(scrollAreaWidgetContents);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        formLayout_2->setWidget(5, QFormLayout::LabelRole, label_5);

        Depth_of_Well = new QLineEdit(scrollAreaWidgetContents);
        Depth_of_Well->setObjectName(QString::fromUtf8("Depth_of_Well"));

        formLayout_2->setWidget(5, QFormLayout::FieldRole, Depth_of_Well);

        label_6 = new QLabel(scrollAreaWidgetContents);
        label_6->setObjectName(QString::fromUtf8("label_6"));

        formLayout_2->setWidget(7, QFormLayout::LabelRole, label_6);

        Depth_to_GW = new QLineEdit(scrollAreaWidgetContents);
        Depth_to_GW->setObjectName(QString::fromUtf8("Depth_to_GW"));

        formLayout_2->setWidget(7, QFormLayout::FieldRole, Depth_to_GW);

        label_7 = new QLabel(scrollAreaWidgetContents);
        label_7->setObjectName(QString::fromUtf8("label_7"));

        formLayout_2->setWidget(8, QFormLayout::LabelRole, label_7);

        Pond_ini_depth = new QLineEdit(scrollAreaWidgetContents);
        Pond_ini_depth->setObjectName(QString::fromUtf8("Pond_ini_depth"));

        formLayout_2->setWidget(8, QFormLayout::FieldRole, Pond_ini_depth);

        label_8 = new QLabel(scrollAreaWidgetContents);
        label_8->setObjectName(QString::fromUtf8("label_8"));

        formLayout_2->setWidget(9, QFormLayout::LabelRole, label_8);

        nr = new QSpinBox(scrollAreaWidgetContents);
        nr->setObjectName(QString::fromUtf8("nr"));
        nr->setValue(1);

        formLayout_2->setWidget(9, QFormLayout::FieldRole, nr);

        label_9 = new QLabel(scrollAreaWidgetContents);
        label_9->setObjectName(QString::fromUtf8("label_9"));

        formLayout_2->setWidget(10, QFormLayout::LabelRole, label_9);

        nl_shallow = new QSpinBox(scrollAreaWidgetContents);
        nl_shallow->setObjectName(QString::fromUtf8("nl_shallow"));
        nl_shallow->setValue(1);

        formLayout_2->setWidget(10, QFormLayout::FieldRole, nl_shallow);

        label_10 = new QLabel(scrollAreaWidgetContents);
        label_10->setObjectName(QString::fromUtf8("label_10"));

        formLayout_2->setWidget(11, QFormLayout::LabelRole, label_10);

        nl_deep = new QSpinBox(scrollAreaWidgetContents);
        nl_deep->setObjectName(QString::fromUtf8("nl_deep"));
        nl_deep->setValue(1);

        formLayout_2->setWidget(11, QFormLayout::FieldRole, nl_deep);

        label_11 = new QLabel(scrollAreaWidgetContents);
        label_11->setObjectName(QString::fromUtf8("label_11"));

        formLayout_2->setWidget(16, QFormLayout::LabelRole, label_11);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setSizeConstraint(QLayout::SetMaximumSize);
        filename_text = new QLineEdit(scrollAreaWidgetContents);
        filename_text->setObjectName(QString::fromUtf8("filename_text"));

        horizontalLayout->addWidget(filename_text);

        file_push_bottom = new QPushButton(scrollAreaWidgetContents);
        file_push_bottom->setObjectName(QString::fromUtf8("file_push_bottom"));

        horizontalLayout->addWidget(file_push_bottom);


        formLayout_2->setLayout(16, QFormLayout::FieldRole, horizontalLayout);

        Generate_Model = new QPushButton(scrollAreaWidgetContents);
        Generate_Model->setObjectName(QString::fromUtf8("Generate_Model"));

        formLayout_2->setWidget(18, QFormLayout::FieldRole, Generate_Model);

        pushReadLayers = new QPushButton(scrollAreaWidgetContents);
        pushReadLayers->setObjectName(QString::fromUtf8("pushReadLayers"));

        formLayout_2->setWidget(19, QFormLayout::FieldRole, pushReadLayers);

        label_12 = new QLabel(scrollAreaWidgetContents);
        label_12->setObjectName(QString::fromUtf8("label_12"));

        formLayout_2->setWidget(17, QFormLayout::LabelRole, label_12);

        Ks_factor = new QLineEdit(scrollAreaWidgetContents);
        Ks_factor->setObjectName(QString::fromUtf8("Ks_factor"));

        formLayout_2->setWidget(17, QFormLayout::FieldRole, Ks_factor);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));

        formLayout_2->setLayout(2, QFormLayout::LabelRole, gridLayout);

        Logarithmic_Radial_Disc = new QCheckBox(scrollAreaWidgetContents);
        Logarithmic_Radial_Disc->setObjectName(QString::fromUtf8("Logarithmic_Radial_Disc"));

        formLayout_2->setWidget(14, QFormLayout::LabelRole, Logarithmic_Radial_Disc);

        label_13 = new QLabel(scrollAreaWidgetContents);
        label_13->setObjectName(QString::fromUtf8("label_13"));

        formLayout_2->setWidget(15, QFormLayout::LabelRole, label_13);

        txtIncreaseFactor = new QLineEdit(scrollAreaWidgetContents);
        txtIncreaseFactor->setObjectName(QString::fromUtf8("txtIncreaseFactor"));

        formLayout_2->setWidget(15, QFormLayout::FieldRole, txtIncreaseFactor);

        label_14 = new QLabel(scrollAreaWidgetContents);
        label_14->setObjectName(QString::fromUtf8("label_14"));

        formLayout_2->setWidget(6, QFormLayout::LabelRole, label_14);

        SettlingChamberDepth = new QLineEdit(scrollAreaWidgetContents);
        SettlingChamberDepth->setObjectName(QString::fromUtf8("SettlingChamberDepth"));

        formLayout_2->setWidget(6, QFormLayout::FieldRole, SettlingChamberDepth);

        label_15 = new QLabel(scrollAreaWidgetContents);
        label_15->setObjectName(QString::fromUtf8("label_15"));

        formLayout_2->setWidget(12, QFormLayout::LabelRole, label_15);

        label_16 = new QLabel(scrollAreaWidgetContents);
        label_16->setObjectName(QString::fromUtf8("label_16"));

        formLayout_2->setWidget(13, QFormLayout::LabelRole, label_16);

        lineEditTopPipe = new QLineEdit(scrollAreaWidgetContents);
        lineEditTopPipe->setObjectName(QString::fromUtf8("lineEditTopPipe"));

        formLayout_2->setWidget(12, QFormLayout::FieldRole, lineEditTopPipe);

        lineEditBottomPipe = new QLineEdit(scrollAreaWidgetContents);
        lineEditBottomPipe->setObjectName(QString::fromUtf8("lineEditBottomPipe"));

        formLayout_2->setWidget(13, QFormLayout::FieldRole, lineEditBottomPipe);


        verticalLayout->addLayout(formLayout_2);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        tableWidget = new QTableWidget(scrollAreaWidgetContents);
        tableWidget->setObjectName(QString::fromUtf8("tableWidget"));

        verticalLayout_2->addWidget(tableWidget);


        verticalLayout->addLayout(verticalLayout_2);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        checkCut = new QCheckBox(scrollAreaWidgetContents);
        checkCut->setObjectName(QString::fromUtf8("checkCut"));

        horizontalLayout_4->addWidget(checkCut);

        CreateCadDrawing = new QPushButton(scrollAreaWidgetContents);
        CreateCadDrawing->setObjectName(QString::fromUtf8("CreateCadDrawing"));

        horizontalLayout_4->addWidget(CreateCadDrawing);


        verticalLayout->addLayout(horizontalLayout_4);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        btnCreateVTKfromOutput = new QPushButton(scrollAreaWidgetContents);
        btnCreateVTKfromOutput->setObjectName(QString::fromUtf8("btnCreateVTKfromOutput"));

        horizontalLayout_2->addWidget(btnCreateVTKfromOutput);


        verticalLayout->addLayout(horizontalLayout_2);


        horizontalLayout_3->addLayout(verticalLayout);

        scrollArea->setWidget(scrollAreaWidgetContents);

        verticalLayout_3->addWidget(scrollArea);


        menubar->addAction(menuPost_Process->menuAction());
        menuPost_Process->addAction(actionPost_Process);

        retranslateUi(DryWellDialog);

        QMetaObject::connectSlotsByName(DryWellDialog);
    } // setupUi

    void retranslateUi(QDialog *DryWellDialog)
    {
        DryWellDialog->setWindowTitle(QCoreApplication::translate("DryWellDialog", "DryWell", nullptr));
        actionPost_Process->setText(QCoreApplication::translate("DryWellDialog", "Post-Process", nullptr));
        menuPost_Process->setTitle(QCoreApplication::translate("DryWellDialog", "Post Process", nullptr));
        label->setText(QCoreApplication::translate("DryWellDialog", "Pond parameter ~alpha (Coefficient)", nullptr));
        Alpha->setText(QCoreApplication::translate("DryWellDialog", "86.061", nullptr));
        label_2->setText(QCoreApplication::translate("DryWellDialog", "Pond parameter ~beta (Exponent)", nullptr));
        Beta->setText(QCoreApplication::translate("DryWellDialog", "2.766", nullptr));
        label_3->setText(QCoreApplication::translate("DryWellDialog", "Mean pond radius", nullptr));
        Pond_radius->setText(QCoreApplication::translate("DryWellDialog", "20", nullptr));
        label_4->setText(QCoreApplication::translate("DryWellDialog", "Well radius", nullptr));
        Well_radius->setText(QCoreApplication::translate("DryWellDialog", "0.5", nullptr));
        label_5->setText(QCoreApplication::translate("DryWellDialog", "Depth of Well", nullptr));
        Depth_of_Well->setText(QCoreApplication::translate("DryWellDialog", "4", nullptr));
        label_6->setText(QCoreApplication::translate("DryWellDialog", "Depth to Groundwater", nullptr));
        Depth_to_GW->setText(QCoreApplication::translate("DryWellDialog", "5", nullptr));
        label_7->setText(QCoreApplication::translate("DryWellDialog", "Pond initial water depth", nullptr));
        Pond_ini_depth->setText(QCoreApplication::translate("DryWellDialog", "2.766", nullptr));
        label_8->setText(QCoreApplication::translate("DryWellDialog", "Number of cells in radial direction", nullptr));
        label_9->setText(QCoreApplication::translate("DryWellDialog", "Number of shallow soil layers", nullptr));
        label_10->setText(QCoreApplication::translate("DryWellDialog", "Number of deep soil layers", nullptr));
        label_11->setText(QCoreApplication::translate("DryWellDialog", "Script File Name", nullptr));
        file_push_bottom->setText(QCoreApplication::translate("DryWellDialog", "...", nullptr));
        Generate_Model->setText(QCoreApplication::translate("DryWellDialog", "Generate Model", nullptr));
        pushReadLayers->setText(QCoreApplication::translate("DryWellDialog", "Read Layers Information", nullptr));
        label_12->setText(QCoreApplication::translate("DryWellDialog", "Ks Factor", nullptr));
        Logarithmic_Radial_Disc->setText(QCoreApplication::translate("DryWellDialog", "Logarithmic Radial discretization", nullptr));
        label_13->setText(QCoreApplication::translate("DryWellDialog", "Increase factor", nullptr));
        txtIncreaseFactor->setText(QCoreApplication::translate("DryWellDialog", "1.2", nullptr));
        label_14->setText(QCoreApplication::translate("DryWellDialog", "Depth of settling chamber", nullptr));
        label_15->setText(QCoreApplication::translate("DryWellDialog", "Top elevation of overflow pipe", nullptr));
        label_16->setText(QCoreApplication::translate("DryWellDialog", "Bottom elevation of overflow pipe", nullptr));
        checkCut->setText(QCoreApplication::translate("DryWellDialog", "Cut the drawing", nullptr));
        CreateCadDrawing->setText(QCoreApplication::translate("DryWellDialog", "CreateCadDrawing", nullptr));
        btnCreateVTKfromOutput->setText(QCoreApplication::translate("DryWellDialog", "Create VTK", nullptr));
    } // retranslateUi

};

namespace Ui {
    class DryWellDialog: public Ui_DryWellDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DRYWELLDIALOG_H
