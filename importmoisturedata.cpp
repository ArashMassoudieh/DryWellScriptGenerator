#include "importmoisturedata.h"
#include "ui_importmoisturedata.h"
#include "QFileDialog"
#include "QMessageBox"
#include "BTC.h"
#include "BTCSet.h"
#include <qdebug.h>

ImportMoistureData::ImportMoistureData(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImportMoistureData)
{
    ui->setupUi(this);
    connect(ui->ChooseFolder,SIGNAL(clicked()),this,SLOT(on_choosefolder()));
    connect(ui->pushButtonExport ,SIGNAL(clicked()),this,SLOT(on_exporttoParaview()));
    connect(ui->Export_Radial_coordinate ,SIGNAL(clicked()),this,SLOT(on_exportRadialtoParaview()));
    connect(ui->pushButtonExportTimeSeries, SIGNAL(clicked()),this, SLOT(on_export_timeseries()));
    connect(ui->ExportProfiles, SIGNAL(clicked()),this, SLOT(on_export_profiles()));
}

void ImportMoistureData::SetMode(_mode Mode)
{
    mode = Mode;
    ui->Export_Radial_coordinate->setText("Export 2D mapped");
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
    snapshots.clear();
    QStringList csvs = directory.entryList(QStringList() << "*.csv" << "*.csv",QDir::Files);
    if (mode==_mode::radial)
    {   foreach(QString filename, csvs) {
            CPointSet<CPoint3d> points((dir+"/"+filename).toStdString(),ECvsMC::EC);
            snapshots.push_back(points);
        }
    }
    else if (mode==_mode::rectangular)
    {
        vector<int> xyzval;
        xyzval.push_back(1); xyzval.push_back(2); xyzval.push_back(3); xyzval.push_back(5);
        ifstream SchedFile(ScheduleFileName.toStdString());
        foreach(QString filename, csvs) {
            vector<string> schedline = aquiutils::getline(SchedFile);
            CPointSet<CPoint3d> points((dir+"/"+filename).toStdString(),xyzval);
            points.hrs = aquiutils::atof(schedline[0]);
            snapshots.push_back(points);
        }
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
        snapshots[i].WriteToVtp3D(dir.toStdString()+"/Mesh_"+aquiutils::numbertostring(i+1)+".vtu");
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
    vector<double> span = {0.5, 0.2};

    for (int i=0; i<snapshots.size(); i++)
    {
        if (mode == _mode::radial)
        {   CPointSet<CPoint> cylendical_points = snapshots[i].MapToCylindrical((range3d.x(0)+range3d.x(1))/2.0,(range3d.y(0)+range3d.y(1))/2.0);
            CPointSet<CPoint> cylendical_points_kernel_smooth = cylendical_points.MapToGrid(2,5,span);

            cylendical_points_kernel_smooth.WriteToVtp2D(dir.toStdString()+"/MC_"+aquiutils::numbertostring(i+1)+".vtp");
        }
        if (mode == _mode::rectangular)
        {
            CPointSet<CPoint> cylendical_points = snapshots[i].MapTo2DV();
            CPointSet<CPoint> cylendical_points_kernel_smooth = cylendical_points.MapToGrid(1,1,span,true);
            cylendical_points_kernel_smooth.WriteToVtp2D(dir.toStdString()+"/MC_"+aquiutils::numbertostring(i+1)+".vtp");
            CPointSet<CPoint> cylendical_points_long = snapshots[i].MapTo2DV(true);
            CPointSet<CPoint> cylendical_points_kernel_smooth_long = cylendical_points_long.MapToGrid(1,1,span,true);
            cylendical_points_kernel_smooth_long.WriteToVtp2D(dir.toStdString()+"/MC_long"+aquiutils::numbertostring(i+1)+".vtp");
        }
    }
    QMessageBox msgBox;
    msgBox.setText("Export completed!");
    msgBox.exec();
}

void ImportMoistureData::on_export_timeseries()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save"), "",
                                                    tr("csv files (*.csv)"));

    QDir directory(dir);
    CPointSet<CPoint3d> range3d = snapshots[0].Range();
    CPointSet<CPoint> mapped_to_cylendrical = snapshots[0].MapToCylindrical((range3d.x(0)+range3d.x(1))/2.0,(range3d.y(0)+range3d.y(1))/2.0);
    CPointSet<CPoint> range2d = mapped_to_cylendrical.Range();
    vector<double> span = {0.5, 0.5};
    CTimeSeries<double> out;
    CPoint point;
    point.setx(ui->center_X->text().toDouble());
    point.sety(ui->center_y->text().toDouble());
    double initial_time;
    if (mode==_mode::radial)
        initial_time = 44436.5;
    else
        initial_time = 0;
    for (int i=0; i<snapshots.size(); i++)
    {
        CPointSet<CPoint> cylendical_points = snapshots[i].MapToCylindrical((range3d.x(0)+range3d.x(1))/2.0,(range3d.y(0)+range3d.y(1))/2.0);
        //out.append(initial_time + cylendical_points.hrs/24.0 , cylendical_points.KernelSmoothValue(point,span));
        if (mode==_mode::radial)
            out.append(initial_time + double(i)/24.0 , cylendical_points.KernelSmoothValue(point,span));
        else
            out.append(snapshots[i].hrs , cylendical_points.KernelSmoothValue(point,span));

    }
    out.writefile(fileName.toStdString());
    QMessageBox msgBox;
    msgBox.setText("Export completed!");
    msgBox.exec();
}

void ImportMoistureData::on_export_profiles()
{

    if (mode == _mode::rectangular)
    {   double initial_time = 44438.3993;
        vector<double> span = {0.5, 0.2};
        CTimeSeriesSet<double> profile_data;
        for (int i=0; i<snapshots.size(); i++)
        {   CPointSet<CPoint> cylendical_points = snapshots[i].MapTo2DV();
            CPointSet<CPoint> cylendical_points_kernel_smooth = cylendical_points.MapToGrid(0.25,0.25,span,false);
            CTimeSeries<double> profile;
            for (int j=0; j<cylendical_points_kernel_smooth.size(); j++)
            {
                profile.append(cylendical_points_kernel_smooth[j].y(),cylendical_points_kernel_smooth[j].Value(0));
            }
            profile_data.append(profile,QString::number(initial_time+double(i)/24.0,'g',7).toStdString());

        }
        QString fileName = QFileDialog::getSaveFileName(this,
                tr("Save"), "",
                tr("csv files (*.csv)"));
        profile_data.writetofile(fileName.toStdString());
    }
}
