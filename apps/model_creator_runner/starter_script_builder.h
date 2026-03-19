#ifndef STARTER_SCRIPT_BUILDER_H
#define STARTER_SCRIPT_BUILDER_H

#include <QString>

struct StarterScriptOptions
{
    QString templateDirectory;
    QString outputFile;
    QString modelType = "Drywell";    // Drywell | Bioswale
    QString inflowFile;
    QString simulationStart = "44435";
    QString simulationEnd = "44438";
    QString outputSeriesFile = "OHQ_output.txt";

    // Optional starter observation settings
    QString observationFile;
    QString observationObject;
    QString observationExpression = "theta";
    QString observationName = "Obs_1";

    // Optional raw OHQ lines appended at the end of generated starter script.
    QString additionalCommands;

    // Optional preset that appends extra model blocks/links.
    // Supported: "", "Drywell_MonitoringWell", "Drywell_GroundwaterBoundary",
    //            "Drywell_PretreatmentChambers", "Bioswale_Underdrain",
    //            "Bioswale_Underdrain_GW"
    QString enrichmentPreset;
};

class StarterScriptBuilder
{
public:
    static bool BuildText(const StarterScriptOptions &options,
                          QString *scriptText,
                          QString *errorMessage = nullptr);

    static bool Write(const StarterScriptOptions &options,
                      QString *errorMessage = nullptr);
};

#endif // STARTER_SCRIPT_BUILDER_H
