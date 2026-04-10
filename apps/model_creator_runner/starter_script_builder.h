// NOTE: This file is part of the DryWellSuite/OpenHydroQual codebase.
#ifndef STARTER_SCRIPT_BUILDER_H
#define STARTER_SCRIPT_BUILDER_H

#include <QString>

struct StarterScriptOptions
{
    /// Directory containing OHQ JSON template resources required by starter generation.
    QString templateDirectory;

    /// Destination path for writing generated starter script text.
    QString outputFile;

    /// Base model flavor for starter creation ("Drywell", "VN_Drywell", or "Bioswale").
    QString modelType = "Drywell";    // Drywell | VN_Drywell | Bioswale

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

    // Optional Ksat scale controls (used by VN generation when applicable).
    QString ksatScaleAll;
    QString ksatScaleG;
    QString ksatScaleUw;

    // ---------------------------------------------------------------------
    // VN-specific configuration
    // ---------------------------------------------------------------------
    //
    // vnBuildMode controls how VN_Drywell scripts are generated:
    //
    //   "Preset"        -> use vnPreset / enrichmentPreset (default, legacy-friendly)
    //   "FullReference" -> use embedded full VN reference OHQ content
    //   "SoftReference" -> template-based VN scaffold + user-provided VN snippets
    //   "LoadFromOhq"   -> load vnBaseOhqFile as authoritative base script
    //
    // For non-VN model types, these fields are ignored.
    QString vnBuildMode = "SoftReference";   // SoftReference | FullReference | LoadFromOhq | Preset

    // VN preset name used only when vnBuildMode == "Preset".
    // If empty, falls back to enrichmentPreset, then to "VN_Drywell".
    QString vnPreset = "VN_Drywell";  // VN_Drywell | VN_Drywell_Pro

    // Optional VN-specific source files.
    //
    // If vnBuildMode == "LoadFromOhq", vnBaseOhqFile is required and is loaded
    // as the authoritative base script. vnSoilLayersFile and
    // vnMoistureLayersFile are appended as additional snippets.
    QString vnBaseOhqFile;
    QString vnSoilLayersFile;
    QString vnMoistureLayersFile;

    // Optional SoftReference grid controls (used when vnBuildMode == "SoftReference").
    int vnSoftGridXCount = 17;
    int vnSoftGridYCount = 12;
    int vnSoftUwGridXCount = 17;
    int vnSoftUwGridYCount = 12;
    double vnSoftCellSize = 586.9;
    double vnSoftUwCellSize = 586.9;
    double vnSoftGapSize = 0.0;
    double vnSoftRwG = 1.2192;
    double vnSoftRwUw = 1.2192;
    double vnSoftRadiusOfInfluence = 20.0;
    double vnSoftDepthOfWellC = 4.8768;
    double vnSoftDepthOfWellG = 7.3152;
    double vnSoftDepthToGroundWater = 43.2816;
    double vnSoftTopElevation = -5.0;
    double vnSoftLayerThickness = 1.0;

    // ---------------------------------------------------------------------
    // General preset mode configuration
    // ---------------------------------------------------------------------
    //
    // Optional preset that appends extra model blocks/links.
    //
    // Supported:
    //   "", "Drywell_MonitoringWell", "Drywell_GroundwaterBoundary",
    //   "Drywell_PretreatmentChambers", "Drywell_SuiteStyle",
    //   "Drywell_LegacyStyle", "VN_Drywell", "VN_Drywell_Pro",
    //   "Bioswale_Underdrain", "Bioswale_Underdrain_GW",
    //   "Bioswale_SuiteStyle", "Bioswale_LegacyStyle"
    //
    // Notes:
    //   - For VN_Drywell in Preset mode, vnPreset is preferred.
    //   - enrichmentPreset remains supported for backward compatibility.
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
