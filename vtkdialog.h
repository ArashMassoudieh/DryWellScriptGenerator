#ifndef VTKDIALOG_H
#define VTKDIALOG_H

#include <QDialog>
#include "BTCSet.h"
#include "QVector2D"
#include "QMap"

#ifdef use_VTK
    #include "VTK.h"
#endif

struct blockinfo
{
    QVector2D location;
    QString name;
};

struct cellplotinfo
{
    blockinfo name_location;
    int column_no;
};

namespace Ui {
class VTKDialog;
}

class VTKDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VTKDialog(QWidget *parent = nullptr);
    ~VTKDialog();
    CTimeSeriesSet<double> AllResults;
    QList<QString> variables;
    QMap<QString,blockinfo> BlockInfo;
    void write_to_vtp(const QVector<cellplotinfo> &cellinfo, QVector<double> &values, QString &filename, const QString &name);

private:
    Ui::VTKDialog *ui;
    CTimeSeriesSet<double> CompleteOutputData;

private slots:
    void onReadResults();
    void onReadScript();
    void onGenerateVTK();



};

QVector<double> ConvertToQVector(const vector<double> &x);

#endif // VTKDIALOG_H
