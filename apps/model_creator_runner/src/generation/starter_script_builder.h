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

    /// Base model flavor for starter creation ("HQ_Drywell", "VN_Drywell", "R_Bioswale", or "JM_Bioretention").
    QString modelType = "HQ_Drywell";    // HQ_Drywell | VN_Drywell | R_Bioswale | JM_Bioretention

    /// Input inflow time series file path.
    QString inflowFile;

    /// When false, generated scripts leave inflow assignments disabled/empty.
    /// This is controlled by the ModelCreatorWindow "Use/No file" button.
    bool useInflowFile = true;

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
    //   "FullReference" -> use embedded full VN reference OHQ content
    //   "SoftReference" -> template-based VN scaffold + user-provided VN snippets
    //   "LoadFromOhq"   -> load vnBaseOhqFile as authoritative base script
    //
    // For non-VN model types, these fields are ignored.
    QString vnBuildMode = "SoftReference";   // SoftReference | FullReference | LoadFromOhq

    // Legacy VN preset name (no dedicated Preset build mode; kept for compatibility).
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
    //   "SoftReference" -> delegate to HqDrywellBuilder::Build(...) using the
    //                      HQ SoftRef path (soil blocks regenerated via
    //                      HqDrywellBuilder::BuildSoilBlockCommand and the
    //                      structure's own Drywell-style script logic)
    //   "FullReference" -> use the embedded HQ full-reference payload through
    //                      HqDrywellBuilder::Build(...)
    //   "LoadFromOhq"   -> load hqBaseOhqFile as authoritative script
    QString hqBuildMode = "SoftReference"; // SoftReference | FullReference | LoadFromOhq
    QString hqBaseOhqFile;
    // Optional HQ SoftReference soil-property file. When provided, HQ soil
    // block parameters are taken from this layer table while geometry remains
    // controlled by the HQ SoftReference geometry fields.
    QString hqSoilPropsFile;
    // Optional HQ SoftReference geometry controls (legacy DryWell-style).
    // Values <= 0 keep embedded/reference geometry for that field.
    int hqSoftRadialCells = 0;
    int hqSoftShallowLayers = 0;
    double hqSoftWellDepth = 0.0;
    double hqSoftWellRadius = 0.0;
    double hqSoftPondRadius = 0.0;
    double hqSoftSurfaceElevation = 0.0;

    // ---------------------------------------------------------------------
    // R_Bioswale-specific build mode configuration
    // ---------------------------------------------------------------------
    //   "SoftReference" -> delegate to RBioswaleBuilder::Build(...) using the
    //                      R SoftRef path (soil blocks regenerated via
    //                      RBioswaleBuilder::BuildSoilBlockCommand and the
    //                      structure's own Bioswale-style script logic)
    //   "FullReference" -> use the embedded R_Bioswale full-reference payload
    //                      through RBioswaleBuilder::Build(...)
    //   "LoadFromOhq"   -> load rBioswaleBaseOhqFile as authoritative script
    QString rBioswaleBuildMode = "SoftReference"; // SoftReference | FullReference | LoadFromOhq
    QString rBioswaleBaseOhqFile;

    // Optional R_Bioswale SoftReference controls (procedural Rosemead-style soil-block generation).
    double rBioSwaleWidth = 0.6096;
    double rSystemWidth = 3.0;
    double rBioSwaleDepth = 0.9144;
    double rLength = 8.0;
    int rLateralCells = 6;
    double rStreetWidth = 5.0;
    int rStreetCells = 10;
    int rVerticalLayers = 0; // Legacy R/Bioswale total nz override; 0 = use file/reference row count.
    int rEngineeredSoilNz = 0; // R/Bioswale engineered-soil vertical rows; 0 = infer from depth split/reference.
    int rNativeSoilNz = 0;     // R/Bioswale native/bottom-soil vertical rows; 0 = infer from depth split/reference.
    double rAnisoRatio = 5.0;
    QString rSoilPropsFile;


    // ---------------------------------------------------------------------
    // JM_Bioretention-specific build mode and geometry (John McCormack Rd)
    // ---------------------------------------------------------------------
    QString jmBuildMode = "SoftReference"; // SoftReference | FullReference | LoadFromOhq | Channel | DTSimple | DTSimpleGutter
    QString jmBaseOhqFile;
    double jmLength = 12.192;              // 40 ft
    double jmWidth = 3.7084;               // 12 ft 2 in
    double jmMulchDepth = 0.0762;          // 3 in
    double jmMediaDepth = 0.9144;          // 36 in
    double jmChokerDepth = 0.0762;         // 3 in
    double jmGravelDepth = 0.6096;         // 24 in
    double jmSumpDepth = 0.3048;           // 12 in
    double jmUnderdrainDiameter = 0.1016;  // 4 in
    double jmCatchmentArea = 1000.0;
    // Remaining centered native-soil domain below the JM aggregate layer.
    // 1 x 1 creates one native block; 4 x 1 creates one row of four blocks.
    int jmNativeHorizontalCells = 4;       // nx, minimum 1
    int jmNativeVerticalLayers = 3;        // nz, minimum 1

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
