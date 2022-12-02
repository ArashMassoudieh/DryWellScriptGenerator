#ifndef IMPORTMOISTUREDATA_H
#define IMPORTMOISTUREDATA_H

#include <QDialog>

namespace Ui {
class ImportMoistureData;
}

class ImportMoistureData : public QDialog
{
    Q_OBJECT

public:
    explicit ImportMoistureData(QWidget *parent = nullptr);
    ~ImportMoistureData();

private:
    Ui::ImportMoistureData *ui;

public slots:
    void on_choosefolder();
};

#endif // IMPORTMOISTUREDATA_H
