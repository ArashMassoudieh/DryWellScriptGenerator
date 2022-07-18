/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionPost_Process;
    QWidget *centralwidget;
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
    QVBoxLayout *verticalLayout_2;
    QTableWidget *tableWidget;
    QHBoxLayout *horizontalLayout_4;
    QCheckBox *checkCut;
    QPushButton *CreateCadDrawing;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *btnCreateVTKfromOutput;
    QMenuBar *menubar;
    QMenu *menuPost_Process;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(645, 644);
        actionPost_Process = new QAction(MainWindow);
        actionPost_Process->setObjectName(QString::fromUtf8("actionPost_Process"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        formLayout_2 = new QFormLayout();
        formLayout_2->setObjectName(QString::fromUtf8("formLayout_2"));
        label = new QLabel(centralwidget);
        label->setObjectName(QString::fromUtf8("label"));

        formLayout_2->setWidget(0, QFormLayout::LabelRole, label);

        Alpha = new QLineEdit(centralwidget);
        Alpha->setObjectName(QString::fromUtf8("Alpha"));

        formLayout_2->setWidget(0, QFormLayout::FieldRole, Alpha);

        label_2 = new QLabel(centralwidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        formLayout_2->setWidget(1, QFormLayout::LabelRole, label_2);

        Beta = new QLineEdit(centralwidget);
        Beta->setObjectName(QString::fromUtf8("Beta"));

        formLayout_2->setWidget(1, QFormLayout::FieldRole, Beta);

        label_3 = new QLabel(centralwidget);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        formLayout_2->setWidget(2, QFormLayout::LabelRole, label_3);

        Pond_radius = new QLineEdit(centralwidget);
        Pond_radius->setObjectName(QString::fromUtf8("Pond_radius"));

        formLayout_2->setWidget(2, QFormLayout::FieldRole, Pond_radius);

        label_4 = new QLabel(centralwidget);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        formLayout_2->setWidget(3, QFormLayout::LabelRole, label_4);

        Well_radius = new QLineEdit(centralwidget);
        Well_radius->setObjectName(QString::fromUtf8("Well_radius"));

        formLayout_2->setWidget(3, QFormLayout::FieldRole, Well_radius);

        label_5 = new QLabel(centralwidget);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        formLayout_2->setWidget(4, QFormLayout::LabelRole, label_5);

        Depth_of_Well = new QLineEdit(centralwidget);
        Depth_of_Well->setObjectName(QString::fromUtf8("Depth_of_Well"));

        formLayout_2->setWidget(4, QFormLayout::FieldRole, Depth_of_Well);

        label_6 = new QLabel(centralwidget);
        label_6->setObjectName(QString::fromUtf8("label_6"));

        formLayout_2->setWidget(5, QFormLayout::LabelRole, label_6);

        Depth_to_GW = new QLineEdit(centralwidget);
        Depth_to_GW->setObjectName(QString::fromUtf8("Depth_to_GW"));

        formLayout_2->setWidget(5, QFormLayout::FieldRole, Depth_to_GW);

        label_7 = new QLabel(centralwidget);
        label_7->setObjectName(QString::fromUtf8("label_7"));

        formLayout_2->setWidget(6, QFormLayout::LabelRole, label_7);

        Pond_ini_depth = new QLineEdit(centralwidget);
        Pond_ini_depth->setObjectName(QString::fromUtf8("Pond_ini_depth"));

        formLayout_2->setWidget(6, QFormLayout::FieldRole, Pond_ini_depth);

        label_8 = new QLabel(centralwidget);
        label_8->setObjectName(QString::fromUtf8("label_8"));

        formLayout_2->setWidget(7, QFormLayout::LabelRole, label_8);

        nr = new QSpinBox(centralwidget);
        nr->setObjectName(QString::fromUtf8("nr"));
        nr->setValue(1);

        formLayout_2->setWidget(7, QFormLayout::FieldRole, nr);

        label_9 = new QLabel(centralwidget);
        label_9->setObjectName(QString::fromUtf8("label_9"));

        formLayout_2->setWidget(8, QFormLayout::LabelRole, label_9);

        nl_shallow = new QSpinBox(centralwidget);
        nl_shallow->setObjectName(QString::fromUtf8("nl_shallow"));
        nl_shallow->setValue(1);

        formLayout_2->setWidget(8, QFormLayout::FieldRole, nl_shallow);

        label_10 = new QLabel(centralwidget);
        label_10->setObjectName(QString::fromUtf8("label_10"));

        formLayout_2->setWidget(9, QFormLayout::LabelRole, label_10);

        nl_deep = new QSpinBox(centralwidget);
        nl_deep->setObjectName(QString::fromUtf8("nl_deep"));
        nl_deep->setValue(1);

        formLayout_2->setWidget(9, QFormLayout::FieldRole, nl_deep);

        label_11 = new QLabel(centralwidget);
        label_11->setObjectName(QString::fromUtf8("label_11"));

        formLayout_2->setWidget(10, QFormLayout::LabelRole, label_11);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setSizeConstraint(QLayout::SetMaximumSize);
        filename_text = new QLineEdit(centralwidget);
        filename_text->setObjectName(QString::fromUtf8("filename_text"));

        horizontalLayout->addWidget(filename_text);

        file_push_bottom = new QPushButton(centralwidget);
        file_push_bottom->setObjectName(QString::fromUtf8("file_push_bottom"));

        horizontalLayout->addWidget(file_push_bottom);


        formLayout_2->setLayout(10, QFormLayout::FieldRole, horizontalLayout);

        Generate_Model = new QPushButton(centralwidget);
        Generate_Model->setObjectName(QString::fromUtf8("Generate_Model"));

        formLayout_2->setWidget(12, QFormLayout::FieldRole, Generate_Model);

        pushReadLayers = new QPushButton(centralwidget);
        pushReadLayers->setObjectName(QString::fromUtf8("pushReadLayers"));

        formLayout_2->setWidget(13, QFormLayout::FieldRole, pushReadLayers);

        label_12 = new QLabel(centralwidget);
        label_12->setObjectName(QString::fromUtf8("label_12"));

        formLayout_2->setWidget(11, QFormLayout::LabelRole, label_12);

        Ks_factor = new QLineEdit(centralwidget);
        Ks_factor->setObjectName(QString::fromUtf8("Ks_factor"));

        formLayout_2->setWidget(11, QFormLayout::FieldRole, Ks_factor);


        verticalLayout->addLayout(formLayout_2);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        tableWidget = new QTableWidget(centralwidget);
        tableWidget->setObjectName(QString::fromUtf8("tableWidget"));

        verticalLayout_2->addWidget(tableWidget);


        verticalLayout->addLayout(verticalLayout_2);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        checkCut = new QCheckBox(centralwidget);
        checkCut->setObjectName(QString::fromUtf8("checkCut"));

        horizontalLayout_4->addWidget(checkCut);

        CreateCadDrawing = new QPushButton(centralwidget);
        CreateCadDrawing->setObjectName(QString::fromUtf8("CreateCadDrawing"));

        horizontalLayout_4->addWidget(CreateCadDrawing);


        verticalLayout->addLayout(horizontalLayout_4);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        btnCreateVTKfromOutput = new QPushButton(centralwidget);
        btnCreateVTKfromOutput->setObjectName(QString::fromUtf8("btnCreateVTKfromOutput"));

        horizontalLayout_2->addWidget(btnCreateVTKfromOutput);


        verticalLayout->addLayout(horizontalLayout_2);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 645, 22));
        menuPost_Process = new QMenu(menubar);
        menuPost_Process->setObjectName(QString::fromUtf8("menuPost_Process"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuPost_Process->menuAction());
        menuPost_Process->addAction(actionPost_Process);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        actionPost_Process->setText(QCoreApplication::translate("MainWindow", "Post-Process", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Pond parameter ~alpha (Coefficient)", nullptr));
        Alpha->setText(QCoreApplication::translate("MainWindow", "86.061", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Pond parameter ~beta (Exponent)", nullptr));
        Beta->setText(QCoreApplication::translate("MainWindow", "2.766", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "Mean pond radius", nullptr));
        Pond_radius->setText(QCoreApplication::translate("MainWindow", "20", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "Well radius", nullptr));
        Well_radius->setText(QCoreApplication::translate("MainWindow", "0.5", nullptr));
        label_5->setText(QCoreApplication::translate("MainWindow", "Depth of Well", nullptr));
        Depth_of_Well->setText(QCoreApplication::translate("MainWindow", "4", nullptr));
        label_6->setText(QCoreApplication::translate("MainWindow", "Depth to Groundwater", nullptr));
        Depth_to_GW->setText(QCoreApplication::translate("MainWindow", "5", nullptr));
        label_7->setText(QCoreApplication::translate("MainWindow", "Pond initial water depth", nullptr));
        Pond_ini_depth->setText(QCoreApplication::translate("MainWindow", "2.766", nullptr));
        label_8->setText(QCoreApplication::translate("MainWindow", "Number of cells in radial direction", nullptr));
        label_9->setText(QCoreApplication::translate("MainWindow", "Number of shallow soil layers", nullptr));
        label_10->setText(QCoreApplication::translate("MainWindow", "Number of deep soil layers", nullptr));
        label_11->setText(QCoreApplication::translate("MainWindow", "Script File Name", nullptr));
        file_push_bottom->setText(QCoreApplication::translate("MainWindow", "...", nullptr));
        Generate_Model->setText(QCoreApplication::translate("MainWindow", "Generate Model", nullptr));
        pushReadLayers->setText(QCoreApplication::translate("MainWindow", "Read Layers Information", nullptr));
        label_12->setText(QCoreApplication::translate("MainWindow", "Ks Factor", nullptr));
        checkCut->setText(QCoreApplication::translate("MainWindow", "Cut the drawing", nullptr));
        CreateCadDrawing->setText(QCoreApplication::translate("MainWindow", "CreateCadDrawing", nullptr));
        btnCreateVTKfromOutput->setText(QCoreApplication::translate("MainWindow", "Create VTK", nullptr));
        menuPost_Process->setTitle(QCoreApplication::translate("MainWindow", "Post Process", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
