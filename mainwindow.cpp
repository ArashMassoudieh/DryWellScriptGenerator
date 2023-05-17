#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "DryWellDialog.h"
#include "dialogrosemead.h"
#include "importmoisturedata.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->actionDrwWell, SIGNAL(triggered()), this, SLOT(on_ActionDryWell()));
    connect(ui->actionBioswale,SIGNAL(triggered()), this, SLOT(on_ActionBioSwale()));
    connect(ui->actionImport_Moisture_Data, SIGNAL(triggered()), this, SLOT(on_ActionImport()));
    connect(ui->actionImport_Moisture_Data_Rosemead, SIGNAL(triggered()), this, SLOT(on_ActionImport_Rosemead()));


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

void MainWindow::on_ActionBioSwale()
{
    DialogRoseMead bioswaleldlg(this);
    bioswaleldlg.exec();
}


void MainWindow::on_ActionImport()
{
    ImportMoistureData importdlg(this);
    importdlg.exec();
}

void MainWindow::on_ActionImport_Rosemead()
{
    ImportMoistureData importdlg(this);
    importdlg.SetMode(ImportMoistureData::_mode::rectangular);
    importdlg.ScheduleFileName = "/home/arash/Dropbox/LA Project/Rosemead_Data/Rosemead_export/TimeStamps.txt";
    importdlg.exec();
}
