#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "DryWellDialog.h"
#include "importmoisturedata.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->actionDrwWell, SIGNAL(triggered()), this, SLOT(on_ActionDryWell()));
    connect(ui->actionImport_Moisture_Data, SIGNAL(triggered()), this, SLOT(on_ActionImport()));


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_ActionDryWell()
{
    DryWellDialog drywelldlg(this);
    drywelldlg.exec(); 
}

void MainWindow::on_ActionImport()
{
    ImportMoistureData importdlg(this);
    importdlg.exec();
}
