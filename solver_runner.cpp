#include "solver_runner.h"
#include "paths.h"

#include "System.h"
#include "Script.h"
#include <QFileInfo>
#include <QDebug>

bool RunOHQ(const QString &inputFile)
{
    try {
        if (inputFile.isEmpty()) {
            qWarning() << "[OHQ Solver] No input file.";
            return false;
        }

        System system;

        system.SetDefaultTemplatePath(ohq_r.toStdString());
        system.SetWorkingFolder(
            QFileInfo(inputFile).canonicalPath().toStdString() + "/"
        );

        QString settings = ohq_r + "settings.json";

        Script script(inputFile, &system);

        qDebug() << "[OHQ Solver] Building system...";
        system.CreateFromScript(script, settings.toStdString());

        qDebug() << "[OHQ Solver] Solving...";
        system.Solve();

        QString outFile =
            QString::fromStdString(system.GetWorkingFolder()) +
            QString::fromStdString(system.OutputFileName());

        qDebug() << "[OHQ Solver] Writing output:" << outFile;
        system.GetOutputs().write(outFile.toStdString());

        return true;
    }
    catch (...) {
        qWarning() << "[OHQ Solver] Exception!";
        return false;
    }
}
