#ifndef DryWellDialog_H
#define DryWellDialog_H

#include <QDialog>
#include "scad_generator.h"
//#include <VTK.h>

struct GeomParams
{
    double pond_alpha_param = 86.061;
    double pond_beta_param = 2.766;
    double pond_radius = 6;
    double well_radious = 0.6;
    unsigned int nr = 10;
    unsigned int n_layers = 1;
    double well_depth = 14;
    double depth_to_gw = 25.9;
    double pond_initial_depth = 0;
    double settlingchamberdepth = 7.3;
    double side_settlingchamberdepth = 7.3;
    unsigned int n_layer_deep = 1;
    double Ks_factor = 1;
    double dr_increase_factor = 1.2;
    bool log_dr_increase = false;
    double pipe_top = -3.9; 
    double pipe_bottom = -10.1; 
};

QT_BEGIN_NAMESPACE
namespace Ui { class DryWellDialog; }
QT_END_NAMESPACE

class DryWellDialog : public QDialog
{
    Q_OBJECT

public:
    DryWellDialog(QWidget *parent = nullptr);
    ~DryWellDialog();
    GeomParams GP;

private:
    Ui::DryWellDialog *ui;
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
#endif // DryWellDialog_H
