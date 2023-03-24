#include "dialogrosemead.h"
#include "ui_dialogrosemead.h"
#include <QFileDialog>

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

void DialogRoseMead::On_File_Select()
{
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save"), "",
            tr("script files (*.ohq)"));

    ui->SoilFileName->setText(fileName);
}

void DialogRoseMead::On_ReadLayer_Info()
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
