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

    /// Base model flavor for starter creation ("HQ_Drywell", "VN_Drywell", or "R_Bioswale").
    QString modelType = "HQ_Drywell";    // HQ_Drywell | VN_Drywell | R_Bioswale

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

    // ---------------------------------------------------------------------
    // HQ_Drywell-specific build mode configuration
    // ---------------------------------------------------------------------
    //   "Preset"        -> delegate to HqDrywellBuilder::Build(...) using the
    //                      structure's native Drywell-style helper functions
    //   "SoftReference" -> delegate to HqDrywellBuilder::Build(...) using the
    //                      HQ SoftRef path (soil blocks regenerated via
    //                      HqDrywellBuilder::BuildSoilBlockCommand and the
    //                      structure's own Drywell-style script logic)
    //   "FullReference" -> use the embedded HQ full-reference payload through
    //                      HqDrywellBuilder::Build(...)
    //   "LoadFromOhq"   -> load hqBaseOhqFile as authoritative script
    QString hqBuildMode = "Preset"; // Preset | SoftReference | FullReference | LoadFromOhq
    QString hqBaseOhqFile;

    // ---------------------------------------------------------------------
    // R_Bioswale-specific build mode configuration
    // ---------------------------------------------------------------------
    //   "Preset"        -> delegate to RBioswaleBuilder::Build(...) using the
    //                      structure's native Bioswale-style helper functions
    //   "SoftReference" -> delegate to RBioswaleBuilder::Build(...) using the
    //                      R SoftRef path (soil blocks regenerated via
    //                      RBioswaleBuilder::BuildSoilBlockCommand and the
    //                      structure's own Bioswale-style script logic)
    //   "FullReference" -> use the embedded R_Bioswale full-reference payload
    //                      through RBioswaleBuilder::Build(...)
    //   "LoadFromOhq"   -> load rBioswaleBaseOhqFile as authoritative script
    QString rBioswaleBuildMode = "Preset"; // Preset | SoftReference | FullReference | LoadFromOhq
    QString rBioswaleBaseOhqFile;

    // Optional R_Bioswale SoftReference controls (procedural Rosemead-style soil-block generation).
    double rBioSwaleWidth = 0.6096;
    double rSystemWidth = 3.0;
    double rBioSwaleDepth = 0.9144;
    double rLength = 8.0;
    int rLateralCells = 6;
    double rStreetWidth = 5.0;
    int rStreetCells = 10;
    double rAnisoRatio = 5.0;
    QString rSoilPropsFile;

    // Optional SoftReference grid controls (used when vnBuildMode == "SoftReference").
    int vnSoftGridXCount = 16;
    int vnSoftGridYCount = 15;
    int vnSoftUwGridXCount = 16;
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
    // Optional VN SoftReference soil-parameter controls (modelcreator-aligned defaults).
    // Source naming correspondence:
    //   theta_s -> theta_sat, theta_r -> theta_res, Ksat -> K_sat_original
    double vnSoftSoilKsatOriginal = 1.05196;
    double vnSoftSoilAlpha = 3.47536;
    double vnSoftSoilN = 1.74582;
    double vnSoftSoilThetaSat = 0.39;
    double vnSoftSoilThetaRes = 0.049;
    // Soil parameter source mode:
    //   Manual: use explicit VN soft soil fields above
    //   VnReferenceDefaults: use VN full-reference depth profile in script builder
    //   ModelCreatorDefaults: use ModelCreator-equivalent constants in script builder
    //   File: linearly interpolate params by depth from vnSoftSoilParameterFile CSV
    QString vnSoftSoilParamMode = "VnReferenceDefaults";
    // Optional CSV source used when vnSoftSoilParamMode == "File".
    // Expected headers (case-insensitive, flexible aliases):
    //   depth/depth_m, Ksat, alpha, n, theta_s/theta_sat, theta_r/theta_res
    QString vnSoftSoilParameterFile;

    // ---------------------------------------------------------------------
    // General preset mode configuration
    // ---------------------------------------------------------------------
    //
    // Optional preset that appends extra model blocks/links.
    //
    // Supported:
    //   "", "HQ_Drywell_MonitoringWell", "HQ_Drywell_GroundwaterBoundary",
    //   "HQ_Drywell_PretreatmentChambers", "HQ_Drywell_SuiteStyle",
    //   "HQ_Drywell_LegacyStyle", "VN_Drywell", "VN_Drywell_Pro",
    //   "R_Bioswale_Underdrain", "R_Bioswale_Underdrain_GW",
    //   "R_Bioswale_SuiteStyle", "R_Bioswale_LegacyStyle"
    //
    // Notes:
    //   - For VN_Drywell in Preset mode, vnPreset is preferred.
    //   - enrichmentPreset remains supported for backward compatibility.
    QString enrichmentPreset;
};

class StarterScriptBuilder
{
public:
    static QString VnReferenceSoilProfileCsv();

    static bool BuildText(const StarterScriptOptions &options,
                          QString *scriptText,
                          QString *errorMessage = nullptr);

    static bool Write(const StarterScriptOptions &options,
                      QString *errorMessage = nullptr);
};

#endif // STARTER_SCRIPT_BUILDER_H
