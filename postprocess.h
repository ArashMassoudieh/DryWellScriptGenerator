#ifndef POSTPROCESS_H
#define POSTPROCESS_H

#include <QDialog>

namespace Ui {
class PostProcess;
}

class PostProcess : public QDialog
{
    Q_OBJECT

public:
    explicit PostProcess(QWidget *parent = nullptr);
    ~PostProcess();

private:
    Ui::PostProcess *ui;
};

#endif // POSTPROCESS_H
