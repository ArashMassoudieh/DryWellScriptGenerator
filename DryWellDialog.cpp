#include "DryWellDialog.h"
#include "ui_DryWellDialog.h"
#include <QFile>
#include <QFileDialog>
#include <math.h>
#include <QDebug>
#include <QTextStream>
#include "vtkdialog.h"
#include "QMessageBox"

DryWellDialog::DryWellDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DryWellDialog)
{
    ui->setupUi(this);
    connect(ui->file_push_bottom, SIGNAL(clicked()),this, SLOT(On_File_Select()));
    connect(ui->Generate_Model, SIGNAL(clicked()),this, SLOT(On_Generate_Model()));
    connect(ui->pushReadLayers, SIGNAL(clicked()),this,SLOT(On_ReadLayer_Info()));
    connect(ui->CreateCadDrawing, SIGNAL(clicked()),this,SLOT(On_CreateCADDrawing()));
    connect(ui->btnCreateVTKfromOutput, SIGNAL(clicked()),this,SLOT(On_CreateVTK()));
    //getting the values into text boxes
    ui->Depth_to_GW->setText(QString::number(GP.depth_to_gw));
    ui->nl_deep->setValue(GP.n_layer_deep);
    ui->nl_shallow->setValue(GP.n_layers);
    ui->nr->setValue(GP.nr);
    ui->Alpha->setText(QString::number(GP.pond_alpha_param));
    ui->Beta->setText(QString::number(GP.pond_beta_param));
    ui->Pond_ini_depth->setText(QString::number(GP.pond_initial_depth));
    ui->SettlingChamberDepth->setText(QString::number(GP.settlingchamberdepth));
    ui->Pond_radius->setText(QString::number(GP.pond_radius));
    ui->Depth_of_Well->setText(QString::number(GP.well_depth));
    ui->Well_radius->setText(QString::number(GP.well_radious));
    ui->Ks_factor->setText(QString::number(GP.Ks_factor));

}

DryWellDialog::~DryWellDialog()
{
    delete ui;
}

void DryWellDialog::On_Generate_Model()
{

    GP.depth_to_gw = ui->Depth_to_GW->text().toDouble();
    GP.n_layer_deep = ui->nl_deep->text().toInt();
    GP.n_layers = ui->nl_shallow->text().toInt();
    GP.nr = ui->nr->text().toInt();
    GP.pond_alpha_param = ui->Alpha->text().toDouble();
    GP.pond_beta_param = ui->Beta->text().toDouble();
    GP.pond_initial_depth = ui->Pond_ini_depth->text().toDouble();
    GP.pond_radius = ui->Pond_radius->text().toDouble();
    GP.well_depth = ui->Depth_of_Well->text().toDouble();
    GP.well_radious = ui->Well_radius->text().toDouble();
    GP.Ks_factor = ui->Ks_factor->text().toDouble();
    GP.log_dr_increase = ui->Logarithmic_Radial_Disc->checkState();
    GP.dr_increase_factor = ui->txtIncreaseFactor->text().toDouble();
    GP.settlingchamberdepth = ui->SettlingChamberDepth->text().toDouble();
    QString path = "C:/Users/12022/Dropbox/Drywell Project/Combined_Model/Longterm_fixed_areas/";

    if (ui->filename_text->text() == "") return;
    QFile file(ui->filename_text->text());
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.write("loadtemplate; filename = /home/arash/Projects/QAquifolium/bin/Debug/../../resources/main_components.json\n");
    file.write("addtemplate; filename = /home/arash/Projects/QAquifolium/bin/Release/../../resources/Pond_Plugin.json\n");
    file.write("addtemplate; filename = /home/arash/Projects/QAquifolium/bin/Release/../../resources/unsaturated_soil.json\n");
    file.write("addtemplate; filename = /home/arash/Projects/QAquifolium/bin/Release/../../resources/Well.json\n");
    file.write("addtemplate; filename = /home/arash/Projects/QAquifolium/bin/Release/../../resources/Sewer_system.json\n");
    file.write("addtemplate; filename = /home/arash/Projects/QAquifolium/bin/Release/../../resources/soil_evapotranspiration_models.json\n");
    file.write("addtemplate; filename = /home/arash/Projects/QAquifolium/bin/Release/../../resources/evapotranspiration_models.json\n");
    file.write("addtemplate; filename = C:/Program Files (x86)/OpenHydroQual/bin/bin/../../resources/pipe_pump_tank.json\n");

    file.write("setvalue; object=system, quantity=simulation_start_time, value=44435\n");
    file.write("setvalue; object=system, quantity=simulation_end_time, value=44437\n");
    file.write("setvalue; object=system, quantity=shakescalered, value=0.75\n");
    file.write("setvalue; object=system, quantity=shakescale, value=0.05\n");
    file.write("setvalue; object=system, quantity=pmute, value=0.02\n");
    file.write("setvalue; object=system, quantity=ngen, value=40\n");
    file.write("setvalue; object=system, quantity=pcross, value=1\n");
    file.write("setvalue; object=system, quantity=outputfile, value=GA_output.txt\n");
    file.write("setvalue; object=system, quantity=maxpop, value=40\n");
    file.write("setvalue; object=system, quantity=write_solution_details, value=No\n");
    file.write("setvalue; object=system, quantity=nr_tolerance, value=0.001\n");
    file.write("setvalue; object=system, quantity=nr_timestep_reduction_factor_fail, value=0.2\n");
    file.write("setvalue; object=system, quantity=nr_timestep_reduction_factor, value=0.75\n");
    file.write("setvalue; object=system, quantity=n_threads, value=4\n");
    file.write("setvalue; object=system, quantity=minimum_timestep, value=1e-06\n");
    file.write("setvalue; object=system, quantity=initial_time_step, value=0.01\n");
    file.write("setvalue; object=system, quantity=c_n_weight, value=1\n");
    
    file.write("create block;type=Pond,inflow=D:/CUA/Dropbox/LA Project/Data/Inflow_corrected_Arash.txt,_width=200,Evapotranspiration=,Precipitation=,bottom_elevation=0[m],Storage=0[m~^3],name=Infiltration_Pond,alpha=86.061,beta=2.766,x=-5971,y=-249,_height=200\n");

#ifndef Brett
    file.write("create parameter;type=Parameter,prior_distribution=normal,value=0,name=dep_storage,high=0.05,low=0.001\n");
    file.write("create parameter;type=Parameter,prior_distribution=,value=0.2,name=Manning_Catchment,high=1,low=0.01\n");
    file.write("create parameter;type=Parameter,prior_distribution=,value=0.1,name=Manning_Sewer,high=1,low=0.01\n");
#endif
    file.write("create parameter;type=Parameter,value=1,prior_distribution=normal,name=Ks_scale,low=0.1,high=10\n");
    double x_base = 0;
    double y_base = 0;
    double dr;

    if (!GP.log_dr_increase)
        dr = (GP.pond_radius - GP.well_radious) / double(GP.nr);
    else
        dr = (GP.pond_radius - GP.well_radious) / (pow(GP.dr_increase_factor, GP.nr) - 1) * (GP.dr_increase_factor - 1);
    double dy = (GP.well_depth) / double(GP.n_layers);
    double dy_deep = (GP.depth_to_gw - GP.well_depth) / double(GP.n_layer_deep);
    // Create pond
    //file.write(QString("// ***** Pond ***** //\n").toUtf8());
    //file.write(QString("create block;type=Pond,y=" + QString::number(y_base) + ",name=Infiltration_Pond,x=" + QString::number(x_base) + ",bottom_elevation=0,alpha=" + QString::number(GP.pond_alpha_param) + ",_width=200,_height=200,beta=" + QString::number(GP.pond_beta_param) + ",Storage=" + QString::number(GP.pond_alpha_param * pow(GP.pond_initial_depth, GP.pond_beta_param)) + ",Precipitation=\n").toUtf8());

    // Create soil blocks adjacent to well
    file.write(QString("// ***** Soil blocks adjacent to the well ***** //\n").toUtf8());
    unsigned int lowest_shallow_layer = GP.n_layers;
    unsigned int sedimentation_layer_id = 0; 
    double lowest_shallow_depth = 0;
    if (uniform)
    {
        for (unsigned int r = 0; r < GP.nr; r++)
        {
            for (unsigned int layer = 0; layer < GP.n_layers; layer++)
            {
                double x = x_base + 200 + r * 300;
                double y = y_base + 300 + layer * 300;
                double r_in;
                double r_out;
                if (!GP.log_dr_increase)
                {
                    r_in = r * dr + GP.well_radious;
                    r_out = (r + 1) * dr + GP.well_radious;
                }
                else
                {
                    r_in = GP.well_radious + dr * (pow(GP.dr_increase_factor, r) - 1) / (GP.dr_increase_factor - 1);
                    r_out = GP.well_radious + dr * (pow(GP.dr_increase_factor, r + 1) - 1) / (GP.dr_increase_factor - 1);
                }
                double area = 3.1415 * (pow(r_out, 2) - pow(r_in, 2));
                file.write(QString("create block;type=Soil,theta_sat=0.4,theta_res=0.05,specific_storage=0.01,x=" + QString::number(x) + ",Evapotranspiration=,n=1.41,y=" + QString::number(y) + ",area=" + QString::number(area) + ",theta=0.2,K_sat_original=1,_width=200,alpha=1,name=Soil (" + QString::number(layer + 1) + "$" + QString::number(r + 1) + "),_height=100,bottom_elevation=" + QString::number(-dy * (layer + 1)) + ",depth=" + QString::number(dy) + ",actual_x=" + QString::number(0.5 * (r_in + r_out)) + ",actual_y=" + QString::number(-dy * (layer + 0.5)) + "\n").toUtf8());
            }
        }
    }
    else
    {
        for (unsigned int r = 0; r < GP.nr; r++)
        {
            double bottom_elev = 0;
            bool reached_bottom = false;
            for (unsigned int layer = 1; layer < LayerData.count(); layer++)
            {
                bottom_elev -= LayerData[layer][0].toDouble();
                if (!reached_bottom)
                {
                    double depth = LayerData[layer][0].toDouble();
                    if (bottom_elev < -GP.well_depth)
                    {
                        lowest_shallow_depth = bottom_elev + depth;
                        depth = GP.well_depth - (-bottom_elev - depth);
                        bottom_elev = -GP.well_depth;
                        reached_bottom = true;
                        lowest_shallow_layer = layer;
                    }
                    if (bottom_elev < -GP.settlingchamberdepth && sedimentation_layer_id==0)
                    {
                        sedimentation_layer_id = layer; 
                    }
                    QString Evap_object = "";
                    if (layer == 1)
                    {
                        //Evap_object = "Evapotranspiration_Soil";
                    }
                    double x = x_base + 200 + r * 300;
                    double y = y_base + 300 + layer * 300;
                    double r_in;
                    double r_out;
                    if (!GP.log_dr_increase)
                    {
                        r_in = r * dr + GP.well_radious;
                        r_out = (r + 1) * dr + GP.well_radious;
                    }
                    else
                    {
                        r_in = GP.well_radious + dr * (pow(GP.dr_increase_factor, r) - 1) / (GP.dr_increase_factor - 1);
                        r_out = GP.well_radious + dr * (pow(GP.dr_increase_factor, r + 1) - 1) / (GP.dr_increase_factor - 1);
                    }
                    double area = 3.1415 * (pow(r_out, 2) - pow(r_in, 2));
                    file.write(QString("create block;type=Soil,theta_sat=" + LayerData[layer][4] + ",theta_res=" + LayerData[layer][5] + ",specific_storage=0.01,x=" + QString::number(x) + ",Evapotranspiration=" + Evap_object + ",n=" + LayerData[layer][3] + ",y=" + QString::number(y) + ",area=" + QString::number(area) + ",theta=0.2,K_sat_original=" + QString::number(LayerData[layer][1].toDouble() * GP.Ks_factor) + ",_width=200,alpha=" + LayerData[layer][2] + ",name=Soil (" + QString::number(layer) + "$" + QString::number(r + 1) + "),_height=100,bottom_elevation=" + QString::number(bottom_elev) + ",depth=" + QString::number(depth) + ",actual_x=" + QString::number(0.5 * (r_in + r_out)) + ",actual_y=" + QString::number(bottom_elev + depth / 2) + "\n").toUtf8());
                }
            }
        }

    }

    // Create deep soil layers
    file.write(QString("// ***** Soil Blocks not adjacent to the well ***** //\n").toUtf8());
    if (uniform)
    {
        for (unsigned int r = 0; r < GP.nr; r++)
            for (unsigned int layer = 0; layer < GP.n_layer_deep; layer++)
            {
                double x = x_base + 200 + r * 300;
                double y = y_base + 300 + layer * 300 + GP.n_layers * 300;
                double r_in;
                double r_out;
                if (!GP.log_dr_increase)
                {
                    r_in = r * dr + GP.well_radious;
                    r_out = (r + 1) * dr + GP.well_radious;
                }
                else
                {
                    r_in = GP.well_radious + dr * (pow(GP.dr_increase_factor, r) - 1) / (GP.dr_increase_factor - 1);
                    r_out = GP.well_radious + dr * (pow(GP.dr_increase_factor, r + 1) - 1) / (GP.dr_increase_factor - 1);
                }
                double area = 3.1415 * (pow(r_out, 2) - pow(r_in, 2));
                file.write(QString("create block;type=Soil,theta_sat=0.4,theta_res=0.05,specific_storage=0.01,x=" + QString::number(x) + ",Evapotranspiration=,n=1.41,y=" + QString::number(y) + ",area=" + QString::number(area) + ",theta=0.2,K_sat_original=" + QString::number(1) + ",_width=200,alpha=1,name=Soil_deep (" + QString::number(layer + 1) + "$" + QString::number(r + 1) + "),_height=100,bottom_elevation=" + QString::number(-dy_deep * (layer + 1) - GP.well_depth) + ",depth=" + QString::number(dy_deep) + ",actual_x=" + QString::number(0.5 * (r_in + r_out)) + ",actual_y=" + QString::number(-dy_deep * (layer + 0.5) - GP.well_depth) + "\n").toUtf8());
            }
        file.write(QString("// ***** Soil Blocks underneath the well ***** //\n").toUtf8());
        for (unsigned int layer = 0; layer < GP.n_layer_deep; layer++)
        {
            double y = y_base + 300 + layer * 300 + GP.n_layers * 300;
            double r_in = 0;
            double r_out = GP.well_radious;
            double area = 3.1415 * (pow(r_out, 2) - pow(r_in, 2));
            file.write(QString("create block;type=Soil,theta_sat=0.4,theta_res=0.05,specific_storage=0.01,x=" + QString::number(0) + ",Evapotranspiration=,n=1.41,y=" + QString::number(y) + ",area=" + QString::number(area) + ",theta=0.2,K_sat_original=" + QString::number(1) + ",_width=100,alpha=1,name=Soil_deep (" + QString::number(layer + 1) + "$" + QString::number(0) + "),_height=100,bottom_elevation=" + QString::number(-dy_deep * (layer + 1) - GP.well_depth) + ",depth=" + QString::number(dy_deep) + ",actual_x=" + QString::number(0.5 * (r_in + r_out)) + ",actual_y=" + QString::number(-dy_deep * (layer + 0.5) - GP.well_depth) + "\n").toUtf8());
        }
    }
    else
    {
        for (unsigned int r = 0; r < GP.nr; r++)
        {
            double bottom_elevation = lowest_shallow_depth;
            for (unsigned int layer = lowest_shallow_layer; layer < LayerData.count(); layer++)
            {
                double x = x_base + 200 + r * 300;
                double y = y_base + 300 + (layer + 1) * 300;
                double r_in;
                double r_out;
                if (!GP.log_dr_increase)
                {
                    r_in = r * dr + GP.well_radious;
                    r_out = (r + 1) * dr + GP.well_radious;
                }
                else
                {
                    r_in = GP.well_radious + dr * (pow(GP.dr_increase_factor, r) - 1) / (GP.dr_increase_factor - 1);
                    r_out = GP.well_radious + dr * (pow(GP.dr_increase_factor, r + 1) - 1) / (GP.dr_increase_factor - 1);
                }
                double area = 3.1415 * (pow(r_out, 2) - pow(r_in, 2));
                double depth = LayerData[layer][0].toDouble();
                bottom_elevation -= depth;
                if (layer == lowest_shallow_layer)
                {
                    depth = depth - (GP.well_depth + lowest_shallow_depth);
                }
                file.write(QString("create block;type=Soil,theta_sat=" + LayerData[layer][4] + ",theta_res=" + LayerData[layer][5] + ",specific_storage=0.01,x=" + QString::number(x) + ",Evapotranspiration=,n=" + LayerData[layer][3] + ",y=" + QString::number(y) + ",area=" + QString::number(area) + ",theta=0.2,K_sat_original=" + QString::number(LayerData[layer][1].toDouble() * GP.Ks_factor) + ",_width=200,alpha=" + LayerData[layer][2] + ",name=Soil_deep (" + QString::number(layer) + "$" + QString::number(r + 1) + "),_height=100,bottom_elevation=" + QString::number(bottom_elevation) + ",depth=" + QString::number(depth) + ",actual_x=" + QString::number(0.5 * (r_in + r_out)) + ",actual_y=" + QString::number(bottom_elevation + depth / 2) + "\n").toUtf8());
            }
        }
        double bottom_elevation = lowest_shallow_depth;
        file.write(QString("// ***** Soil Blocks underneath the well ***** //\n").toUtf8());
        for (unsigned int layer = lowest_shallow_layer; layer < LayerData.count(); layer++)
        {
            double depth = LayerData[layer][0].toDouble();
            bottom_elevation -= depth;
            if (layer == lowest_shallow_layer)
            {
                depth = depth - (GP.well_depth + lowest_shallow_depth);
            }
            double y = y_base + 300 + (layer + 1) * 300;
            double r_in = 0;
            double r_out = GP.well_radious;
            double area = 3.1415 * (pow(r_out, 2) - pow(r_in, 2));
            file.write(QString("create block;type=Soil,theta_sat=" + LayerData[layer][4] + ",theta_res=" + LayerData[layer][5] + ",specific_storage=0.01,x=" + QString::number(0) + ",Evapotranspiration=,n=" + LayerData[layer][3] + ",y=" + QString::number(y) + ",area=" + QString::number(area) + ",theta=0.2,K_sat_original=" + QString::number(LayerData[layer][1].toDouble() * GP.Ks_factor) + ",_width=100,alpha=1,name=Soil_deep (" + QString::number(layer) + "$" + QString::number(0) + "),_height=100,bottom_elevation=" + QString::number(bottom_elevation) + ",depth=" + QString::number(depth) + ",actual_x=" + QString::number(0.5 * (r_in + r_out)) + ",actual_y=" + QString::number(bottom_elevation + depth / 2) + "\n").toUtf8());
        }

    }

    // Create Well
    file.write(QString("// ***** Dry well well ***** //\n").toUtf8());
    if (uniform)
    {
        file.write(QString("create block;type=Well_aggregate,y=" + QString::number(y_base + (GP.n_layers - 1) * 300 / 2) + ",name=DryWell,x=" + QString::number(x_base) + ",depth=0[m],bottom_elevation=" + QString::number(-GP.well_depth) + ",diameter=" + QString::number(GP.well_radious * 2) + " ,porosity = 0.4,_width=100,_height=" + QString::number(300 * (GP.n_layers - 1)/2 + 100) + "\n").toUtf8());
        file.write(QString("create block; type = Well, bottom_elevation = " + QString::number(GP.settlingchamberdepth) + "[m], name = Sedimentation_Chamber, _height = " + QString::number(300 * (GP.n_layers - 1) / 2 + 100) + ", diameter = " + QString::number(GP.well_radious * 2) + "[m], _width = 100, depth = 0, x = " + QString::number(x_base) + ", y = " + QString::number(y_base + (GP.n_layers - 1) * 300) + "\n").toUtf8());
    }
    else
    {
        file.write(QString("create block;type=Well_aggregate,y=" + QString::number(y_base + (LayerData.count()) * 300 / 2) + ",name=DryWell,x=" + QString::number(x_base) + ",depth=" + QString::number(GP.well_depth + GP.pond_initial_depth - GP.settlingchamberdepth) + ",bottom_elevation=" + QString::number(-GP.well_depth) + ",porosity = 0.4,diameter=" + QString::number(GP.well_radious * 2) + ",_width=100,_height=" + QString::number(300 * (lowest_shallow_layer - 1)/2 + 100) + "\n").toUtf8());
        file.write(QString("create block; type = Well, bottom_elevation = " + QString::number(GP.settlingchamberdepth) + "[m], name = Sedimentation_Chamber, _height = " + QString::number(300 * (lowest_shallow_layer - 1) / 2 + 100) + ", diameter = " + QString::number(GP.well_radious * 2) + "[m], _width = 100, depth = 0, x = " + QString::number(x_base) + ", y = " + QString::number(y_base + (GP.n_layers - 1) * 300) + "\n").toUtf8());
        file.write("create block;type=Well,name=Side_Settling_Chamber,bottom_elevation=-7.3[m],_width=100,depth=0,inflow=,diameter=2[m],x=-4505,y=130,_height=4450\n");
    }


    

// Vertical soil connectors
    file.write(QString("// ***** Vertical shallow soil connectors ***** //\n").toUtf8());
    if (uniform)
    {
        for (unsigned int r = 0; r<GP.nr; r++)
            for (unsigned int layer=0; layer<GP.n_layers-1; layer++)
                file.write(QString("create link;from=Soil ("+QString::number(layer+1)+"$"+QString::number(r+1)+"),to=Soil ("+QString::number(layer+2)+"$"+QString::number(r+1)+"),type=soil_to_soil_link,name=Soil ("+QString::number(layer+1)+"$"+QString::number(r+1)+") - Soil ("+QString::number(layer+2)+"$"+QString::number(r+1)+")\n").toUtf8());
    }
    else
    {
        for (unsigned int r = 0; r<GP.nr; r++)
            for (unsigned int layer=1; layer<lowest_shallow_layer; layer++)
                file.write(QString("create link;from=Soil ("+QString::number(layer)+"$"+QString::number(r+1)+"),to=Soil ("+QString::number(layer+1)+"$"+QString::number(r+1)+"),type=soil_to_soil_link,name=Soil ("+QString::number(layer)+"$"+QString::number(r+1)+") - Soil ("+QString::number(layer+1)+"$"+QString::number(r+1)+")\n").toUtf8());
    }

// Horizontal soil conncetors
    file.write(QString("// ***** Horizontal shallow soil connectors ***** //\n").toUtf8());
    if (uniform)
    {
        for (unsigned int r = 0; r<GP.nr-1; r++)
            for (unsigned int layer=0; layer<GP.n_layers; layer++)
            {   double radius;
                if (!GP.log_dr_increase)
                {   radius = (r+1)*dr + GP.well_radious;
                    file.write(QString("create link;from=Soil ("+QString::number(layer+1)+"$"+QString::number(r+1)+"),to=Soil ("+QString::number(layer+1)+"$"+QString::number(r+2)+"),type=soil_to_soil_H_link,name=Soil ("+QString::number(layer+1)+"$"+QString::number(r+1)+") - Soil ("+QString::number(layer+1)+"$"+QString::number(r+2)+"),length="+QString::number(dr)+",area="+QString::number(3.1415*radius*dy)+"\n").toUtf8());
                }
                else
                {   radius = GP.well_radious + dr*(pow(GP.dr_increase_factor,r+1)-1)/(GP.dr_increase_factor-1);
                    double length = 0.5*dr*(pow(GP.dr_increase_factor,r+1)+pow(GP.dr_increase_factor,r));
                    file.write(QString("create link;from=Soil ("+QString::number(layer+1)+"$"+QString::number(r+1)+"),to=Soil ("+QString::number(layer+1)+"$"+QString::number(r+2)+"),type=soil_to_soil_H_link,name=Soil ("+QString::number(layer+1)+"$"+QString::number(r+1)+") - Soil ("+QString::number(layer+1)+"$"+QString::number(r+2)+"),length="+QString::number(length)+",area="+QString::number(3.1415*radius*dy)+"\n").toUtf8());
                }

            }
    }
    else
    {
        for (unsigned int r = 0; r<GP.nr-1; r++)
            for (unsigned int layer=1; layer<lowest_shallow_layer+1; layer++)
            {
                double radius;
                double length;
                if (!GP.log_dr_increase)
                {   radius = (r+1)*dr + GP.well_radious;
                    length = dr;
                }
                else
                {   radius = GP.well_radious + dr*(pow(GP.dr_increase_factor-1,r+1))/(GP.dr_increase_factor-1);
                    length = 0.5*dr*(pow(GP.dr_increase_factor,r+1)+pow(GP.dr_increase_factor,r));
                }
                file.write(QString("create link;from=Soil ("+QString::number(layer)+"$"+QString::number(r+1)+"),to=Soil ("+QString::number(layer)+"$"+QString::number(r+2)+"),type=soil_to_soil_H_link,name=Soil ("+QString::number(layer+1)+"$"+QString::number(r+1)+") - Soil ("+QString::number(layer)+"$"+QString::number(r+2)+"),length="+QString::number(length)+",area="+QString::number(3.1415*radius*dy)+"\n").toUtf8());
            }
    }


// Soil to Deep soil connectors
    file.write(QString("// ***** Vertical soil to deep soil connectors ***** //\n").toUtf8());
    if (uniform)
        for (unsigned int r = 0; r<GP.nr; r++)
            file.write(QString("create link;from=Soil ("+QString::number(lowest_shallow_layer)+"$"+QString::number(r+1)+"),to=Soil_deep (1$"+QString::number(r+1)+"),type=soil_to_soil_link,name=Soil ("+QString::number(GP.n_layers)+"$"+QString::number(r+1)+") - Soil_deep (1$"+QString::number(r+1)+")\n").toUtf8());
    else
        for (unsigned int r = 0; r<GP.nr; r++)
            file.write(QString("create link;from=Soil ("+QString::number(lowest_shallow_layer)+"$"+QString::number(r+1)+"),to=Soil_deep ("+QString::number(lowest_shallow_layer)+"$"+QString::number(r+1)+"),type=soil_to_soil_link,name=Soil ("+QString::number(lowest_shallow_layer)+"$"+QString::number(r+1)+") - Soil_deep ("+QString::number(lowest_shallow_layer)+"$"+QString::number(r+1)+")\n").toUtf8());

// Horizontal Deep soil connectors
    file.write(QString("// ***** Horizontal deep soil connectors ***** //\n").toUtf8());
    for (unsigned int r = 0; r<GP.nr-1; r++)
    {
        double radius;
        double length;
        if (!GP.log_dr_increase)
        {   radius = (r+1)*dr + GP.well_radious;
            length = dr;
        }
        else
        {   radius = GP.well_radious + dr*(pow(GP.dr_increase_factor,r+1)-1)/(GP.dr_increase_factor-1);
            length = 0.5*dr*(pow(GP.dr_increase_factor,r+1)+pow(GP.dr_increase_factor,r));
        }
        if (uniform)
            for (unsigned int layer=0; layer<GP.n_layer_deep; layer++)
            {
                file.write(QString("create link;from=Soil_deep ("+QString::number(layer+1)+"$"+QString::number(r+1)+"),to=Soil_deep ("+QString::number(layer+1)+"$"+QString::number(r+2)+"),type=soil_to_soil_H_link,name=Soil_deep ("+QString::number(layer+1)+"$"+QString::number(r+1)+") - Soil_deep ("+QString::number(layer+1)+"$"+QString::number(r+2)+"),length="+QString::number(length)+",area="+QString::number(3.1415*radius*dy_deep)+"\n").toUtf8());
            }
        else
            for (unsigned int layer=lowest_shallow_layer; layer<LayerData.count(); layer++)
            {
                double depth = LayerData[layer][0].toDouble();
                if (layer==lowest_shallow_layer)
                {
                    depth = depth - (GP.well_depth + lowest_shallow_depth);
                }
                file.write(QString("create link;from=Soil_deep ("+QString::number(layer)+"$"+QString::number(r+1)+"),to=Soil_deep ("+QString::number(layer)+"$"+QString::number(r+2)+"),type=soil_to_soil_H_link,name=Soil_deep ("+QString::number(layer)+"$"+QString::number(r+1)+") - Soil_deep ("+QString::number(layer)+"$"+QString::number(r+2)+"),length="+QString::number(length)+",area="+QString::number(3.1415*radius*depth)+"\n").toUtf8());
            }

    }
    if (uniform)
        for (unsigned int layer=0; layer<GP.n_layer_deep; layer++)
            file.write(QString("create link;from=Soil_deep ("+QString::number(layer+1)+"$0),to=Soil_deep ("+QString::number(layer+1)+"$1),type=soil_to_soil_H_link,name=Soil_deep ("+QString::number(layer+1)+"$0) - Soil_deep ("+QString::number(layer+1)+"$1),length="+QString::number(GP.well_radious/2+dr/2)+",area="+QString::number(3.1415*dy_deep*GP.well_radious)+"\n").toUtf8());
    else
        for (unsigned int layer=lowest_shallow_layer; layer<LayerData.count(); layer++)
        {
            double depth = LayerData[layer][0].toDouble();
            if (layer==lowest_shallow_layer)
            {
                depth = depth - (GP.well_depth + lowest_shallow_depth);
            }
            file.write(QString("create link;from=Soil_deep ("+QString::number(layer)+"$0),to=Soil_deep ("+QString::number(layer)+"$1),type=soil_to_soil_H_link,name=Soil_deep ("+QString::number(layer)+"$0) - Soil_deep ("+QString::number(layer)+"$1),length="+QString::number(GP.well_radious/2+dr/2)+",area="+QString::number(3.1415*depth*GP.well_radious)+"\n").toUtf8());
        }

// Vertical Deep soil connectors
    file.write(QString("// ***** Vertical deep soil connectors ***** //\n").toUtf8());
    for (unsigned int r = 0; r<GP.nr; r++)
    {   if (uniform)
            for (unsigned int layer=0; layer<GP.n_layer_deep-1; layer++)
                file.write(QString("create link;from=Soil_deep ("+QString::number(layer+1)+"$"+QString::number(r+1)+"),to=Soil_deep ("+QString::number(layer+2)+"$"+QString::number(r+1)+"),type=soil_to_soil_link,name=Soil_deep ("+QString::number(layer+1)+"$"+QString::number(r+1)+") - Soil_deep ("+QString::number(layer+2)+"$"+QString::number(r+1)+")\n").toUtf8());
        else
            for (unsigned int layer=lowest_shallow_layer; layer<LayerData.count()-1; layer++)
                file.write(QString("create link;from=Soil_deep ("+QString::number(layer)+"$"+QString::number(r+1)+"),to=Soil_deep ("+QString::number(layer+1)+"$"+QString::number(r+1)+"),type=soil_to_soil_link,name=Soil_deep ("+QString::number(layer)+"$"+QString::number(r+1)+") - Soil_deep ("+QString::number(layer+1)+"$"+QString::number(r+1)+")\n").toUtf8());
    }

    if (uniform)
        for (unsigned int layer=0; layer<GP.n_layer_deep-1; layer++)
            file.write(QString("create link;from=Soil_deep ("+QString::number(layer+1)+"$"+QString::number(0)+"),to=Soil_deep ("+QString::number(layer+2)+"$"+QString::number(0)+"),type=soil_to_soil_link,name=Soil_deep ("+QString::number(layer+1)+"$"+QString::number(0)+") - Soil_deep ("+QString::number(layer+2)+"$"+QString::number(0)+")\n").toUtf8());
    else
        for (unsigned int layer=lowest_shallow_layer; layer<LayerData.count()-1; layer++)
            file.write(QString("create link;from=Soil_deep ("+QString::number(layer)+"$"+QString::number(0)+"),to=Soil_deep ("+QString::number(layer+1)+"$"+QString::number(0)+"),type=soil_to_soil_link,name=Soil_deep ("+QString::number(layer)+"$"+QString::number(0)+") - Soil_deep ("+QString::number(layer+1)+"$"+QString::number(0)+")\n").toUtf8());

// Connect pond to soil
//    file.write(QString("// ***** pond to soil connectors ***** //\n").toUtf8());
//    for (unsigned int r = 0; r<GP.nr; r++)
//        file.write(QString("create link;from=Infiltration_Pond,to=Soil ("+QString::number(1)+"$"+QString::number(r+1)+"),type=surfacewater_to_soil_link,name=Pond - Soil ("+QString::number(1)+"$"+QString::number(r+1)+")\n").toUtf8());


// Connect Well to soil H
    file.write(QString("// ***** Horizontal drywell to soil connectors ***** //\n").toUtf8());
    if (uniform)
    {   for (unsigned int layer=0; layer<GP.n_layers; layer++)
            file.write(QString("create link;from=DryWell,to=Soil ("+QString::number(layer+1)+"$1),type=Well2soil horizontal link,name=DryWell - Soil ("+QString::number(layer+1)+"$1),length="+QString::number(dr/2)+"\n").toUtf8());
    }
    else
    {
        for (unsigned int layer=sedimentation_layer_id; layer<lowest_shallow_layer+1; layer++)
            file.write(QString("create link;from=DryWell,to=Soil ("+QString::number(layer)+"$1),type=Well2soil horizontal link,name=DryWell - Soil ("+QString::number(layer)+"$1),length="+QString::number(dr/2)+"\n").toUtf8());
    }

// Connect Well to soil V
    file.write(QString("// ***** Vertical drywell to soil connectors ***** //\n").toUtf8());
    if (uniform)
        file.write(QString("create link;from=DryWell,to=Soil_deep ("+QString::number(1)+"$0),type=Well2soil vertical link,name=DryWell - Soil_deep ("+QString::number(1)+"$0)\n").toUtf8());
    else
        file.write(QString("create link;from=DryWell,to=Soil_deep ("+QString::number(lowest_shallow_layer)+"$0),type=Well2soil vertical link,name=DryWell - Soil_deep ("+QString::number(lowest_shallow_layer)+"$0)\n").toUtf8());

// Connect Pond to Well
    file.write(QString("// ***** Pond to well connector ***** //\n").toUtf8());
    file.write(QString("create link;from=Infiltration_Pond,to=Side_Settling_Chamber,type=Surface water to well,name=Infiltration_Pond - Side_Settling_Chamber,length=3,ManningCoeff=100\n").toUtf8());

// GW
    file.write(QString("// ***** GW block ***** //\n").toUtf8());
    double y = y_base + 300 + GP.n_layers*300 + GP.n_layer_deep*300;
    if (!uniform)
        y = y_base + 300 + GP.n_layers*300 + (LayerData.count()+1)*300;
    file.write(QString("create block;type=fixed_head,y="+QString::number(y)+",name=GW,x="+QString::number(x_base)+",head="+QString::number(-GP.depth_to_gw)+",_width="+QString::number(200+GP.nr*300)+",_height=100,Storage=100000\n").toUtf8());

// Connect Soil to GW
    file.write(QString("// ***** Soil to GW connectors ***** //\n").toUtf8());
    if (uniform)
        for (unsigned int r = 0; r<GP.nr; r++)
            file.write(QString("create link;from=Soil_deep ("+QString::number(GP.n_layer_deep)+"$"+QString::number(1+r)+"),to=GW,type=soil_to_fixedhead_link,name=Soil_deep ("+QString::number(GP.n_layer_deep)+"$"+QString::number(1+r)+") - GW\n").toUtf8());
    else
        for (unsigned int r = 0; r<GP.nr; r++)
            file.write(QString("create link;from=Soil_deep ("+QString::number(LayerData.count()-1)+"$"+QString::number(1+r)+"),to=GW,type=soil_to_fixedhead_link,name=Soil_deep ("+QString::number(LayerData.count()-1)+"$"+QString::number(1+r)+") - GW\n").toUtf8());

    if (uniform)
        file.write(QString("create link;from=Soil_deep ("+QString::number(GP.n_layer_deep)+"$0),to=GW,type=soil_to_fixedhead_link,name=Soil_deep ("+QString::number(GP.n_layer_deep)+"$0) - GW\n").toUtf8());
    else
        file.write(QString("create link;from=Soil_deep ("+QString::number(LayerData.count()-1)+"$0),to=GW,type=soil_to_fixedhead_link,name=Soil_deep ("+QString::number(LayerData.count()-1)+"$0) - GW\n").toUtf8());
#ifdef ModelCatchments
//Catchments
    file.write("create block;type=Catchment,_width=500,_height=350,Width=57.3816[m],depth=0[m],Precipitation=Ave_04082020,loss_coefficient=1[1/day],x=-6500,Slope=0.015,area=9688.46[m~^2],Evapotranspiration=,depression_storage=0[m],elevation=0[m],ManningCoeff=0.02,name=5,y=-1100\n");
    file.write("create block;type=Catchment,_width=500,_height=350,Width=59.6158[m],depth=0[m],Precipitation=Ave_04082020,loss_coefficient=1[1/day],x=-5500,Slope=0.015,area=33338.14[m~^2],Evapotranspiration=,depression_storage=0[m],elevation=0[m],ManningCoeff=0.02,name=4,y=-1100\n");
    file.write("create block;type=Catchment,_width=500,_height=350,Width=90.0593[m],depth=0[m],Precipitation=Ave_04082020,loss_coefficient=1[1/day],x=-4500,Slope=0.015,area=12921.25[m~^2],Evapotranspiration=,depression_storage=0[m],elevation=0[m],ManningCoeff=0.02,name=3,y=-1100\n");
    file.write("create block;type=Catchment,_width=500,_height=350,Width=90.4677[m],depth=0[m],Precipitation=Ave_04082020,loss_coefficient=1[1/day],x=-3500,Slope=0.015,area=3306.80[m~^2],Evapotranspiration=,depression_storage=0[m],elevation=0[m],ManningCoeff=0.02,name=2,y=-1100\n");
    file.write("create block;type=Catchment,_width=500,_height=350,Width=74.7339[m],depth=0[m],Precipitation=Ave_04082020,loss_coefficient=1[1/day],x=-2500,Slope=0.015,area=2610.37[m~^2],Evapotranspiration=,depression_storage=0[m],elevation=0[m],ManningCoeff=0.02,name=1,y=-1100\n");
    file.write("create block;type=Catchment,_width=500,_height=350,Width=89.8246[m],depth=0[m],Precipitation=Ave_04082020,loss_coefficient=1[1/day],x=-1500,Slope=0.015,area=4986.94[m~^2],Evapotranspiration=,depression_storage=0[m],elevation=0[m],ManningCoeff=0.02,name=6,y=-15\n");
    file.write("create block;type=Catchment,_width=500,_height=350,Width=90.2208[m],depth=0[m],Precipitation=Ave_04082020,loss_coefficient=1[1/day],x=-2500,Slope=0.015,area=5796.54[m~^2],Evapotranspiration=,depression_storage=0[m],elevation=0[m],ManningCoeff=0.02,name=7,y=-15\n");
    file.write("create block;type=Catchment,_width=500,_height=350,Width=90.2208[m],depth=0[m],Precipitation=Ave_04082020,loss_coefficient=1[1/day],x=-3500,Slope=0.015,area=6646.88[m~^2],Evapotranspiration=,depression_storage=0[m],elevation=0[m],ManningCoeff=0.02,name=8,y=-15\n");
    file.write("create block;type=Catchment,_width=500,_height=350,Width=90.2208[m],depth=0[m],Precipitation=Ave_04082020,loss_coefficient=1[1/day],x=-4500,Slope=0.015,area=11174.21[m~^2],Evapotranspiration=,depression_storage=0[m],elevation=0[m],ManningCoeff=0.02,name=9,y=-15\n");
    file.write("create block;type=Catchment,_width=500,_height=350,Width=54.5287[m],depth=0[m],Precipitation=Ave_04082020,loss_coefficient=1[1/day],x=-5500,Slope=0.015,area=2898.58[m~^2],Evapotranspiration=,depression_storage=0[m],elevation=0[m],ManningCoeff=0.02,name=10,y=-15\n");
    file.write("create block;type=Catchment,_width=500,_height=350,Width=49.5239[m],depth=0[m],Precipitation=Ave_04082020,loss_coefficient=1[1/day],x=-6500,Slope=0.015,area=7275.99[m~^2],Evapotranspiration=,depression_storage=0[m],elevation=0[m],ManningCoeff=0.02,name=11,y=-15\n");
    file.write("create block;type=Catchment,_width=500,_height=350,Width=36[m],depth=0[m],Precipitation=Ave_04082020,loss_coefficient=1[1/day],x=-7500,Slope=0.015,area=8380.68[m~^2],Evapotranspiration=,depression_storage=0[m],elevation=0[m],ManningCoeff=0.02,name=12,y=-15\n");
    file.write("create block;type=Catchment,_width=500,_height=350,Width=80[m],depth=0[m],Precipitation=Ave_04082020,loss_coefficient=1[1/day],x=-8500,Slope=0.015,area=4177.39[m~^2],Evapotranspiration=,depression_storage=0[m],elevation=0[m],ManningCoeff=0.02,name=13,y=-15\n");


//Sewer_Channels
    file.write("create block;type=Sewerchannelsegment,_height=200,depth=0[m],diameter=0.9144[m],bottom_elevation=7.51[m],inflow=,length=57.38[m],x=-6000,ManningCoeff=0.01,name=SC5,y=-600,_width=200\n");
    file.write("create block;type=Sewerchannelsegment,_height=200,depth=0[m],diameter=0.9144[m],bottom_elevation=6.85[m],inflow=,length=59.62[m],x=-5000,ManningCoeff=0.01,name=SC4,y=-600,_width=200\n");
    file.write("create block;type=Sewerchannelsegment,_height=200,depth=0[m],diameter=0.9144[m],bottom_elevation=5.93[m],inflow=,length=90.06[m],x=-4000,ManningCoeff=0.01,name=SC3,y=-600,_width=200\n");
    file.write("create block;type=Sewerchannelsegment,_height=200,depth=0[m],diameter=0.9144[m],bottom_elevation=4.51[m],inflow=,length=90.47[m],x=-2995,ManningCoeff=0.01,name=SC2,y=-600,_width=200\n");
    file.write("create block;type=Sewerchannelsegment,_height=200,depth=0[m],diameter=0.9144[m],bottom_elevation=2.24[m],inflow=,length=116.23[m],x=-1983,ManningCoeff=0.01,name=SC1,y=-608,_width=200\n");
    file.write("create block;type=Sewerchannelsegment,_height=200,depth=0[m],diameter=0.9144[m],bottom_elevation=10.67[m],inflow=,length=10.97[m],x=-8000,ManningCoeff=0.01,name=SC13,y=485,_width=200\n");
    file.write("create block;type=Sewerchannelsegment,_height=200,depth=0[m],diameter=0.9144[m],bottom_elevation=9.87[m],inflow=,length=49.52[m],x=-7000,ManningCoeff=0.01,name=SC12,y=485,_width=200\n");
    file.write("create block;type=Sewerchannelsegment,_height=200,depth=0[m],diameter=0.9144[m],bottom_elevation=8.91[m],inflow=,length=54.53[m],x=-6000,ManningCoeff=0.01,name=SC11,y=485,_width=200\n");
    file.write("create block;type=Sewerchannelsegment,_height=200,depth=0[m],diameter=0.9144[m],bottom_elevation=7.73[m],inflow=,length=90.22[m],x=-5000,ManningCoeff=0.02,name=SC10,y=488,_width=200\n");
    file.write("create block;type=Sewerchannelsegment,_height=200,depth=0[m],diameter=0.9144[m],bottom_elevation=5.86[m],inflow=,length=90.22[m],x=-4000,ManningCoeff=0.01,name=SC9,y=485,_width=200\n");
    file.write("create block;type=Sewerchannelsegment,_height=200,depth=0[m],diameter=0.9144[m],bottom_elevation=4.19[m],inflow=,length=90.22[m],x=-3000,ManningCoeff=0.01,name=SC8,y=485,_width=200\n");
    file.write("create block;type=Sewerchannelsegment,_height=200,depth=0[m],diameter=0.9144[m],bottom_elevation=2.96[m],inflow=,length=89.82[m],x=-2000,ManningCoeff=0.01,name=SC7,y=582,_width=200\n");
    file.write("create block;type=Sewerchannelsegment,_height=200,depth=0[m],diameter=0.9144[m],bottom_elevation=2.37[m],inflow=,length=9.16[m],x=-1000,ManningCoeff=0.01,name=SC6,y=485,_width=200\n");
    file.write("create block;type=Sewerchannelsegment,_height=200,depth=0[m],diameter=0.9144[m],bottom_elevation=7.51[m],inflow=,length=57.38[m],x=-5800,ManningCoeff=0.01,name=SCfive,y=-800,_width=200\n");
    file.write("create block;type=Sewerchannelsegment,_height=200,depth=0[m],diameter=0.9144[m],bottom_elevation=6.85[m],inflow=,length=59.62[m],x=-4800,ManningCoeff=0.01,name=SCfour,y=-800,_width=200\n");
    file.write("create block;type=Sewerchannelsegment,_height=200,depth=0[m],diameter=0.9144[m],bottom_elevation=5.93[m],inflow=,length=90.06[m],x=-3800,ManningCoeff=0.01,name=SCthree,y=-800,_width=200\n");
    file.write("create block;type=Sewerchannelsegment,_height=200,depth=0[m],diameter=0.9144[m],bottom_elevation=4.51[m],inflow=,length=90.47[m],x=-2800,ManningCoeff=0.01,name=SCtwo,y=-800,_width=200\n");
    file.write("create block;type=Sewerchannelsegment,_height=200,depth=0[m],diameter=0.9144[m],bottom_elevation=2.24[m],inflow=,length=116.23[m],x=-1800,ManningCoeff=0.01,name=SCone,y=-800,_width=200\n");
//Sewer_links
    file.write("create link;from=SC5,to=SC4,type=Sewer2Sewer_link,name=SC5 - SC4\n");
    file.write("create link;from=5,to=SC5,type=Catchment_link,name=5 - SC5\n");
    file.write("create link;from=4,to=SC4,type=Catchment_link,name=4 - SC4\n");
    file.write("create link;from=SC4,to=SC3,type=Sewer2Sewer_link,name=SC4 - SC3\n");
    file.write("create link;from=SC3,to=SC2,type=Sewer2Sewer_link,name=SC3 - SC2\n");
    file.write("create link;from=SC2,to=SC1,type=Sewer2Sewer_link,name=SC2 - SC1\n");
    file.write("create link;from=3,to=SC3,type=Catchment_link,name=3 - SC3\n");
    file.write("create link;from=2,to=SC2,type=Catchment_link,name=2 - SC2\n");
    file.write("create link;from=1,to=SC1,type=Catchment_link,name=1 - SC1\n");
    file.write("create link;from=SC13,to=SC12,type=Sewer2Sewer_link,name=SC13 - SC12\n");
    file.write("create link;from=SC12,to=SC11,type=Sewer2Sewer_link,name=SC12 - SC11\n");
    file.write("create link;from=SC11,to=SC10,type=Sewer2Sewer_link,name=SC11 - SC10\n");
    file.write("create link;from=SC10,to=SC9,type=Sewer2Sewer_link,name=SC10 - SC9\n");
    file.write("create link;from=SC9,to=SC8,type=Sewer2Sewer_link,name=SC9 - SC8\n");
    file.write("create link;from=SC7,to=SC6,type=Sewer2Sewer_link,name=SC7 - SC6\n");
    file.write("create link;from=SC8,to=SC7,type=Sewer2Sewer_link,name=SC8 - SC7\n");
//Catchment_to_Sewer_pipes
    file.write("create link;from=13,to=SC13,type=Catchment_link,name=13 - SC13\n");
    file.write("create link;from=12,to=SC12,type=Catchment_link,name=12 - SC12\n");
    file.write("create link;from=11,to=SC11,type=Catchment_link,name=11 - SC11\n");
    file.write("create link;from=10,to=SC10,type=Catchment_link,name=10 - SC10\n");
    file.write("create link;from=9,to=SC9,type=Catchment_link,name=9 - SC9\n");
    file.write("create link;from=8,to=SC8,type=Catchment_link,name=8 - SC8\n");
    file.write("create link;from=7,to=SC7,type=Catchment_link,name=7 - SC7\n");
    file.write("create link;from=6,to=SC6,type=Catchment_link,name=6 - SC6\n");
    file.write("create link;from=SCfive,to=SCfour,type=Sewer2Sewer_link,name=SCfive - SCfour\n");
    file.write("create link;from=5,to=SCfive,type=Catchment_link,name=5 - SCfive\n");
    file.write("create link;from=4,to=SCfour,type=Catchment_link,name=4 - SCfour\n");
    file.write("create link;from=SCfour,to=SCthree,type=Sewer2Sewer_link,name=SCfour - SCthree\n");
    file.write("create link;from=SCthree,to=SCtwo,type=Sewer2Sewer_link,name=SCthree - SCtwo\n");
    file.write("create link;from=SCtwo,to=SCone,type=Sewer2Sewer_link,name=SCtwo - SCone\n");
    file.write("create link;from=3,to=SCthree,type=Catchment_link,name=3 - SCthree\n");
    file.write("create link;from=2,to=SCtwo,type=Catchment_link,name=2 - SCtwo\n");
    file.write("create link;from=1,to=SCone,type=Catchment_link,name=1 - SCone\n");
//Sewer to pond
    file.write("create link;from=SC1,to=Infiltration_Pond,type=sewer2pond,name=SC1 - Infiltration_Pond,end_elevation=0[m]\n");
    file.write("create link;from=SCone,to=Infiltration_Pond,type=sewer2pond,name=SCone - Infiltration_Pond,end_elevation=0[m]\n");
    file.write("create link;from=SC6,to=Infiltration_Pond,type=sewer2pond,name=SC6 - Infiltration_Pond,end_elevation=0[m]\n");
#endif
//DS boundary & link
    file.write("create block;type=fixed_head,_height=200,_width=200,y=-526,Storage=100000[m~^3],head=0[m],name=Downstream_Boundary,x=821\n");
    file.write("create link;from=Infiltration_Pond,to=Downstream_Boundary,type=wier,name=weir,alpha=392619,beta=2.995,crest_elevation=1.914[m]\n");
    file.write("create block;type = junction_elastic, name = Junction_Elastic, elasticity = 100, y = 4683, elevation = -7[m], x = -1151, _width = 200, _height = 200\n");
    file.write("create link;from=Side_Settling_Chamber,to=Sedimentation_Chamber,type=Sewer_pipe,start_elevation=-4.2[m],ManningCoeff=0.011,end_elevation=-4.3[m],diameter=0.1[m],name=Side_Settling_Chamber - Sedimentation_Chamber,length=3[m]\n");
    file.write("create link;from=Sedimentation_Chamber,to=DryWell,type=Sewer_pipe,start_elevation=-0.2[m],ManningCoeff=0.011,end_elevation=-19.8[m],diameter=0.1[m],name=Sedimentation_Chamber - DryWell,length=19.6[m]\n");
    file.write("create link;from=Sedimentation_Chamber,to=Junction_Elastic,type=darcy_connector,name=Sedimentation_Chamber - Junction_Elastic,Transmissivity=100[m~^3/day]\n");
    file.write("create link;from=Junction_Elastic,to=DryWell,type=darcy_connector,name=Junction_Elastic - DryWell,Transmissivity=100[m~^3/day]\n");
    file.write("create link;from=Side_Settling_Chamber,to=Soil (15$5),type=darcy_connector,name=Side_Settling_Chamber - Soil,Transmissivity=100[m~^3/day]\n");

#ifdef ModelCatchments
//Observations
    file.write("create observation;type=Observation,object=SC1 - Infiltration_Pond,observed_data=/media/arash/E/Dropbox/Drywell Project/Combined_Model/TimeSeriesData/MeasuredFlowData_FI1.txt,name=Obs_FortIrwin1,expression=flow,error_standard_deviation=1,error_structure=normal\n");
    file.write("create observation;type=Observation,object=SC6 - Infiltration_Pond,observed_data=/media/arash/E/Dropbox/Drywell Project/Combined_Model/TimeSeriesData/MeasuredFlowData_FI2.txt,name=Obs_FortIrwin2,expression=flow,error_standard_deviation=1,error_structure=normal\n");
    file.write("create observation;type=Observation,error_standard_deviation=1,expression=depth,error_structure=normal,name=Depth,observed_data=/media/arash/E/Dropbox/Drywell Project/Combined_Model/TimeSeriesData/All_Depth.txt,object=Infiltration_Pond\n");
//Set as parameters
    file.write("setasparameter; object= 5, parametername= dep_storage, quantity= depression_storage\n");
    file.write("setasparameter; object= 5, parametername= Manning_Catchment, quantity= ManningCoeff\n");
    file.write("setasparameter; object= 4, parametername= dep_storage, quantity= depression_storage\n");
    file.write("setasparameter; object= 4, parametername= Manning_Catchment, quantity= ManningCoeff\n");
    file.write("setasparameter; object= 3, parametername= dep_storage, quantity= depression_storage\n");
    file.write("setasparameter; object= 3, parametername= Manning_Catchment, quantity= ManningCoeff\n");
    file.write("setasparameter; object= 2, parametername= dep_storage, quantity= depression_storage\n");
    file.write("setasparameter; object= 2, parametername= Manning_Catchment, quantity= ManningCoeff\n");
    file.write("setasparameter; object= 1, parametername= dep_storage, quantity= depression_storage\n");
    file.write("setasparameter; object= 1, parametername= Manning_Catchment, quantity= ManningCoeff\n");
    file.write("setasparameter; object= 6, parametername= dep_storage, quantity= depression_storage\n");
    file.write("setasparameter; object= 6, parametername= Manning_Catchment, quantity= ManningCoeff\n");
    file.write("setasparameter; object= 7, parametername= dep_storage, quantity= depression_storage\n");
    file.write("setasparameter; object= 7, parametername= Manning_Catchment, quantity= ManningCoeff\n");
    file.write("setasparameter; object= 8, parametername= dep_storage, quantity= depression_storage\n");
    file.write("setasparameter; object= 8, parametername= Manning_Catchment, quantity= ManningCoeff\n");
    file.write("setasparameter; object= 9, parametername= dep_storage, quantity= depression_storage\n");
    file.write("setasparameter; object= 9, parametername= Manning_Catchment, quantity= ManningCoeff\n");
    file.write("setasparameter; object= 10, parametername= dep_storage, quantity= depression_storage\n");
    file.write("setasparameter; object= 10, parametername= Manning_Catchment, quantity= ManningCoeff\n");
    file.write("setasparameter; object= 11, parametername= dep_storage, quantity= depression_storage\n");
    file.write("setasparameter; object= 11, parametername= Manning_Catchment, quantity= ManningCoeff\n");
    file.write("setasparameter; object= 12, parametername= dep_storage, quantity= depression_storage\n");
    file.write("setasparameter; object= 12, parametername= Manning_Catchment, quantity= ManningCoeff\n");
    file.write("setasparameter; object= 13, parametername= dep_storage, quantity= depression_storage\n");
    file.write("setasparameter; object= 13, parametername= Manning_Catchment, quantity= ManningCoeff\n");
    file.write("setasparameter; object= SC5, parametername= Manning_Sewer, quantity= ManningCoeff\n");
    file.write("setasparameter; object= SC4, parametername= Manning_Sewer, quantity= ManningCoeff\n");
    file.write("setasparameter; object= SC3, parametername= Manning_Sewer, quantity= ManningCoeff\n");
    file.write("setasparameter; object= SC2, parametername= Manning_Sewer, quantity= ManningCoeff\n");
    file.write("setasparameter; object= SC1, parametername= Manning_Sewer, quantity= ManningCoeff\n");
    file.write("setasparameter; object= SC13, parametername= Manning_Sewer, quantity= ManningCoeff\n");
    file.write("setasparameter; object= SC12, parametername= Manning_Sewer, quantity= ManningCoeff\n");
    file.write("setasparameter; object= SC11, parametername= Manning_Sewer, quantity= ManningCoeff\n");
    file.write("setasparameter; object= SC10, parametername= Manning_Sewer, quantity= ManningCoeff\n");
    file.write("setasparameter; object= SC9, parametername= Manning_Sewer, quantity= ManningCoeff\n");
    file.write("setasparameter; object= SC8, parametername= Manning_Sewer, quantity= ManningCoeff\n");
    file.write("setasparameter; object= SC7, parametername= Manning_Sewer, quantity= ManningCoeff\n");
    file.write("setasparameter; object= SC6, parametername= Manning_Sewer, quantity= ManningCoeff\n");
    file.write("setasparameter; object= SCfive, parametername= Manning_Sewer, quantity= ManningCoeff\n");
    file.write("setasparameter; object= SCfour, parametername= Manning_Sewer, quantity= ManningCoeff\n");
    file.write("setasparameter; object= SCthree, parametername= Manning_Sewer, quantity= ManningCoeff\n");
    file.write("setasparameter; object= SCtwo, parametername= Manning_Sewer, quantity= ManningCoeff\n");
    file.write("setasparameter; object= SCone, parametername= Manning_Sewer, quantity= ManningCoeff\n");
#endif
    file.write("create observation;type=Observation,object=Side_Settling_Chamber,name=Side_depth,expression=(depth-0.3),observed_data=/home/arash/Dropbox/LA Project/Data/Depth_PreTreat_Shifted.txt,error_structure=normal,error_standard_deviation=1\n");
    file.write("create observation;type=Observation,object=Sedimentation_Chamber,name=depth_sedimentation_chamber,expression=(depth-0.3),observed_data=/home/arash/Dropbox/LA Project/Data/Depth_Drywell_Shifted.txt,error_structure=normal,error_standard_deviation=1\n");


    if (uniform)
    {   for (unsigned int r = 0; r<GP.nr; r++)
        {   for (unsigned int layer=0; layer<GP.n_layers; layer++)
            {
                file.write(QString("setasparameter; object= Soil ("+QString::number(layer+1)+"$"+QString::number(r+1)+"), parametername= Ks_scale, quantity= K_sat_scale_factor\n").toUtf8());
            }
        }
    }
    else
    {
        for (unsigned int r = 0; r<GP.nr; r++)
        {
            for (unsigned int layer=1; layer<=lowest_shallow_layer; layer++)
            {
                file.write(QString("setasparameter; object= Soil ("+QString::number(layer)+"$"+QString::number(r+1)+"), parametername= Ks_scale, quantity= K_sat_scale_factor\n").toUtf8());
            }
        }

    }
    if (uniform)
    {   for (unsigned int r = 0; r<GP.nr; r++)
            for (unsigned int layer=0; layer<GP.n_layer_deep; layer++)
            {
                file.write(QString("setasparameter; object= Soil_deep ("+QString::number(layer+1)+"$"+QString::number(r+1)+"), parametername= Ks_scale, quantity= K_sat_scale_factor\n").toUtf8());
            }

        for (unsigned int layer=0; layer<GP.n_layer_deep; layer++)
        {

            file.write(QString("setasparameter; object= Soil_deep ("+QString::number(layer+1)+"$"+QString::number(0)+"), parametername= Ks_scale, quantity= K_sat_scale_factor\n").toUtf8());
        }
    }
    else
    {
        for (unsigned int r = 0; r<GP.nr; r++)
        {
            for (unsigned int layer=lowest_shallow_layer; layer<LayerData.count(); layer++)
            {
                file.write(QString("setasparameter; object= Soil_deep ("+QString::number(layer)+"$"+QString::number(r+1)+"), parametername= Ks_scale, quantity= K_sat_scale_factor\n").toUtf8());
            }
        }

        for (unsigned int layer=lowest_shallow_layer; layer<LayerData.count(); layer++)
        {
            file.write(QString("setasparameter; object= Soil_deep ("+QString::number(layer)+"$"+QString::number(0)+"), parametername= Ks_scale, quantity= K_sat_scale_factor\n").toUtf8());
        }
    }
    file.close();
    QMessageBox msgBox;
    msgBox.setText("The model file was created!");
    msgBox.exec();
}

void DryWellDialog::On_File_Select()
{
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save"), "",
            tr("script files (*.ohq)"));

    ui->filename_text->setText(fileName);
}

void DryWellDialog::On_ReadLayer_Info()
{
    ui->tableWidget->clear();
    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open"), "",
            tr("text files (*.txt)"));

    QFile inputfile(fileName);
    if (!inputfile.open(QIODevice::ReadOnly|QIODevice::Text))
        qDebug()<< "file "<<fileName<< " cannot be opened";


    QTextStream in(&inputfile);
    //QString line = in.readLine();

    while (!in.atEnd())
       {
            LayerData.append(in.readLine().split(","));

       }
    inputfile.close();

    if (LayerData.count()>0)
    {   ui->tableWidget->setColumnCount(LayerData[0].count());
        ui->tableWidget->setRowCount(LayerData.count()-1);
    }

    for (unsigned int j=0; j<LayerData[0].count(); j++)
    {   QTableWidgetItem *tableitem = new QTableWidgetItem(LayerData[0][j]);
        ui->tableWidget->setHorizontalHeaderItem(j,tableitem);
    }

    for (unsigned int i=1; i<LayerData.count(); i++)
        for (unsigned int j=0; j<LayerData[i].count(); j++)
        {
            QTableWidgetItem *tableitem = new QTableWidgetItem(LayerData[i][j]);
            ui->tableWidget->setItem(i-1,j,tableitem);
        }
    uniform = false;

}

void DryWellDialog::On_CreateCADDrawing()
{
    double gap = 0.05;
    GP.depth_to_gw = ui->Depth_to_GW->text().toDouble();
    GP.n_layer_deep = ui->nl_deep->text().toInt();
    GP.n_layers = ui->nl_shallow->text().toInt();
    GP.nr = ui->nr->text().toInt();
    GP.pond_alpha_param = ui->Alpha->text().toDouble();
    GP.pond_beta_param = ui->Beta->text().toDouble();
    GP.pond_initial_depth = ui->Pond_ini_depth->text().toDouble();
    GP.pond_radius = ui->Pond_radius->text().toDouble();
    GP.well_depth = ui->Depth_of_Well->text().toDouble();
    GP.well_radious = ui->Well_radius->text().toDouble();

    double dr = (GP.pond_radius-GP.well_radious)/double(GP.nr);
    double dy = (GP.well_depth)/double(GP.n_layers);
    double dy_deep = (GP.depth_to_gw-GP.well_depth)/double(GP.n_layer_deep);

    QString fileName;

    if (!freecad)
    {   fileName = QFileDialog::getSaveFileName(this,
            tr("Save"), "",
            tr("SCAD files (*.csg)"));
    }
    else
    {
        fileName = QFileDialog::getSaveFileName(this,
                    tr("Save"), "",
                    tr("Python files (*.py)"));
    }
    if (fileName=="") return;
    QFile file(fileName);
    file.open(QIODevice::WriteOnly|QIODevice::Text);
    unsigned int lowest_shallow_layer = GP.n_layers;
    double lowest_shallow_depth = 0;
    file.write("import Draft\n");
    if (uniform)
    {   for (unsigned int r = 0; r<GP.nr; r++)
        {   for (unsigned int layer=0; layer<GP.n_layers; layer++)
            {   double r_in = r*dr + GP.well_radious;
                double r_out = (r+1)*dr + GP.well_radious;
                QVector3D basecenter(0,0,-dy*(layer+1)+gap);
                QVector<double> color;
                color.append(0.5);color.append(0.5);color.append(0);color.append(0.2);
                QString statement;
                if (!freecad)
                    statement = scad.GenerateTube(basecenter,color,r_out-gap,r_in+gap, dy-2*gap);
                else
                    statement = scad.GenerateTubeFC(QString("shallowlayer_"+QString::number(r+1)+"_"+QString::number(layer+1)) , basecenter,color,r_out-gap,r_in+gap, dy-2*gap);
                file.write(statement.toUtf8());
            }
        }
    }
    else
    {
        for (unsigned int r = 0; r<GP.nr; r++)
        {   double bottom_elev = 0;
            bool reached_bottom = false;
            for (unsigned int layer=1; layer<LayerData.count(); layer++)
            {   bottom_elev -= LayerData[layer][0].toDouble();
                if (!reached_bottom)
                {   double depth = LayerData[layer][0].toDouble();
                    if (bottom_elev<-GP.well_depth)
                    {
                        lowest_shallow_depth = bottom_elev + depth;
                        depth = GP.well_depth-(-bottom_elev-depth);
                        bottom_elev = -GP.well_depth;
                        reached_bottom = true;
                        lowest_shallow_layer = layer;
                    }
                    double r_in = r*dr + GP.well_radious;
                    double r_out = (r+1)*dr + GP.well_radious;
                    QVector3D basecenter(0,0,bottom_elev+gap);
                    QVector<double> color;
                    color.append(0.5);color.append(0.5);color.append(0);color.append(0.2);
                    QString statement;
                    if (!freecad)
                        statement = scad.GenerateTube(basecenter,color,r_out-gap,r_in+gap, depth-2*gap);
                    else
                        statement = scad.GenerateTubeFC(QString("shallowlayer_"+QString::number(r+1)+"_"+QString::number(layer)), basecenter,color,r_out-gap,r_in+gap, depth-2*gap);
                    file.write(statement.toUtf8());

                }
            }
        }

    }
    //deep soil layer
    if (uniform)
    {   for (unsigned int r = 0; r<GP.nr; r++)
            for (unsigned int layer=0; layer<GP.n_layer_deep; layer++)
            {
                double r_in = r*dr + GP.well_radious;
                double r_out = (r+1)*dr + GP.well_radious;
                QVector3D basecenter(0,0,-dy_deep*(layer+1)-GP.well_depth+gap);
                QVector<double> color;
                color.append(0.7);color.append(0.5);color.append(0);color.append(0.2);
                QString statement;
                if (!freecad)
                    statement = scad.GenerateTube(basecenter,color,r_out-gap,r_in+gap, dy_deep-2*gap);
                else
                    statement = scad.GenerateTubeFC(QString("deeplayer_"+QString::number(r+1)+"_"+QString::number(layer+1)), basecenter,color,r_out-gap,r_in+gap, dy_deep-2*gap);
                file.write(statement.toUtf8());
            }

        for (unsigned int layer=0; layer<GP.n_layer_deep; layer++)
        {
            double r_out = GP.well_radious;
            QVector3D basecenter(0,0,-dy_deep*(layer+1)-GP.well_depth+gap);
            QVector<double> color;
            color.append(0.7);color.append(0.5);color.append(0);color.append(0.2);
            QString statement;
            if (!freecad)
                statement = scad.GenerateCylinder(basecenter,color,r_out-gap, dy_deep-2*gap);
            else
                statement = scad.GenerateCylinderFC(QString("deeplayer_"+QString::number(0)+"_"+QString::number(layer+1)),basecenter,color,r_out-gap, dy_deep-2*gap);
            file.write(statement.toUtf8());
        }
    }
    else
    {
        for (unsigned int r = 0; r<GP.nr; r++)
        {   double bottom_elevation = lowest_shallow_depth;
            for (unsigned int layer=lowest_shallow_layer; layer<LayerData.count(); layer++)
            {   double r_in = r*dr + GP.well_radious;
                double r_out = (r+1)*dr + GP.well_radious;
                double depth = LayerData[layer][0].toDouble();
                bottom_elevation -= depth;
                if (layer==lowest_shallow_layer)
                {
                    depth = depth - (GP.well_depth + lowest_shallow_depth);
                }
                QVector3D basecenter(0,0,bottom_elevation+gap);
                QVector<double> color;
                color.append(0.7);color.append(0.5);color.append(0);color.append(0.2);
                QString statement;
                if (!freecad)
                    statement = scad.GenerateTube(basecenter,color,r_out-gap,r_in+gap, depth-2*gap);
                else
                    statement = scad.GenerateTubeFC(QString("deeplayer_"+QString::number(r+1)+"_"+QString::number(layer+1)),basecenter,color,r_out-gap,r_in+gap, depth-2*gap);
                file.write(statement.toUtf8());
            }
        }
        double bottom_elevation = lowest_shallow_depth;
        for (unsigned int layer=lowest_shallow_layer; layer<LayerData.count(); layer++)
        {
            double depth = LayerData[layer][0].toDouble();
            bottom_elevation -= depth;
            if (layer==lowest_shallow_layer)
            {
                depth = depth - (GP.well_depth + lowest_shallow_depth);
            }
            double r_out = GP.well_radious;
            QVector3D basecenter(0,0,bottom_elevation+gap);
            QVector<double> color;
            color.append(0.7);color.append(0.5);color.append(0);color.append(0.2);
            QString statement;
            if (!freecad)
                statement = scad.GenerateCylinder(basecenter,color,r_out-gap, depth-2*gap);
            else
                statement = scad.GenerateCylinderFC(QString("deeplayer_"+QString::number(0)+"_"+QString::number(layer+1)),basecenter,color,r_out-gap, depth-2*gap);
            file.write(statement.toUtf8());
        }

    }
    QVector3D basecenter(0,0,-GP.depth_to_gw-0.1-gap);
    QVector3D Size(GP.pond_radius*1.2*2,GP.pond_radius*1.2*2,0.1);
    QVector<double> color;
    color.append(0.1);color.append(0.2);color.append(0.9);color.append(0.3);
    QString statement;
    if (!freecad)
        statement = scad.GenerateBox(basecenter,Size,color);
    else
        statement = scad.GenerateBoxFC("Groundwater", basecenter,Size,color);
    file.write(statement.toUtf8());

    if (freecad)

    {
        QVector<double> color;
        color.append(0.8);color.append(0.4);color.append(0.1);color.append(0.4);
        QString statement = scad.CreatePondFC("Pond_empty", pow(3.1415/GP.pond_alpha_param/GP.pond_beta_param,1.0/(GP.pond_beta_param-1)),2.0/(GP.pond_beta_param-1),gap,GP.pond_radius,false,color,10);
        file.write(statement.toUtf8());
        color.clear();
        color.append(0.0);color.append(0.5);color.append(0.7);color.append(0.15);
        statement = scad.CreatePondFC("Pond_full", pow(3.1415/GP.pond_alpha_param/GP.pond_beta_param,1.0/(GP.pond_beta_param-1)),2.0/(GP.pond_beta_param-1),2*gap,GP.pond_radius/2,true,color,5);
        file.write(statement.toUtf8());
    }
    QVector3D origin(GP.pond_radius*1.4,GP.pond_radius*1.4,-400);
    QVector3D size(GP.pond_radius*2.8, GP.pond_radius*2.8,800);
    color.clear(); color.append(0); color.append(0);color.append(0);color.append(0);

    if (ui->checkCut->isChecked() && freecad)
    {
        statement = scad.GenerateBoxFC("cutter",origin,size,color);
        file.write(statement.toUtf8());
        color.clear(); color.append(0.7); color.append(0.6); color.append(0);color.append(0.2);
        if (uniform)
        {
            for (unsigned int r = 0; r<GP.nr; r++)
            {   for (unsigned int layer=0; layer<GP.n_layers; layer++)
                {   statement = scad.BooleanCut(QString("shallowlayer_"+QString::number(r+1)+"_"+QString::number(layer+1)+"_cut"), QString("shallowlayer_"+QString::number(r+1)+"_"+QString::number(layer+1)),"cutter",color);
                    file.write(statement.toUtf8());
                }
            }
        }
        else
        {
            for (unsigned int r = 0; r<GP.nr; r++)
            {
                for (unsigned int layer=0; layer<lowest_shallow_layer; layer++)
                {
                    statement = scad.BooleanCut(QString("shallowlayer_"+QString::number(r+1)+"_"+QString::number(layer+1)+"_cut"), QString("shallowlayer_"+QString::number(r+1)+"_"+QString::number(layer+1)),"cutter",color);
                    file.write(statement.toUtf8());
                }
            }
        }
        //deep layer
        color.clear(); color.append(0.9); color.append(0.5); color.append(0.1);color.append(0.2);
        if (uniform)
        {   for (unsigned int r = 0; r<GP.nr; r++)
                for (unsigned int layer=0; layer<GP.n_layer_deep; layer++)
                {
                    statement = scad.BooleanCut(QString("deeplayer_"+QString::number(r+1)+"_"+QString::number(layer+1)+"_cut"), QString("deeplayer_"+QString::number(r+1)+"_"+QString::number(layer+1)),"cutter",color);
                    file.write(statement.toUtf8());
                }

            for (unsigned int layer=0; layer<GP.n_layer_deep; layer++)
            {
                statement = scad.BooleanCut(QString("deeplayer_"+QString::number(0)+"_"+QString::number(layer+1)+"_cut"), QString("deeplayer_"+QString::number(0)+"_"+QString::number(layer+1)),"cutter",color);
                file.write(statement.toUtf8());
            }
        }
        else
        {
            for (unsigned int r = 0; r<GP.nr; r++)
            {
                for (unsigned int layer=lowest_shallow_layer; layer<LayerData.count(); layer++)
                {
                    statement = scad.BooleanCut(QString("deeplayer_"+QString::number(r+1)+"_"+QString::number(layer+1)+"_cut"), QString("deeplayer_"+QString::number(r+1)+"_"+QString::number(layer+1)),"cutter",color);
                    file.write(statement.toUtf8());
                }
            }
            for (unsigned int layer=lowest_shallow_layer; layer<LayerData.count(); layer++)
            {
                statement = scad.BooleanCut(QString("deeplayer_"+QString::number(0)+"_"+QString::number(layer+1)+"_cut"), QString("deeplayer_"+QString::number(0)+"_"+QString::number(layer+1)),"cutter",color);
                file.write(statement.toUtf8());
            }

        }
        color.append(0.1);color.append(0.2);color.append(0.9);color.append(0.3);
        statement = scad.BooleanCut(QString("Groundwater_cut"), QString("Groundwater"),QString("cutter"),color);
        file.write(statement.toUtf8());
        color.append(0.8);color.append(0.4);color.append(0.1);color.append(0.4);
        statement = scad.BooleanCut(QString("Pond_empty_cut"), QString("Pond_empty"),QString("cutter"),color);
        file.write(statement.toUtf8());
        color.clear();
        color.append(0.0);color.append(0.5);color.append(0.7);color.append(0.15);
        statement = scad.BooleanCut(QString("Pond_full_cut"), QString("Pond_full"),QString("cutter"),color);
        file.write(statement.toUtf8());
    }

    file.write("App.ActiveDocument.recompute()\n");
    file.close();

}

void DryWellDialog::On_CreateVTK()
{
#ifdef use_VTK
    VTKDialog *vtkDialog = new VTKDialog(this);
    vtkDialog->show();
#endif
}




