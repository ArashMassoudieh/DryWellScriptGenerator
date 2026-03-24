// NOTE: This file is part of the DryWellSuite/OpenHydroQual codebase.
/*
#include "solver_runner.h"
#include "paths.h"

#include "System.h"
#include "Script.h"

#include <QFileInfo>
#include <QDebug>
#include <QCoreApplication>

bool RunOHQ(const QString &inputFile)
{
    try {
        if (inputFile.isEmpty()) {
            qWarning() << "[OHQ Solver] No input file.";
            return false;
        }

        // ----------------------------
        // Construct System
        // ----------------------------
        System system;

        // ----------------------------
        // Default Template Path
        // (matches original console runner)
        // ----------------------------
        QString templatePath =
            qApp->applicationDirPath() + "/../../resources/";

        system.SetDefaultTemplatePath(templatePath.toStdString());

        // ----------------------------
        // Working Folder
        // ----------------------------
        QString workFolder =
            QFileInfo(inputFile).canonicalPath() + "/";

        system.SetWorkingFolder(workFolder.toStdString());

        // ----------------------------
        // Read Script
        // ----------------------------
        Script scr(inputFile.toStdString(), &system);

        // ----------------------------
        // Settings.json
        // ----------------------------
        QString settingsFile = templatePath + "settings.json";

        // ----------------------------
        // Create model from script
        // ----------------------------
        system.CreateFromScript(scr, settingsFile.toStdString());
        system.SetSilent(false);

        // ----------------------------
        // Solve system
        // ----------------------------
        system.Solve();

        // ----------------------------
        // Write outputs
        // ----------------------------
        QString outputFile =
            QString::fromStdString(system.GetWorkingFolder()) +
            QString::fromStdString(system.OutputFileName());

        system.GetOutputs().write(outputFile.toStdString());

        qDebug() << "[OHQ Solver] Output written:" << outputFile;

        return true;
    }
    catch (std::exception &e) {
        qWarning() << "[OHQ Solver] Exception:" << e.what();
        return false;
    }
    catch (...) {
        qWarning() << "[OHQ Solver] Unknown exception!";
        return false;
    }
}
*/
