#ifndef DIALOGROSEMEAD_H
#define DIALOGROSEMEAD_H

#include <QDialog>

enum columns {Depth,K_sat,alpha,n,theta_s,theta_r};

namespace Ui {
class DialogRoseMead;
}

class DialogRoseMead : public QDialog
{
    Q_OBJECT

public:
    explicit DialogRoseMead(QWidget *parent = nullptr);
    ~DialogRoseMead();

private:
    Ui::DialogRoseMead *ui;
    QList<QStringList> LayerData;

public slots:
    void On_ReadLayer_Info(const QString &filename="");
    void accept();


};

#endif // DIALOGROSEMEAD_H
