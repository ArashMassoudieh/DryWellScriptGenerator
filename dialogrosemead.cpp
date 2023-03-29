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
    int lowest_up;
    double bottom_elevation = 0;
    for (int layer=0; layer<LayerData.size();layer++)
    {
        bottom_elevation -= LayerData[layer][Depth].toDouble();
        double y=layer*200;
        double area = ui->lineEditBioSwaleWidth->text().toDouble()*ui->lineEditLenght->text().toDouble();
        if (bottom_elevation>=-ui->lineEditBioSwaleDepth->text().toDouble())
        {   file.write(QString("create block;type=Soil,theta_sat=0.4,theta_res=0.2,specific_storage=0.01,x=" + QString::number(x) + ",Evapotranspiration=,n=1.41,y=" + QString::number(y) + ",area=" + QString::number(area) + ",theta=0.2,K_sat_original=10,_width=150,alpha=1,name=EngineeredSoil (" + QString::number(layer + 1)+ "),_height=100,bottom_elevation=" + QString::number(bottom_elevation) + ",depth=" + QString::number(LayerData[layer][Depth].toDouble()) + ",actual_x=0,actual_y=" + QString::number(bottom_elevation+LayerData[layer][Depth].toDouble()/2) + "\n").toUtf8());
            lowest_up = layer;
        }
    }
    //Top Left side
    bottom_elevation = 0;
    for (int layer=0; layer<LayerData.size();layer++)
    {
        bottom_elevation -= LayerData[layer][Depth].toDouble();
        double y=layer*200;
        double area = (ui->SystemWidth->text().toDouble()-ui->lineEditBioSwaleWidth->text().toDouble())/2.0*ui->lineEditLenght->text().toDouble()/double(ui->spinBoxLateralCells->value());

        for (int column = 0; column<ui->spinBoxLateralCells->text().toInt(); column++)
        {
            x = 200*(column+1);
            if (bottom_elevation>=-ui->lineEditBioSwaleDepth->text().toDouble())
                file.write(QString("create block;type=Soil,theta_sat="+LayerData[layer][theta_s]+",theta_res="+LayerData[layer][theta_r]+",specific_storage=0.01,x=" + QString::number(x) + ",Evapotranspiration=,n="+LayerData[layer][n]+",y=" + QString::number(y) + ",area=" + QString::number(area) + ",theta=0.2,K_sat_original=1,_width=150,alpha="+LayerData[layer][alpha]+",name=LeftTop (" + QString::number(layer + 1)+ "$" + QString::number(column + 1)+ "),_height=100,bottom_elevation=" + QString::number(bottom_elevation) + ",depth=" + QString::number(LayerData[layer][Depth].toDouble()) + ",actual_x=0,actual_y=" + QString::number(bottom_elevation+LayerData[layer][Depth].toDouble()/2) + "\n").toUtf8());
        }
    }

    //Top Right side
    bottom_elevation = 0;
    for (int layer=0; layer<LayerData.size();layer++)
    {
        bottom_elevation -= LayerData[layer][Depth].toDouble();
        double y=layer*200;
        double area = (ui->SystemWidth->text().toDouble()-ui->lineEditBioSwaleWidth->text().toDouble())/2.0*ui->lineEditLenght->text().toDouble()/double(ui->spinBoxLateralCells->value());

        for (int column = 0; column<ui->spinBoxLateralCells->text().toInt(); column++)
        {
            x = -200*(column+1);
            if (bottom_elevation>=-ui->lineEditBioSwaleDepth->text().toDouble())
                file.write(QString("create block;type=Soil,theta_sat="+LayerData[layer][theta_s]+",theta_res="+LayerData[layer][theta_r]+",specific_storage=0.01,x=" + QString::number(x) + ",Evapotranspiration=,n="+LayerData[layer][n]+",y=" + QString::number(y) + ",area=" + QString::number(area) + ",theta=0.2,K_sat_original=1,_width=150,alpha="+LayerData[layer][alpha]+",name=RightTop (" + QString::number(layer + 1)+ "$" + QString::number(column + 1)+ "),_height=100,bottom_elevation=" + QString::number(bottom_elevation) + ",depth=" + QString::number(LayerData[layer][Depth].toDouble()) + ",actual_x=0,actual_y=" + QString::number(bottom_elevation+LayerData[layer][Depth].toDouble()/2) + "\n").toUtf8());
        }
    }

    //UnderneathEngineered soil
    x=0;
    bottom_elevation = 0;
    for (int layer=0; layer<LayerData.size();layer++)
    {
        bottom_elevation -= LayerData[layer][Depth].toDouble();
        double y=layer*200;
        double area = ui->lineEditBioSwaleWidth->text().toDouble()*ui->lineEditLenght->text().toDouble();
        if (bottom_elevation<-ui->lineEditBioSwaleDepth->text().toDouble())
            file.write(QString("create block;type=Soil,theta_sat="+LayerData[layer][theta_s]+",theta_res="+LayerData[layer][theta_r]+",specific_storage=0.01,x=" + QString::number(x) + ",Evapotranspiration=,n="+LayerData[layer][n]+",y=" + QString::number(y) + ",area=" + QString::number(area) + ",theta=0.2,K_sat_original=1,_width=150,alpha="+LayerData[layer][alpha]+",name=UEngineered (" + QString::number(layer + 1)+ "),_height=100,bottom_elevation=" + QString::number(bottom_elevation) + ",depth=" + QString::number(LayerData[layer][Depth].toDouble()) + ",actual_x=0,actual_y=" + QString::number(bottom_elevation+LayerData[layer][Depth].toDouble()/2) + "\n").toUtf8());
    }

    //Bottom Left side
    bottom_elevation = 0;
    for (int layer=0; layer<LayerData.size();layer++)
    {
        bottom_elevation -= LayerData[layer][Depth].toDouble();
        double y=layer*200;
        double area = (ui->SystemWidth->text().toDouble()-ui->lineEditBioSwaleWidth->text().toDouble())/2.0*ui->lineEditLenght->text().toDouble()/double(ui->spinBoxLateralCells->value());

        for (int column = 0; column<ui->spinBoxLateralCells->text().toInt(); column++)
        {
            x = 200*(column+1);
            if (bottom_elevation<-ui->lineEditBioSwaleDepth->text().toDouble())
                file.write(QString("create block;type=Soil,theta_sat="+LayerData[layer][theta_s]+",theta_res="+LayerData[layer][theta_r]+",specific_storage=0.01,x=" + QString::number(x) + ",Evapotranspiration=,n="+LayerData[layer][n]+",y=" + QString::number(y) + ",area=" + QString::number(area) + ",theta=0.2,K_sat_original=1,_width=150,alpha="+LayerData[layer][alpha]+",name=LeftBottom (" + QString::number(layer + 1)+ "$" + QString::number(column + 1)+ "),_height=100,bottom_elevation=" + QString::number(bottom_elevation) + ",depth=" + QString::number(LayerData[layer][Depth].toDouble()) + ",actual_x=0,actual_y=" + QString::number(bottom_elevation+LayerData[layer][Depth].toDouble()/2) + "\n").toUtf8());
        }
    }

    //Bottom Right side
    bottom_elevation = 0;
    for (int layer=0; layer<LayerData.size();layer++)
    {
        bottom_elevation -= LayerData[layer][Depth].toDouble();
        double y=layer*200;
        double area = (ui->SystemWidth->text().toDouble()-ui->lineEditBioSwaleWidth->text().toDouble())/2.0*ui->lineEditLenght->text().toDouble()/double(ui->spinBoxLateralCells->value());

        for (int column = 0; column<ui->spinBoxLateralCells->text().toInt(); column++)
        {
            x = -200*(column+1);
            if (bottom_elevation<-ui->lineEditBioSwaleDepth->text().toDouble())
                file.write(QString("create block;type=Soil,theta_sat="+LayerData[layer][theta_s]+",theta_res="+LayerData[layer][theta_r]+",specific_storage=0.01,x=" + QString::number(x) + ",Evapotranspiration=,n="+LayerData[layer][n]+",y=" + QString::number(y) + ",area=" + QString::number(area) + ",theta=0.2,K_sat_original=1,_width=150,alpha="+LayerData[layer][alpha]+",name=RightBottom (" + QString::number(layer + 1)+ "$" + QString::number(column + 1)+ "),_height=100,bottom_elevation=" + QString::number(bottom_elevation) + ",depth=" + QString::number(LayerData[layer][Depth].toDouble()) + ",actual_x=0,actual_y=" + QString::number(bottom_elevation+LayerData[layer][Depth].toDouble()/2) + "\n").toUtf8());
        }
    }

    //vertical Eng soil connector
    bottom_elevation = 0;
    for (int layer=0; layer<LayerData.size();layer++)
    {
        bottom_elevation -= LayerData[layer][Depth].toDouble();
        if (bottom_elevation - LayerData[layer+1][Depth].toDouble() >=-ui->lineEditBioSwaleDepth->text().toDouble())
            file.write(QString("create link;from=EngineeredSoil ("+QString::number(layer+1)+"),to=EngineeredSoil ("+QString::number(layer+2)+"),type=soil_to_soil_link,name=EngineeredSoil_V ("+QString::number(layer+1)+")\n").toUtf8());
        else
            break;
    }

    //engineered to left
    bottom_elevation = 0;
    for (int layer=0; layer<LayerData.size();layer++)
    {
        double length = ui->lineEditBioSwaleWidth->text().toDouble()/2.0 + (ui->SystemWidth->text().toDouble()-ui->lineEditBioSwaleWidth->text().toDouble())/double(ui->spinBoxLateralCells->value())/4.0;
        double area = LayerData[layer][Depth].toDouble()*ui->lineEditLenght->text().toDouble();
        bottom_elevation -= LayerData[layer][Depth].toDouble();
        if (bottom_elevation >=-ui->lineEditBioSwaleDepth->text().toDouble())
            file.write(QString("create link;from=EngineeredSoil ("+QString::number(layer+1)+"),to=LeftTop ("+QString::number(layer+1)+"$1),type=soil_to_soil_H_link,name=EngineeredSoil-LeftTop ("+QString::number(layer+1)+"),length="+QString::number(length)+"[m],area="+QString::number(area)+"[m~^2])\n").toUtf8());
        else
            break;
    }

    //engineered to right
    bottom_elevation = 0;
    for (int layer=0; layer<LayerData.size();layer++)
    {
        double length = ui->lineEditBioSwaleWidth->text().toDouble()/2.0 + (ui->SystemWidth->text().toDouble()-ui->lineEditBioSwaleWidth->text().toDouble())/double(ui->spinBoxLateralCells->value())/4.0;
        double area = LayerData[layer][Depth].toDouble()*ui->lineEditLenght->text().toDouble();
        bottom_elevation -= LayerData[layer][Depth].toDouble();
        if (bottom_elevation >=-ui->lineEditBioSwaleDepth->text().toDouble())
            file.write(QString("create link;from=EngineeredSoil ("+QString::number(layer+1)+"),to=RightTop ("+QString::number(layer+1)+"$1),type=soil_to_soil_H_link,name=EngineeredSoil-RightTop ("+QString::number(layer+1)+"),length="+QString::number(length)+"[m],area="+QString::number(area)+"[m~^2])\n").toUtf8());
        else
            break;

    }

    //left to left - H
    bottom_elevation = 0;
    for (int layer=0; layer<LayerData.size();layer++)
    {
        double length = (ui->SystemWidth->text().toDouble()-ui->lineEditBioSwaleWidth->text().toDouble())/double(ui->spinBoxLateralCells->value())/2.0;
        double area = LayerData[layer][Depth].toDouble()*ui->lineEditLenght->text().toDouble();
        bottom_elevation -= LayerData[layer][Depth].toDouble();
        if (bottom_elevation >=-ui->lineEditBioSwaleDepth->text().toDouble())
        {
            for (int column=0;column<ui->spinBoxLateralCells->text().toInt()-1; column++)
                file.write(QString("create link;from=LeftTop ("+QString::number(layer+1)+"$" +QString::number(column+1) + "),to=LeftTop ("+QString::number(layer+1)+"$" +QString::number(column+2) + "),type=soil_to_soil_H_link,name=LeftTopH ("+QString::number(layer+1)+"$"+QString::number(column+1)+"),length="+QString::number(length)+"[m],area="+QString::number(area)+"[m~^2])\n").toUtf8());
        }
        else
            break;
    }

    //right to right - H
    bottom_elevation = 0;
    for (int layer=0; layer<LayerData.size();layer++)
    {
        double length = (ui->SystemWidth->text().toDouble()-ui->lineEditBioSwaleWidth->text().toDouble())/double(ui->spinBoxLateralCells->value())/2.0;
        double area = LayerData[layer][Depth].toDouble()*ui->lineEditLenght->text().toDouble();
        bottom_elevation -= LayerData[layer][Depth].toDouble();
        if (bottom_elevation >=-ui->lineEditBioSwaleDepth->text().toDouble())
        {
            for (int column=0;column<ui->spinBoxLateralCells->text().toInt()-1; column++)
                file.write(QString("create link;from=RightTop ("+QString::number(layer+1)+"$" +QString::number(column+1) + "),to=RightTop ("+QString::number(layer+1)+"$" +QString::number(column+2) + "),type=soil_to_soil_H_link,name=RoghtTopH ("+QString::number(layer+1)+"$"+QString::number(column+1)+"),length="+QString::number(length)+"[m],area="+QString::number(area)+"[m~^2])\n").toUtf8());
        }
        else
            break;
    }

    //left top - V
    bottom_elevation = 0;
    for (int layer=0; layer<LayerData.size();layer++)
    {
        bottom_elevation -= LayerData[layer][Depth].toDouble();
        if (bottom_elevation - LayerData[layer+1][Depth].toDouble() >=-ui->lineEditBioSwaleDepth->text().toDouble())
        {
            for (int column=0; column<ui->spinBoxLateralCells->text().toInt();column++)
            file.write(QString("create link;from=LeftTop ("+QString::number(layer+1)+"$" +QString::number(column+1) + "),to=LeftTop ("+QString::number(layer+2)+"$" +QString::number(column+1) + "),type=soil_to_soil_link,name=LeftTop_V ("+QString::number(layer+1)+"$" +QString::number(column+1) + ")\n").toUtf8());

        }
        else
            break;
    }

    //Right top - V
    bottom_elevation = 0;
    for (int layer=0; layer<LayerData.size();layer++)
    {
        bottom_elevation -= LayerData[layer][Depth].toDouble();
        if (bottom_elevation - LayerData[layer+1][Depth].toDouble() >=-ui->lineEditBioSwaleDepth->text().toDouble())
        {
            for (int column=0; column<ui->spinBoxLateralCells->text().toInt();column++)
            file.write(QString("create link;from=RightTop ("+QString::number(layer+1)+"$" +QString::number(column+1) + "),to=RightTop ("+QString::number(layer+2)+"$" +QString::number(column+1) + "),type=soil_to_soil_link,name=RightTop_V ("+QString::number(layer+1)+"$" +QString::number(column+1) + ")\n").toUtf8());
        }
        else
            break;
    }

    //up_to_bottom
    file.write(QString("create link;from=EngineeredSoil ("+QString::number(lowest_up+1)+"),to=UEngineered ("+QString::number(lowest_up+2) + "),type=soil_to_soil_link,name=Engineered_to_bottom ("+QString::number(lowest_up+1)+")\n").toUtf8());

    //left
    for (int column=0; column<ui->spinBoxLateralCells->text().toInt();column++)
        file.write(QString("create link;from=LeftTop ("+QString::number(lowest_up+1)+"$" +QString::number(column+1) + "),to=LeftBottom ("+QString::number(lowest_up+2)+"$" +QString::number(column+1) + "),type=soil_to_soil_link,name=Left_to_bottom ("+QString::number(lowest_up+1)+"$" +QString::number(column+1) + ")\n").toUtf8());


    //right
    for (int column=0; column<ui->spinBoxLateralCells->text().toInt();column++)
        file.write(QString("create link;from=RightTop ("+QString::number(lowest_up+1)+"$" +QString::number(column+1) + "),to=RightBottom ("+QString::number(lowest_up+2)+"$" +QString::number(column+1) + "),type=soil_to_soil_link,name=Right_to_bottom ("+QString::number(lowest_up+1)+"$" +QString::number(column+1) + ")\n").toUtf8());

    //U Engineered - V
    for (int layer = lowest_up+1; layer<LayerData.size()-1; layer++)
    {
        file.write(QString("create link;from=UEngineered ("+QString::number(layer+1)+"),to=UEngineered ("+QString::number(layer+2) + "),type=soil_to_soil_link,name=UEngineered_V ("+QString::number(layer+1)+")\n").toUtf8());

    }

    //LowerLeft - V
    for (int layer = lowest_up+1; layer<LayerData.size()-1; layer++)
    {
        for (int column=0; column<ui->spinBoxLateralCells->text().toInt();column++)
            file.write(QString("create link;from=LeftBottom ("+QString::number(layer+1)+"$" +QString::number(column+1) + "),to=LeftBottom ("+QString::number(layer+2)+"$" +QString::number(column+1) + "),type=soil_to_soil_link,name=LeftBottom_V ("+QString::number(layer+1)+"$" +QString::number(column+1) + ")\n").toUtf8());

    }

    //LowerRight - V
    for (int layer = lowest_up+1; layer<LayerData.size()-1; layer++)
    {
        for (int column=0; column<ui->spinBoxLateralCells->text().toInt();column++)
            file.write(QString("create link;from=RightBottom ("+QString::number(layer+1)+"$" +QString::number(column+1) + "),to=RightBottom ("+QString::number(layer+2)+"$" +QString::number(column+1) + "),type=soil_to_soil_link,name=RightBottom_V ("+QString::number(layer+1)+"$" +QString::number(column+1) + ")\n").toUtf8());

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

    QStringList headers = in.readLine().split(",");
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
    {   QTableWidgetItem *tableitem = new QTableWidgetItem(headers[j]);
        ui->tableWidget->setHorizontalHeaderItem(j,tableitem);
    }

    for (unsigned int i=1; i<LayerData.count(); i++)
        for (unsigned int j=0; j<LayerData[i].count(); j++)
        {
            QTableWidgetItem *tableitem = new QTableWidgetItem(LayerData[i][j]);
            ui->tableWidget->setItem(i-1,j,tableitem);
        }


}
