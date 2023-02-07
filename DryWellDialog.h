#ifndef DryWellDialog_H
#define DryWellDialog_H

#include <QDialog>
#include "scad_generator.h"
//#include <VTK.h>

struct GeomParams
{
    double pond_alpha_param = 86.061;
    double pond_beta_param = 2.766;
    double pond_radius=6;
    double well_radious=1;
    unsigned int nr=10;
    unsigned int n_layers=1;
    double well_depth=22;
    double depth_to_gw=25.9;
    double pond_initial_depth = 0;
    double settlingchamberdepth = 14;
    double side_settlingchamberdepth = 5.4;
    unsigned int n_layer_deep = 1;
    double Ks_factor=1;
    double dr_increase_factor = 1.2;
    bool log_dr_increase = false;
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
