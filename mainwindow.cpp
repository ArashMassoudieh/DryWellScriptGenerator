#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "DryWellDialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->actionDrwWell, SIGNAL(triggered()), this, SLOT(on_ActionDryWell()));
    
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
