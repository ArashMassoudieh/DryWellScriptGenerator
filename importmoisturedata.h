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
    explicit ImportMoistureData(QWidget *parent = nullptr);
    ~ImportMoistureData();
    vector<CPointSet<CPoint3d>> snapshots;
    enum class _mode {radial, rectangular} mode = _mode::radial;
    void SetMode(_mode Mode);
    QString ScheduleFileName;
private:
    Ui::ImportMoistureData *ui;

public slots:
    void on_choosefolder();
    void on_exporttoParaview();
    void on_exportRadialtoParaview();
    void on_export_timeseries();
    void on_export_profiles();
};

#endif // IMPORTMOISTUREDATA_H
