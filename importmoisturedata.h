// NOTE: This file is part of the DryWellSuite/OpenHydroQual codebase.
#ifndef IMPORTMOISTUREDATA_H
#define IMPORTMOISTUREDATA_H

#include <QDialog>
#include <cpointset.h>

namespace Ui {
class ImportMoistureData;
}

class ImportMoistureData : public QDialog
{
    Q_OBJECT

public:
    /// Dialog used to import moisture snapshots (CSV) and export derived datasets/VTK artifacts.
    explicit ImportMoistureData(QWidget *parent = nullptr);
    ~ImportMoistureData();
    /// Parsed per-time-step point clouds loaded from selected directory.
    vector<CPointSet<CPoint3d>> snapshots;
    /// Import mode controls expected CSV shape and downstream export mapping logic.
    enum class _mode {radial, rectangular, planar2d} mode = _mode::radial;
    /// Set import/export mode (radial vs rectangular vs planar 2D).
    void SetMode(_mode Mode);
    /// Optional schedule file used by rectangular mode to attach timestamps.
    QString ScheduleFileName;
private:
    Ui::ImportMoistureData *ui;

public slots:
    /// Select input directory and load all CSV snapshots.
    void on_choosefolder();
    /// Export loaded snapshots to 3D VTK point + mesh files for ParaView.
    void on_exporttoParaview();
    /// Export radial/2D mapped views for ParaView.
    void on_exportRadialtoParaview();
    /// Export a smoothed moisture time-series at selected probe coordinates.
    void on_export_timeseries();
    /// Export depth profiles (rectangular mode) across loaded snapshots.
    void on_export_profiles();
};

#endif // IMPORTMOISTUREDATA_H
