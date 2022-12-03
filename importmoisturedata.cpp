#include "importmoisturedata.h"
#include "ui_importmoisturedata.h"
#include "QFileDialog"
#include "QMessageBox"

ImportMoistureData::ImportMoistureData(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImportMoistureData)
{
    ui->setupUi(this);
    connect(ui->ChooseFolder,SIGNAL(clicked()),this,SLOT(on_choosefolder()));
    connect(ui->pushButtonExport ,SIGNAL(clicked()),this,SLOT(on_exporttoParaview()));
}

ImportMoistureData::~ImportMoistureData()
{
    delete ui;
}

void ImportMoistureData::on_choosefolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                 "/home",
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);

    QDir directory(dir);

    QStringList csvs = directory.entryList(QStringList() << "*.csv" << "*.csv",QDir::Files);
    foreach(QString filename, csvs) {
        CPointSet<CPoint3d> points((dir+"/"+filename).toStdString());
        snapshots.push_back(points);
    }

    QMessageBox msgBox;
    msgBox.setText("Loading data is finished!");
    msgBox.exec();

}


void ImportMoistureData::on_exporttoParaview()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Save Directory"),
                                                 "/home",
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);

    QDir directory(dir);
    vector<double> limits = {-0.2,0.2,-10000,10000,-10000,10000};
    for (int i=0; i<snapshots.size(); i++)
    {
        snapshots[i].WriteToPointsVtp(dir.toStdString()+"/MC_"+aquiutils::numbertostring(i+1)+".vtp",limits);
    }
}
