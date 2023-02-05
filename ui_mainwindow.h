/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.12.11
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionDrwWell;
    QAction *actionBioswale;
    QAction *actionImport_Moisture_Data;
    QWidget *centralwidget;
    QMenuBar *menubar;
    QMenu *menuCreate_Model;
    QMenu *menuImport;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(800, 600);
        actionDrwWell = new QAction(MainWindow);
        actionDrwWell->setObjectName(QString::fromUtf8("actionDrwWell"));
        actionBioswale = new QAction(MainWindow);
        actionBioswale->setObjectName(QString::fromUtf8("actionBioswale"));
        actionImport_Moisture_Data = new QAction(MainWindow);
        actionImport_Moisture_Data->setObjectName(QString::fromUtf8("actionImport_Moisture_Data"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 800, 20));
        menuCreate_Model = new QMenu(menubar);
        menuCreate_Model->setObjectName(QString::fromUtf8("menuCreate_Model"));
        menuImport = new QMenu(menubar);
        menuImport->setObjectName(QString::fromUtf8("menuImport"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuCreate_Model->menuAction());
        menubar->addAction(menuImport->menuAction());
        menuCreate_Model->addAction(actionDrwWell);
        menuCreate_Model->addAction(actionBioswale);
        menuImport->addAction(actionImport_Moisture_Data);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", nullptr));
        actionDrwWell->setText(QApplication::translate("MainWindow", "DryWell", nullptr));
        actionBioswale->setText(QApplication::translate("MainWindow", "Bioswale", nullptr));
        actionImport_Moisture_Data->setText(QApplication::translate("MainWindow", "Import Moisture Data", nullptr));
        menuCreate_Model->setTitle(QApplication::translate("MainWindow", "Create Model", nullptr));
        menuImport->setTitle(QApplication::translate("MainWindow", "Import", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
