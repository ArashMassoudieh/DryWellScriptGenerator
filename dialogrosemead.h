#ifndef DIALOGROSEMEAD_H
#define DIALOGROSEMEAD_H

#include <QDialog>

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

public slots:
    void On_File_Select();
};

#endif // DIALOGROSEMEAD_H
