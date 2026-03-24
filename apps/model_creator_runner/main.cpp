// NOTE: This file is part of the DryWellSuite/OpenHydroQual codebase.
#include <QApplication>

#include "modelcreatorwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ModelCreatorWindow window;
    window.show();

    return app.exec();
}
