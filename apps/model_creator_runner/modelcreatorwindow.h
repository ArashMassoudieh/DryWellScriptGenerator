#ifndef MODELCREATORWINDOW_H
#define MODELCREATORWINDOW_H

#include <QDateTime>
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
class OHQProcessRunner;
class SimpleLinePlotWidget;

class ModelCreatorWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ModelCreatorWindow(QWidget *parent = nullptr);

private slots:
    void chooseExecutable();
    void chooseScript();
    void chooseWorkingDirectory();
    void chooseArtifactsDirectory();
    void chooseTemplateDirectory();
    void chooseGeneratedScriptPath();
    void chooseInflowFile();
    void chooseObservationFile();
    void chooseDepthProfileFile();
    void loadAdditionalCommandsFromFile();
    void syncEnrichmentPresetForModel();
    void previewScript();
    void generateStarterScript();
    void generateAndRunStarterScript();
    void runScript();
    void refreshPlots();
    void exportArtifacts();
    void compareOutputVsObservation();
    void loadOutputParams();
    void updateOutputPlotFromSelection();
    void computeDepthProfileFromOutput();
    void exportPlotDataCsv();
    void exportAllDepthSlicesCsv();
    void clearComparisonHistory();

private:
    bool generateStarterScriptInternal();
    QVector<QPointF> loadSeriesFromFile(const QString &path, QString *errorMessage) const;
    bool loadOutputColumns(QString *errorMessage);
    QVector<QPointF> computeDepthSliceSeries(double targetX, int xIdx, int yIdx, int depthIdx) const;
    void appendComparisonHistory() const;
    QStringList collectExportArtifacts() const;
    void appendLog(const QString &text);
    void loadSettings();
    void saveSettings() const;
    QStringList collectRunArtifacts() const;
    void copyArtifacts(const QStringList &artifacts);
    void writeArtifactManifest(const QStringList &artifacts);

    QComboBox *modelTypeCombo;
    QLineEdit *exePathEdit;
    QLineEdit *scriptPathEdit;
    QLineEdit *workingDirEdit;
    QLineEdit *artifactsDirEdit;
    QLineEdit *templateDirEdit;
    QLineEdit *generatedScriptEdit;
    QComboBox *enrichmentPresetCombo;
    QLineEdit *inflowFileEdit;
    QLineEdit *simulationStartEdit;
    QLineEdit *simulationEndEdit;
    QLineEdit *outputSeriesFileEdit;
    QLineEdit *observationFileEdit;
    QLineEdit *depthProfileFileEdit;
    QLineEdit *observationObjectEdit;
    QLineEdit *observationExpressionEdit;
    QLineEdit *observationNameEdit;
    QTextEdit *additionalCommandsEdit;
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
    QPushButton *generateScriptButton;
    QPushButton *generateAndRunButton;
    QPushButton *runButton;
    QPushButton *exportArtifactsButton;
    QPushButton *stopButton;
    OHQProcessRunner *runner;
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
};

#endif // MODELCREATORWINDOW_H
