#include <QApplication>
#include "paths.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    InitializePaths();   // initialize shared paths

    MainWindow w;
    w.show();
    return a.exec();
}
