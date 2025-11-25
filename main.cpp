#include <QApplication>
#include "mainwindow.h"

#include "paths.h"
#include"solver_runner.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    InitializePaths();   // initialize shared paths

    MainWindow w;
    w.show();
    return a.exec();

    //QString fileName = "DW_test.ohq";
    //RunOHQ(fileName);   // <<< automatically run solver

}
