#include "postprocess.h"
#include "ui_postprocess.h"

PostProcess::PostProcess(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PostProcess)
{
    ui->setupUi(this);
}

PostProcess::~PostProcess()
{
    delete ui;
}
