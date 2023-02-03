#include "importmoisturedata.h"
#include "ui_importmoisturedata.h"
#include "QFileDialog"
#include "QMessageBox"
#include "BTC.h"

ImportMoistureData::ImportMoistureData(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImportMoistureData)
{
    ui->setupUi(this);
    connect(ui->ChooseFolder,SIGNAL(clicked()),this,SLOT(on_choosefolder()));
    connect(ui->pushButtonExport ,SIGNAL(clicked()),this,SLOT(on_exporttoParaview()));
    connect(ui->Export_Radial_coordinate ,SIGNAL(clicked()),this,SLOT(on_exportRadialtoParaview()));
    connect(ui->pushButtonExportTimeSeries, SIGNAL(clicked()),this, SLOT(on_export_timeseries()));
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
    vector<double> limits = {-10000,10000,-10000,10000,-10000,10000};
    for (int i=0; i<snapshots.size(); i++)
    {
        snapshots[i].WriteToPointsVtp(dir.toStdString()+"/MC_"+aquiutils::numbertostring(i+1)+".vtp",limits);
    }
    QMessageBox msgBox;
    msgBox.setText("Export completed!");
    msgBox.exec();
}

void ImportMoistureData::on_exportRadialtoParaview()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Save Directory"),
                                                 "/home",
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);

    QDir directory(dir);
    CPointSet<CPoint3d> range3d = snapshots[0].Range();
    CPointSet<CPoint> mapped_to_cylendrical = snapshots[0].MapToCylindrical((range3d.x(0)+range3d.x(1))/2.0,(range3d.y(0)+range3d.y(1))/2.0);
    CPointSet<CPoint> range2d = mapped_to_cylendrical.Range();
    vector<double> span = {0.5, 0.5};

    for (int i=0; i<snapshots.size(); i++)
    {
        CPointSet<CPoint> cylendical_points = snapshots[i].MapToCylindrical((range3d.x(0)+range3d.x(1))/2.0,(range3d.y(0)+range3d.y(1))/2.0);
        CPointSet<CPoint> cylendical_points_kernel_smooth = cylendical_points.MapToGrid(2,5,span);

        cylendical_points_kernel_smooth.WriteToVtp2D(dir.toStdString()+"/MC_"+aquiutils::numbertostring(i+1)+".vtp");
    }
    QMessageBox msgBox;
    msgBox.setText("Export completed!");
    msgBox.exec();
}

void ImportMoistureData::on_export_timeseries()
{
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save"), "",
            tr("script files (*.csv)"));

    QDir directory(dir);
    CPointSet<CPoint3d> range3d = snapshots[0].Range();
    CPointSet<CPoint> mapped_to_cylendrical = snapshots[0].MapToCylindrical((range3d.x(0)+range3d.x(1))/2.0,(range3d.y(0)+range3d.y(1))/2.0);
    CPointSet<CPoint> range2d = mapped_to_cylendrical.Range();
    vector<double> span = {0.5, 0.5};
    CTimeSeries<double> out;
    CPoint point;
    point.setx(ui->center_X->text().toDouble());
    point.sety(ui->center_y->text().toDouble());
    for (int i=0; i<snapshots.size(); i++)
    {
        CPointSet<CPoint> cylendical_points = snapshots[i].MapToCylindrical((range3d.x(0)+range3d.x(1))/2.0,(range3d.y(0)+range3d.y(1))/2.0);
        out.append(cylendical_points.KernelSmoothValue(point,span));
    }
    out.writefile(fileName.toStdString());
    QMessageBox msgBox;
    msgBox.setText("Export completed!");
    msgBox.exec();
}
