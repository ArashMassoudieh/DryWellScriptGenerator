// NOTE: This file is part of the DryWellSuite/OpenHydroQual codebase.
#ifndef MODELCREATORWINDOW_H
#define MODELCREATORWINDOW_H

#include <QDateTime>
#include <QList>
#include <QMainWindow>
#include <QPointF>
#include <QStringList>
#include <QVector>

class QComboBox;
class QLineEdit;
class QPushButton;
class QTabWidget;
class QTextEdit;
class QLabel;
class QCheckBox;
class OHQProcessRunner;
class SimpleLinePlotWidget;
struct StarterScriptOptions;

class ModelCreatorWindow : public QMainWindow
{
    Q_OBJECT

public:
    /// Main UI window for starter generation, OHQ execution, plotting, and export workflows.
    explicit ModelCreatorWindow(QWidget *parent = nullptr);

private slots:
    // File/folder selection helpers.
    void chooseExecutable();
    void chooseScript();
    void chooseWorkingDirectory();
    void chooseArtifactsDirectory();
    void chooseTemplateDirectory();
    void chooseGeneratedScriptPath();
    void chooseGuiConfigTemplate();
    void chooseInflowFile();
    void chooseObservationFile();
    void chooseDepthProfileFile();
    void chooseVnBaseOhqFile();
    void chooseVnSoilLayersFile();
    void chooseVnMoistureLayersFile();
    void chooseVnSoftSoilParameterFile();
    void showVnReferenceDefaultsTable();
    void loadAdditionalCommandsFromFile();
    void applySuggestedDefaults();
    void quickGenerateRunAndSave();

    // Generation/run actions.
    void syncEnrichmentPresetForModel();
    void previewScript();
    void generateStarterScript();
    void generateAndRunStarterScript();
    void runScript();

    // Analysis/export actions.
    void refreshPlots();
    void exportArtifacts();
    void compareOutputVsObservation();
    void loadOutputParams();
    void updateOutputPlotFromSelection();
    void computeDepthProfileFromOutput();
    void exportPlotDataCsv();
    void exportAllDepthSlicesCsv();
    void clearComparisonHistory();
    void exportVnSoilProfileCsv();
    void exportVnDepthSliceCsv();
    void exportVnMetadataJson();
    void exportVnErtSnapshotCsv();
    void exportVtkInventoryCsv();
    void saveVnGeneratedFieldFile();
    void useVnGeneratedFieldFile();

private:
    void suggestSimulationWindowFromInflow(const QString &path, bool forceApply = false);
    /// Shared starter-generation implementation used by Generate and Generate + Run.
    bool generateStarterScriptInternal();
    /// Parse a numeric series from text/csv-like file formats into points.
    QVector<QPointF> loadSeriesFromFile(const QString &path, QString *errorMessage) const;
    /// Load numeric output columns from configured OHQ output file.
    bool loadOutputColumns(QString *errorMessage);
    /// Compute interpolated depth-slice series at @p targetX for selected columns.
    QVector<QPointF> computeDepthSliceSeries(double targetX, int xIdx, int yIdx, int depthIdx) const;
    /// Append latest comparison metrics snapshot to history file (if enabled).
    void appendComparisonHistory() const;
    /// Discover exportable files under current working directory tree.
    QStringList collectExportArtifacts() const;
    /// Append plain text lines to UI log pane.
    void appendLog(const QString &text);
    /// Restore persisted user settings into UI controls.
    void loadSettings();
    /// Persist current UI selections/paths.
    void saveSettings() const;
    /// Discover files modified during/after current run window.
    QStringList collectRunArtifacts() const;
    /// Copy discovered artifacts into configured artifacts directory.
    void copyArtifacts(const QStringList &artifacts);
    /// Write manifest CSV for copied/discovered artifacts.
    void writeArtifactManifest(const QStringList &artifacts);
    /// Show/hide context-sensitive and optional setup rows based on model/preset.
    void updateFieldVisibilityForContext();
    bool validateVnAwarenessInputs(QString *errorMessage, bool forRun) const;
    bool writeVnMetadataJson(const QString &targetPath, QString *errorMessage = nullptr) const;
    QString currentEffectiveVnInitTheta() const;
    QString currentEffectiveVnFieldMode() const;
    QString currentEffectiveVnFieldPoints() const;
    QString currentEffectiveVnFieldSeed() const;
    QString currentEffectiveVnFieldDx() const;
    QString currentEffectiveVnFieldPdf() const;
    QString currentEffectiveKsatAll() const;
    QString currentEffectiveKsatG() const;
    QString currentEffectiveKsatUw() const;
    QString defaultVnGeneratedFieldFilePath() const;
    void syncVnToolDefaultPaths();
    bool writeVnGeneratedFieldFile(const QString &targetPath, QString *errorMessage = nullptr) const;

    QComboBox *modelTypeCombo;
    QComboBox *workflowModeCombo;
    QLineEdit *exePathEdit;
    /// Optional executable argument template (supports {script} token).
    QLineEdit *exeArgsEdit;
    QLineEdit *guiConfigTemplateEdit;
    QLineEdit *scriptPathEdit;
    QLineEdit *workingDirEdit;
    QLineEdit *artifactsDirEdit;
    QLineEdit *templateDirEdit;
    QLineEdit *generatedScriptEdit;
    QComboBox *enrichmentPresetCombo;
    QLineEdit *inflowFileEdit;
    QLineEdit *simulationStartEdit;
    QLineEdit *simulationEndEdit;
    QLineEdit *ksatScaleEdit;
    QLineEdit *ksatScaleGEdit;
    QLineEdit *ksatScaleUwEdit;
    QLineEdit *outputSeriesFileEdit;
    QLineEdit *observationFileEdit;
    QLineEdit *depthProfileFileEdit;
    QLineEdit *vnBaseOhqFileEdit;
    QLineEdit *vnSoilLayersFileEdit;
    QLineEdit *vnMoistureLayersFileEdit;
    QComboBox *vnBuildModeCombo;
    QLineEdit *vnSoftGridXEdit;
    QLineEdit *vnSoftGridYEdit;
    QLineEdit *vnSoftUwGridXEdit;
    QLineEdit *vnSoftUwGridYEdit;
    QLineEdit *vnSoftCellSizeEdit;
    QLineEdit *vnSoftUwCellSizeEdit;
    QLineEdit *vnSoftGapSizeEdit;
    QLineEdit *vnSoftRwGEdit;
    QLineEdit *vnSoftRwUwEdit;
    QLineEdit *vnSoftRadiusInfluenceEdit;
    QLineEdit *vnSoftDepthWellCEdit;
    QLineEdit *vnSoftDepthWellGEdit;
    QLineEdit *vnSoftDepthToGwEdit;
    QLineEdit *vnSoftTopElevationEdit;
    QLineEdit *vnSoftLayerThicknessEdit;
    QLineEdit *vnSoftSoilKsatOriginalEdit;
    QLineEdit *vnSoftSoilAlphaEdit;
    QLineEdit *vnSoftSoilNEdit;
    QLineEdit *vnSoftSoilThetaSatEdit;
    QLineEdit *vnSoftSoilThetaResEdit;
    QComboBox *vnSoftSoilParamModeCombo;
    QLineEdit *vnSoftSoilParameterFileEdit;
    QComboBox *vnInitThetaModeCombo;
    QLineEdit *vnFieldPointsEdit;
    QLineEdit *vnFieldSeedEdit;
    QLineEdit *vnFieldDxEdit;
    QComboBox *vnFieldPdfModeCombo;
    QLineEdit *vnSoilProfileExportEdit;
    QLineEdit *vnDepthSliceExportEdit;
    QLineEdit *vnErtSnapshotExportEdit;
    QLineEdit *vtkInventoryExportEdit;
    QLineEdit *observationObjectEdit;
    QLineEdit *observationExpressionEdit;
    QLineEdit *observationNameEdit;
    QTextEdit *additionalCommandsEdit;
    QCheckBox *showOptionalFieldsCheck;
    QCheckBox *allowGuiExecutionCheck;
    QWidget *inflowRowWidget = nullptr;
    QWidget *modelTypeRowWidget = nullptr;
    QWidget *presetRowWidget = nullptr;
    QWidget *templateDirRowWidget = nullptr;
    QWidget *generatedScriptRowWidget = nullptr;
    QWidget *guiConfigTemplateRowWidget = nullptr;
    QWidget *simulationStartRowWidget = nullptr;
    QWidget *simulationEndRowWidget = nullptr;
    QWidget *outputSeriesRowWidget = nullptr;
    QWidget *observationFileRowWidget = nullptr;
    QWidget *depthProfileRowWidget = nullptr;
    QWidget *observationObjectRowWidget = nullptr;
    QWidget *observationExpressionRowWidget = nullptr;
    QWidget *observationNameRowWidget = nullptr;
    QWidget *additionalCommandsRowWidget = nullptr;
    QWidget *vnBaseRowWidget = nullptr;
    QWidget *vnSoilRowWidget = nullptr;
    QWidget *vnMoistureRowWidget = nullptr;
    QWidget *vnBuildModeRowWidget = nullptr;
    QWidget *vnSoftGridXRowWidget = nullptr;
    QWidget *vnSoftGridYRowWidget = nullptr;
    QWidget *vnSoftUwGridXRowWidget = nullptr;
    QWidget *vnSoftUwGridYRowWidget = nullptr;
    QWidget *vnSoftCellSizeRowWidget = nullptr;
    QWidget *vnSoftUwCellSizeRowWidget = nullptr;
    QWidget *vnSoftGapSizeRowWidget = nullptr;
    QWidget *vnSoftRadiusRowWidget = nullptr;
    QWidget *vnSoftDepthRowWidget = nullptr;
    QWidget *vnSoftTopElevationRowWidget = nullptr;
    QWidget *vnSoftLayerThicknessRowWidget = nullptr;
    QWidget *vnSoftSoilParamsRowWidget = nullptr;
    QWidget *vnInitThetaRowWidget = nullptr;
    QWidget *vnFieldGeneratorRowWidget = nullptr;
    QWidget *vnSoilToolRowWidget = nullptr;
    QWidget *vnOutputToolRowWidget = nullptr;
    QTabWidget *tabs;
    QTextEdit *logView;
    SimpleLinePlotWidget *inflowPlot;
    SimpleLinePlotWidget *outputPlot;
    SimpleLinePlotWidget *observationPlot;
    SimpleLinePlotWidget *depthProfilePlot;
    QPushButton *refreshPlotsButton;
    QPushButton *compareButton;
    QPushButton *reloadOutputColumnsButton;
    QPushButton *computeDepthSliceButton;
    QPushButton *exportPlotDataButton;
    QPushButton *exportAllDepthSlicesButton;
    QPushButton *clearComparisonHistoryButton;
    QComboBox *outputXAxisCombo;
    QComboBox *outputYAxisCombo;
    QComboBox *depthColumnCombo;
    QLineEdit *sliceXEdit;
    QLabel *comparisonSummaryLabel;
    QPushButton *previewScriptButton;
    QPushButton *quickRunButton;
    QPushButton *generateScriptButton;
    QPushButton *generateAndRunButton;
    QPushButton *runButton;
    QPushButton *exportArtifactsButton;
    QPushButton *stopButton;
    QPushButton *exportVnSoilProfileButton;
    QPushButton *exportVnDepthSliceButton;
    QPushButton *exportVnMetadataButton;
    QPushButton *exportVnErtSnapshotButton;
    QPushButton *exportVtkInventoryButton;
    QPushButton *saveVnGeneratedFieldButton;
    QPushButton *useVnGeneratedFieldButton;
    OHQProcessRunner *runner;
    /// Timestamp captured when a run begins (used for artifact recency checks).
    QDateTime runStartedAt;
    QVector<QVector<double>> outputNumericColumns;
    QStringList outputNumericHeaders;
    bool lastComparisonValid = false;
    int lastComparisonN = 0;
    double lastComparisonRmse = 0.0;
    double lastComparisonMae = 0.0;
    double lastComparisonBias = 0.0;
    double lastComparisonR2 = 0.0;
    mutable QString lastComparisonHistorySignature;
    /// Raw (filtered) process output accumulated during current run.
    QString currentRunOutput;
    /// Count of known non-actionable runtime warning lines suppressed in UI log.
    int suppressedRuntimeNoiseLines = 0;
    /// Auto-retry argument candidates for GUI executable launches.
    QList<QStringList> pendingGuiRetryArgs;
    QString pendingGuiRetryScript;
    QString pendingGuiRetryWorkingDirectory;
    QString pendingGuiRetryExecutable;
    QString lastSelectedModelType;
    bool solveProgressObserved = false;
    bool simulationWindowAutoSuggested = true;
    bool inflowAutoSuggested = true;
};

#endif // MODELCREATORWINDOW_H
