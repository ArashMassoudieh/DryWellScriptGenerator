#include "dialogrosemead.h"
#include "ui_dialogrosemead.h"
#include <QFileDialog>
#include <QDebug>

DialogRoseMead::DialogRoseMead(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogRoseMead)
{
    ui->setupUi(this);
    connect(ui->pushButtonSoilProps, SIGNAL(clicked()),this, SLOT(On_ReadLayer_Info()));
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

}

DialogRoseMead::~DialogRoseMead()
{
    delete ui;
}

void DialogRoseMead::accept()
{
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save"), "",
            tr("script files (*.ohq)"));

    if (fileName == "") return;
    QFile file(fileName);
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




    //Engineered soil
    double x=0;
    double bottom_elevation = 0;
    for (int layer=0; layer<LayerData.size();layer++)
    {
        bottom_elevation -= LayerData[layer][Depth].toDouble();
        double y=layer*200;
        double area = ui->lineEditBioSwaleWidth->text().toDouble()*ui->lineEditLenght->text().toDouble();
        if (bottom_elevation>=-ui->lineEditBioSwaleDepth->text().toDouble())
            file.write(QString("create block;type=Soil,theta_sat=0.4,theta_res=0.2,specific_storage=0.01,x=" + QString::number(x) + ",Evapotranspiration=,n=1.41,y=" + QString::number(y) + ",area=" + QString::number(area) + ",theta=0.2,K_sat_original=10,_width=200,alpha=1,name=EnginneredSoil (" + QString::number(layer + 1)+ "),_height=100,bottom_elevation=" + QString::number(bottom_elevation) + ",depth=" + QString::number(LayerData[layer][Depth].toDouble()) + ",actual_x=0,actual_y=" + QString::number(bottom_elevation+LayerData[layer][Depth].toDouble()/2) + "\n").toUtf8());

    }
    //Left side
    bottom_elevation = 0;
    for (int layer=0; layer<LayerData.size();layer++)
    {
        bottom_elevation -= LayerData[layer][Depth].toDouble();
        double y=layer*200;
        double area = (ui->SystemWidth->text().toDouble()-ui->lineEditBioSwaleWidth->text().toDouble())/2.0*ui->lineEditLenght->text().toDouble();

        for (int column = 0; column<ui->spinBoxLateralCells->text().toInt(); column++)
        {
            x = 200*(column+1);
            if (bottom_elevation>=-ui->lineEditBioSwaleDepth->text().toDouble())
                file.write(QString("create block;type=Soil,theta_sat="+LayerData[layer][theta_s]+",theta_res="+LayerData[layer][theta_r]+",specific_storage=0.01,x=" + QString::number(x) + ",Evapotranspiration=,n="+LayerData[layer][n]+",y=" + QString::number(y) + ",area=" + QString::number(area) + ",theta=0.2,K_sat_original=1,_width=150,alpha="+LayerData[layer][alpha]+",name=LeftTop (" + QString::number(layer + 1)+ "$" + QString::number(column + 1)+ "),_height=100,bottom_elevation=" + QString::number(bottom_elevation) + ",depth=" + QString::number(LayerData[layer][Depth].toDouble()) + ",actual_x=0,actual_y=" + QString::number(bottom_elevation+LayerData[layer][Depth].toDouble()/2) + "\n").toUtf8());
        }
    }




    file.close();
}

void DialogRoseMead::On_ReadLayer_Info()
{
    ui->tableWidget->clear();
    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open"), "",
            tr("text files (*.txt)"));

    ui->SoilFileName->setText(fileName);

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


}
