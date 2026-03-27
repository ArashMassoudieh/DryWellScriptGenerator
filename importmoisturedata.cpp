// NOTE: This file is part of the DryWellSuite/OpenHydroQual codebase.
#include "importmoisturedata.h"
#include "ui_importmoisturedata.h"
#include "QFileDialog"
#include "QMessageBox"
#include "BTC.h"
#include "BTCSet.h"
#include <qdebug.h>
#include <QComboBox>
#include <QSignalBlocker>

ImportMoistureData::ImportMoistureData(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImportMoistureData)
{
    // Wire all dialog actions to explicit slots (legacy SIGNAL/SLOT style retained for compatibility).
    ui->setupUi(this);
    connect(ui->ChooseFolder,SIGNAL(clicked()),this,SLOT(on_choosefolder()));
    connect(ui->pushButtonExport ,SIGNAL(clicked()),this,SLOT(on_exporttoParaview()));
    connect(ui->Export_Radial_coordinate ,SIGNAL(clicked()),this,SLOT(on_exportRadialtoParaview()));
    connect(ui->pushButtonExportTimeSeries, SIGNAL(clicked()),this, SLOT(on_export_timeseries()));
    connect(ui->ExportProfiles, SIGNAL(clicked()),this, SLOT(on_export_profiles()));

    ui->modeCombo->clear();
    ui->modeCombo->addItem("Radial", static_cast<int>(_mode::radial));
    ui->modeCombo->addItem("Rectangular 3D", static_cast<int>(_mode::rectangular));
    ui->modeCombo->addItem("Planar 2D", static_cast<int>(_mode::planar2d));
    connect(ui->modeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int idx) {
        const auto selectedMode = static_cast<_mode>(ui->modeCombo->itemData(idx).toInt());
        SetMode(selectedMode);
    });
    SetMode(mode);
}

void ImportMoistureData::SetMode(_mode Mode)
{
    // Current UI only customizes the 2D export label; import/export math branches on `mode`.
    mode = Mode;
    if (ui && ui->modeCombo) {
        const QSignalBlocker blocker(ui->modeCombo);
        const int comboIndex = ui->modeCombo->findData(static_cast<int>(mode));
        if (comboIndex >= 0) ui->modeCombo->setCurrentIndex(comboIndex);
    }
    if (mode == _mode::planar2d) {
        ui->Export_Radial_coordinate->setText("Export 2D mapped (planar)");
    } else {
        ui->Export_Radial_coordinate->setText("Export 2D mapped");
    }
}

ImportMoistureData::~ImportMoistureData()
{
    delete ui;
}

void ImportMoistureData::on_choosefolder()
{
    // Input directory is expected to contain one CSV per snapshot/time step.
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                 "/home",
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);

    QDir directory(dir);
    snapshots.clear();
    QStringList csvs = directory.entryList(QStringList() << "*.csv" << "*.csv",QDir::Files);
    if (mode==_mode::radial)
    {   foreach(QString filename, csvs) {
            // Radial mode: parser constructor resolves expected EC/MC format.
            CPointSet<CPoint3d> points((dir+"/"+filename).toStdString(),ECvsMC::EC);
            snapshots.push_back(points);
        }
    }
    else if (mode==_mode::rectangular || mode==_mode::planar2d)
    {
        vector<int> xyzval;
        if (mode == _mode::planar2d) {
            // Planar 2D mode CSV column mapping: x,y,value (z reuses y as flat placeholder).
            // Expected columns: [x, y, value].
            xyzval.push_back(1); xyzval.push_back(2); xyzval.push_back(2); xyzval.push_back(3);
        } else {
            // Rectangular mode CSV column mapping: x,y,z,value columns.
            xyzval.push_back(1); xyzval.push_back(2); xyzval.push_back(3); xyzval.push_back(5);
        }
        ifstream SchedFile(ScheduleFileName.toStdString());
        foreach(QString filename, csvs) {
            // Attach schedule timestamp (`hrs`) from companion schedule file.
            vector<string> schedline = aquiutils::getline(SchedFile);
            CPointSet<CPoint3d> points((dir+"/"+filename).toStdString(),xyzval);
            if (!schedline.empty()) {
                points.hrs = aquiutils::atof(schedline[0]);
            }
            snapshots.push_back(points);
        }
    }

    QMessageBox msgBox;
    msgBox.setText("Loading data is finished!");
    msgBox.exec();

}


void ImportMoistureData::on_exporttoParaview()
{
    // Export both point-cloud and meshed representation for each snapshot.
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
    // Build a consistent radial center from first snapshot bounds.
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
            // Smooth to regular 2D grid before writing VTP.
            CPointSet<CPoint> cylendical_points_kernel_smooth = cylendical_points.MapToGrid(2,5,span);

            cylendical_points_kernel_smooth.WriteToVtp2D(dir.toStdString()+"/MC_"+aquiutils::numbertostring(i+1)+".vtp");
        }
        if (mode == _mode::rectangular || mode == _mode::planar2d)
        {
            // Rectangular mode exports both standard and "long" 2D mappings.
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
    // Export a single-point moisture time-series with kernel smoothing in cylindrical space.
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
        // Legacy radial mode uses index-based hourly increments.
        if (mode==_mode::radial)
            out.append(initial_time + double(i)/24.0 , cylendical_points.KernelSmoothValue(point,span));
        else
            // Rectangular mode uses imported schedule timestamps.
            out.append(snapshots[i].hrs , cylendical_points.KernelSmoothValue(point,span));

    }
    out.writefile(fileName.toStdString());
    QMessageBox msgBox;
    msgBox.setText("Export completed!");
    msgBox.exec();
}

void ImportMoistureData::on_export_profiles()
{

    if (mode == _mode::rectangular || mode == _mode::planar2d)
    {   double initial_time = 44438.3993;
        // Profile export supported for rectangular and planar 2D modes.
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
