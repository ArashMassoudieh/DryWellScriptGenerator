#ifndef STARTER_SCRIPT_BUILDER_H
#define STARTER_SCRIPT_BUILDER_H

#include <QString>

struct StarterScriptOptions
{
    /// Directory containing OHQ JSON template resources required by starter generation.
    QString templateDirectory;
    /// Destination path for writing generated starter script text.
    QString outputFile;
    /// Base model flavor for starter creation ("Drywell" or "Bioswale").
    QString modelType = "Drywell";    // Drywell | Bioswale
    /// Input inflow time series file path.
    QString inflowFile;
    /// Simulation start time (OHQ numeric timestamp).
    QString simulationStart = "44435";
    /// Simulation end time (OHQ numeric timestamp).
    QString simulationEnd = "44438";
    /// Output series filename used by OHQ during run.
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
    //            "Drywell_PretreatmentChambers", "Drywell_LegacyStyle",
    //            "Bioswale_Underdrain", "Bioswale_Underdrain_GW",
    //            "Bioswale_SuiteStyle", "Bioswale_LegacyStyle"
    QString enrichmentPreset;
};

class StarterScriptBuilder
{
public:
    /**
     * @brief Build starter script text from validated options.
     * @param options Inputs controlling template imports and starter model content.
     * @param scriptText Output string receiving generated script text when successful.
     * @param errorMessage Optional output message for validation or generation failure.
     * @return true when script text was generated, false otherwise.
     */
    static bool BuildText(const StarterScriptOptions &options,
                          QString *scriptText,
                          QString *errorMessage = nullptr);

    /**
     * @brief Build and write a starter script to @c options.outputFile.
     * @param options Inputs controlling validation and script generation.
     * @param errorMessage Optional output message for write/build errors.
     * @return true when file write succeeds, false otherwise.
     */
    static bool Write(const StarterScriptOptions &options,
                      QString *errorMessage = nullptr);
};

#endif // STARTER_SCRIPT_BUILDER_H
