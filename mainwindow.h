#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

public slots:
    void on_ActionDryWell();
    void on_ActionImport();
    void on_ActionBioSwale();
    void on_ActionImport_Rosemead();
};

#endif // MAINWINDOW_H
