#include "importmoisturedata.h"
#include "ui_importmoisturedata.h"
#include "QFileDialog"

ImportMoistureData::ImportMoistureData(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImportMoistureData)
{
    ui->setupUi(this);
    connect(ui->ChooseFolder,SIGNAL(clicked()),this,SLOT(on_choosefolder()));
}

ImportMoistureData::~ImportMoistureData()
{
    delete ui;
}

void ImportMoistureData::on_choosefolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                 "/home",
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
}
