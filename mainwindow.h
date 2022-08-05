#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "scad_generator.h"
//#include <VTK.h>

struct GeomParams
{
    double pond_alpha_param = 86.061;
    double pond_beta_param = 2.766;
    double pond_radius=26;
    double well_radious=1;
    unsigned int nr=1;
    unsigned int n_layers=1;
    double well_depth=41.95;
    double depth_to_gw=70.28;
    double pond_initial_depth = 0;
    unsigned int n_layer_deep = 1;
    double Ks_factor=1;
    double dr_increase_factor = 1.2;
    bool log_dr_increase = false;
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    GeomParams GP;

private:
    Ui::MainWindow *ui;
    QList<QStringList> LayerData;
    bool uniform = true;
    SCAD_Generator scad;
    bool freecad = true;
private slots:
    void On_Generate_Model();
    void On_File_Select();
    void On_ReadLayer_Info();
    void On_CreateCADDrawing();
    void On_CreateVTK();

};
#endif // MAINWINDOW_H
