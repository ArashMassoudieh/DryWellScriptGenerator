#include "modelcreatorwindow.h"

#include "ohqprocessrunner.h"
#include "simplelineplotwidget.h"
#include "starter_script_builder.h"
#include "scripteditordialog.h"

#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QTabWidget>
#include <QTextStream>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <cmath>

namespace {
QString stamp(const QString &message)
{
    return QString("[%1] %2")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODate), message);
}

bool InterpolateY(const QVector<QPointF> &series, double x, double *yOut)
{
    if (series.size() < 2 || yOut == nullptr) {
        return false;
    }

    QVector<QPointF> sorted = series;
    std::sort(sorted.begin(), sorted.end(), [](const QPointF &a, const QPointF &b) {
        return a.x() < b.x();
    });

    if (x < sorted.first().x() || x > sorted.last().x()) {
        return false;
    }

    for (int i = 1; i < sorted.size(); ++i) {
        const double x0 = sorted[i - 1].x();
        const double x1 = sorted[i].x();
        if (x >= x0 && x <= x1) {
            if (x1 == x0) {
                *yOut = sorted[i].y();
                return true;
            }
            const double y0 = sorted[i - 1].y();
            const double y1 = sorted[i].y();
            const double w = (x - x0) / (x1 - x0);
            *yOut = y0 + w * (y1 - y0);
            return true;
        }
    }

    return false;
}

bool InterpolateYSorted(const QVector<QPointF> &sortedSeries, double x, double *yOut)
{
    if (sortedSeries.size() < 2 || yOut == nullptr) {
        return false;
    }

    if (x < sortedSeries.first().x() || x > sortedSeries.last().x()) {
        return false;
    }

    for (int i = 1; i < sortedSeries.size(); ++i) {
        const double x0 = sortedSeries[i - 1].x();
        const double x1 = sortedSeries[i].x();
        if (x >= x0 && x <= x1) {
            if (x1 == x0) {
                *yOut = sortedSeries[i].y();
                return true;
            }
            const double y0 = sortedSeries[i - 1].y();
            const double y1 = sortedSeries[i].y();
            const double w = (x - x0) / (x1 - x0);
            *yOut = y0 + w * (y1 - y0);
            return true;
        }
    }

    return false;
}
}

ModelCreatorWindow::ModelCreatorWindow(QWidget *parent)
    : QMainWindow(parent),
      modelTypeCombo(new QComboBox(this)),
      exePathEdit(new QLineEdit(this)),
      scriptPathEdit(new QLineEdit(this)),
      workingDirEdit(new QLineEdit(this)),
      artifactsDirEdit(new QLineEdit(this)),
      templateDirEdit(new QLineEdit(this)),
      generatedScriptEdit(new QLineEdit(this)),
      enrichmentPresetCombo(new QComboBox(this)),
      inflowFileEdit(new QLineEdit(this)),
      simulationStartEdit(new QLineEdit(this)),
      simulationEndEdit(new QLineEdit(this)),
      outputSeriesFileEdit(new QLineEdit(this)),
      observationFileEdit(new QLineEdit(this)),
      depthProfileFileEdit(new QLineEdit(this)),
      observationObjectEdit(new QLineEdit(this)),
      observationExpressionEdit(new QLineEdit(this)),
      observationNameEdit(new QLineEdit(this)),
      additionalCommandsEdit(new QTextEdit(this)),
      tabs(new QTabWidget(this)),
      logView(new QTextEdit(this)),
      inflowPlot(new SimpleLinePlotWidget(tr("Inflow"), this)),
      outputPlot(new SimpleLinePlotWidget(tr("Output series"), this)),
      observationPlot(new SimpleLinePlotWidget(tr("Observations"), this)),
      depthProfilePlot(new SimpleLinePlotWidget(tr("Depth moisture profile"), this)),
      refreshPlotsButton(new QPushButton(tr("Refresh plots"), this)),
      compareButton(new QPushButton(tr("Compare output vs observations"), this)),
      reloadOutputColumnsButton(new QPushButton(tr("Load output params"), this)),
      computeDepthSliceButton(new QPushButton(tr("Build depth slice"), this)),
      exportPlotDataButton(new QPushButton(tr("Export plot data"), this)),
      exportAllDepthSlicesButton(new QPushButton(tr("Export all depth slices"), this)),
      clearComparisonHistoryButton(new QPushButton(tr("Clear history"), this)),
      outputXAxisCombo(new QComboBox(this)),
      outputYAxisCombo(new QComboBox(this)),
      depthColumnCombo(new QComboBox(this)),
      sliceXEdit(new QLineEdit(this)),
      comparisonSummaryLabel(new QLabel(tr("Comparison: n/a"), this)),
      previewScriptButton(new QPushButton(tr("Review/Edit .ohq"), this)),
      generateScriptButton(new QPushButton(tr("Generate starter .ohq"), this)),
      generateAndRunButton(new QPushButton(tr("Generate + Run"), this)),
      runButton(new QPushButton(tr("Run selected .ohq"), this)),
      exportArtifactsButton(new QPushButton(tr("Export run artifacts"), this)),
      stopButton(new QPushButton(tr("Stop"), this)),
      runner(new OHQProcessRunner(this))
{
    auto *central = new QWidget(this);
    auto *centralLayout = new QVBoxLayout(central);
    centralLayout->addWidget(tabs);

    auto *runTab = new QWidget(this);
    auto *layout = new QVBoxLayout(runTab);
    tabs->addTab(runTab, tr("Setup + Run"));

    modelTypeCombo->addItems({"Drywell", "Bioswale"});
    enrichmentPresetCombo->addItem(tr("None"), "");
    enrichmentPresetCombo->addItem(tr("Drywell + Monitoring Well"), "Drywell_MonitoringWell");
    enrichmentPresetCombo->addItem(tr("Drywell + Groundwater Boundary"), "Drywell_GroundwaterBoundary");
    enrichmentPresetCombo->addItem(tr("Drywell + Pretreatment Chambers"), "Drywell_PretreatmentChambers");
    enrichmentPresetCombo->addItem(tr("Drywell (Legacy ScriptGenerator style)"), "Drywell_LegacyStyle");
    enrichmentPresetCombo->addItem(tr("Bioswale + Underdrain"), "Bioswale_Underdrain");
    enrichmentPresetCombo->addItem(tr("Bioswale + Underdrain + Groundwater"), "Bioswale_Underdrain_GW");
    enrichmentPresetCombo->addItem(tr("Bioswale (DryWellSuite style)"), "Bioswale_SuiteStyle");

    auto addFileRow = [](QVBoxLayout *targetLayout, const QString &labelText, QLineEdit *edit, const QString &buttonText, auto slot) {
        auto *row = new QHBoxLayout();
        row->addWidget(new QLabel(labelText));
        row->addWidget(edit, 1);
        auto *btn = new QPushButton(buttonText);
        QObject::connect(btn, &QPushButton::clicked, slot);
        row->addWidget(btn);
        targetLayout->addLayout(row);
    };

    auto addTextRow = [](QVBoxLayout *targetLayout, const QString &labelText, QWidget *editor) {
        auto *row = new QHBoxLayout();
        row->addWidget(new QLabel(labelText));
        row->addWidget(editor, 1);
        targetLayout->addLayout(row);
    };

    addTextRow(layout, tr("Model type"), modelTypeCombo);
    addTextRow(layout, tr("Model enrichment preset"), enrichmentPresetCombo);
    addFileRow(layout, tr("OHQ executable"), exePathEdit, tr("Browse"), [this]() { chooseExecutable(); });
    addFileRow(layout, tr("OHQ script (.ohq)"), scriptPathEdit, tr("Browse"), [this]() { chooseScript(); });
    scriptPathEdit->setToolTip(tr("Select an existing .ohq file if you want to run without generating a new starter script."));
    addFileRow(layout, tr("Working directory"), workingDirEdit, tr("Browse"), [this]() { chooseWorkingDirectory(); });
    addFileRow(layout, tr("Artifacts directory"), artifactsDirEdit, tr("Browse"), [this]() { chooseArtifactsDirectory(); });
    addFileRow(layout, tr("Template resources dir"), templateDirEdit, tr("Browse"), [this]() { chooseTemplateDirectory(); });
    addFileRow(layout, tr("Generated script path"), generatedScriptEdit, tr("Browse"), [this]() { chooseGeneratedScriptPath(); });
    addFileRow(layout, tr("Inflow file"), inflowFileEdit, tr("Browse"), [this]() { chooseInflowFile(); });
    addTextRow(layout, tr("Simulation start"), simulationStartEdit);
    addTextRow(layout, tr("Simulation end"), simulationEndEdit);
    addTextRow(layout, tr("Output series file"), outputSeriesFileEdit);
    addFileRow(layout, tr("Observation file (optional)"), observationFileEdit, tr("Browse"), [this]() { chooseObservationFile(); });
    addFileRow(layout, tr("Depth profile file (optional)"), depthProfileFileEdit, tr("Browse"), [this]() { chooseDepthProfileFile(); });
    observationObjectEdit->setPlaceholderText(tr("e.g. Soil (1$1)"));
    observationObjectEdit->setToolTip(tr("Target soil/layer object used for observation extraction in generated script."));
    observationExpressionEdit->setPlaceholderText(tr("e.g. theta"));
    observationExpressionEdit->setToolTip(tr("Observed quantity/expression, e.g. moisture variable theta."));
    observationNameEdit->setPlaceholderText(tr("e.g. Obs_1"));
    addTextRow(layout, tr("Soil layer/object (observation target)"), observationObjectEdit);
    addTextRow(layout, tr("Moisture/expression (observation quantity)"), observationExpressionEdit);
    addTextRow(layout, tr("Observation series name"), observationNameEdit);
    additionalCommandsEdit->setPlaceholderText(tr("Optional additional OHQ commands, one per line..."));
    auto *additionalRow = new QHBoxLayout();
    additionalRow->addWidget(new QLabel(tr("Additional OHQ commands")));
    additionalRow->addWidget(additionalCommandsEdit, 1);
    auto *loadCommandsButton = new QPushButton(tr("Load file"), this);
    connect(loadCommandsButton, &QPushButton::clicked, this, &ModelCreatorWindow::loadAdditionalCommandsFromFile);
    additionalRow->addWidget(loadCommandsButton);
    layout->addLayout(additionalRow);

    auto *runSectionLabel = new QLabel(tr("OHQ run controls"), this);
    QFont runSectionFont = runSectionLabel->font();
    runSectionFont.setBold(true);
    runSectionLabel->setFont(runSectionFont);
    layout->addWidget(runSectionLabel);
    auto *actions = new QHBoxLayout();
    actions->addWidget(previewScriptButton);
    actions->addWidget(generateScriptButton);
    actions->addWidget(generateAndRunButton);
    actions->addWidget(runButton);
    actions->addWidget(stopButton);
    layout->addLayout(actions);
    layout->addWidget(new QLabel(tr("Tip: You can load an existing .ohq script and click \"Run selected .ohq\" without generating a starter script."), this));

    logView->setReadOnly(true);
    layout->addWidget(logView, 1);

    auto *plotsTab = new QWidget(this);
    auto *plotsLayout = new QVBoxLayout(plotsTab);
    auto *plotActions = new QHBoxLayout();
    plotActions->addWidget(compareButton);
    plotActions->addWidget(reloadOutputColumnsButton);
    plotActions->addWidget(new QLabel(tr("X:"), this));
    plotActions->addWidget(outputXAxisCombo);
    plotActions->addWidget(new QLabel(tr("Y:"), this));
    plotActions->addWidget(outputYAxisCombo);
    plotActions->addWidget(new QLabel(tr("Depth:"), this));
    plotActions->addWidget(depthColumnCombo);
    plotActions->addWidget(new QLabel(tr("Slice X/R:"), this));
    sliceXEdit->setPlaceholderText(tr("e.g. 0.5"));
    sliceXEdit->setMaximumWidth(120);
    plotActions->addWidget(sliceXEdit);
    plotActions->addWidget(computeDepthSliceButton);
    plotActions->addWidget(refreshPlotsButton);
    plotsLayout->addLayout(plotActions);
    comparisonSummaryLabel->setWordWrap(true);
    plotsLayout->addWidget(comparisonSummaryLabel);
    plotsLayout->addWidget(inflowPlot, 1);
    plotsLayout->addWidget(outputPlot, 1);
    plotsLayout->addWidget(observationPlot, 1);
    plotsLayout->addWidget(depthProfilePlot, 1);
    tabs->addTab(plotsTab, tr("Plots"));

    auto *exportTab = new QWidget(this);
    auto *exportLayout = new QVBoxLayout(exportTab);
    auto *exportLabel = new QLabel(tr("Export artifacts and analysis outputs"), this);
    QFont exportFont = exportLabel->font();
    exportFont.setBold(true);
    exportLabel->setFont(exportFont);
    exportLayout->addWidget(exportLabel);

    auto *exportActions = new QHBoxLayout();
    exportActions->addWidget(exportArtifactsButton);
    exportActions->addWidget(exportPlotDataButton);
    exportActions->addWidget(exportAllDepthSlicesButton);
    exportActions->addWidget(clearComparisonHistoryButton);
    exportLayout->addLayout(exportActions);
    exportLayout->addStretch(1);
    tabs->addTab(exportTab, tr("Export"));

    setCentralWidget(central);
    setWindowTitle(tr("Model Creator Runner (New Workflow)"));
    resize(1100, 700);

    stopButton->setEnabled(false);

    connect(previewScriptButton, &QPushButton::clicked, this, &ModelCreatorWindow::previewScript);
    connect(generateScriptButton, &QPushButton::clicked, this, &ModelCreatorWindow::generateStarterScript);
    connect(generateAndRunButton, &QPushButton::clicked, this, &ModelCreatorWindow::generateAndRunStarterScript);
    connect(runButton, &QPushButton::clicked, this, &ModelCreatorWindow::runScript);
    connect(exportArtifactsButton, &QPushButton::clicked, this, &ModelCreatorWindow::exportArtifacts);
    connect(stopButton, &QPushButton::clicked, runner, &OHQProcessRunner::stop);
    connect(refreshPlotsButton, &QPushButton::clicked, this, &ModelCreatorWindow::refreshPlots);
    connect(compareButton, &QPushButton::clicked, this, &ModelCreatorWindow::compareOutputVsObservation);
    connect(reloadOutputColumnsButton, &QPushButton::clicked, this, &ModelCreatorWindow::loadOutputParams);
    connect(computeDepthSliceButton, &QPushButton::clicked, this, &ModelCreatorWindow::computeDepthProfileFromOutput);
    connect(exportPlotDataButton, &QPushButton::clicked, this, &ModelCreatorWindow::exportPlotDataCsv);
    connect(exportAllDepthSlicesButton, &QPushButton::clicked, this, &ModelCreatorWindow::exportAllDepthSlicesCsv);
    connect(clearComparisonHistoryButton, &QPushButton::clicked, this, &ModelCreatorWindow::clearComparisonHistory);
    connect(outputXAxisCombo, &QComboBox::currentTextChanged, this, &ModelCreatorWindow::updateOutputPlotFromSelection);
    connect(outputYAxisCombo, &QComboBox::currentTextChanged, this, &ModelCreatorWindow::updateOutputPlotFromSelection);
    connect(depthColumnCombo, &QComboBox::currentTextChanged, this, &ModelCreatorWindow::computeDepthProfileFromOutput);
    connect(sliceXEdit, &QLineEdit::editingFinished, this, &ModelCreatorWindow::computeDepthProfileFromOutput);
    connect(outputSeriesFileEdit, &QLineEdit::editingFinished, this, &ModelCreatorWindow::refreshPlots);
    connect(workingDirEdit, &QLineEdit::editingFinished, this, &ModelCreatorWindow::refreshPlots);
    connect(depthProfileFileEdit, &QLineEdit::editingFinished, this, &ModelCreatorWindow::refreshPlots);
    connect(modelTypeCombo, &QComboBox::currentTextChanged, this, &ModelCreatorWindow::syncEnrichmentPresetForModel);
    connect(modelTypeCombo, &QComboBox::currentTextChanged, this, [this]() { saveSettings(); });
    connect(enrichmentPresetCombo, &QComboBox::currentTextChanged, this, [this]() { saveSettings(); });

    const auto saveOnEdit = [this](QLineEdit *edit) {
        connect(edit, &QLineEdit::editingFinished, this, [this]() { saveSettings(); });
    };
    saveOnEdit(exePathEdit);
    saveOnEdit(scriptPathEdit);
    saveOnEdit(workingDirEdit);
    saveOnEdit(artifactsDirEdit);
    saveOnEdit(templateDirEdit);
    saveOnEdit(generatedScriptEdit);
    saveOnEdit(inflowFileEdit);
    saveOnEdit(simulationStartEdit);
    saveOnEdit(simulationEndEdit);
    saveOnEdit(outputSeriesFileEdit);
    saveOnEdit(observationFileEdit);
    saveOnEdit(depthProfileFileEdit);
    saveOnEdit(observationObjectEdit);
    saveOnEdit(observationExpressionEdit);
    saveOnEdit(observationNameEdit);
    connect(additionalCommandsEdit, &QTextEdit::textChanged, this, [this]() { saveSettings(); });

    connect(runner, &OHQProcessRunner::runStarted, this, [this]() {
        runStartedAt = QDateTime::currentDateTime();
        currentRunOutput.clear();
        previewScriptButton->setEnabled(false);
        generateScriptButton->setEnabled(false);
        generateAndRunButton->setEnabled(false);
        runButton->setEnabled(false);
        exportArtifactsButton->setEnabled(false);
        stopButton->setEnabled(true);
        appendLog(stamp(tr("Run started.")));
    });

    connect(runner, &OHQProcessRunner::outputReady, this, [this](const QString &text) {
        currentRunOutput += text;
        appendLog(text);
    });

    connect(runner, &OHQProcessRunner::runFinished, this, [this](int exitCode) {
        previewScriptButton->setEnabled(true);
        generateScriptButton->setEnabled(true);
        generateAndRunButton->setEnabled(true);
        runButton->setEnabled(true);
        exportArtifactsButton->setEnabled(true);
        stopButton->setEnabled(false);
        appendLog(stamp(tr("Run finished with exit code %1").arg(exitCode)));
        if (exitCode != 0) {
            if (currentRunOutput.contains("error while loading shared libraries", Qt::CaseInsensitive)) {
                QMessageBox::warning(this,
                                     tr("Runtime dependency error"),
                                     tr("OHQ failed to start due to missing shared libraries.\n\n"
                                        "Details:\n%1\n\n"
                                        "Please ensure required runtime libraries (e.g., VTK) are available via LD_LIBRARY_PATH or system linker paths.")
                                         .arg(currentRunOutput.trimmed()));
                appendLog(stamp(tr("Detected shared-library runtime error; artifact scan skipped.")));
            } else {
                appendLog(stamp(tr("Run exited with non-zero code; artifact scan skipped.")));
            }
            return;
        }

        refreshPlots();

        const QStringList artifacts = collectRunArtifacts();
        if (artifacts.isEmpty()) {
            appendLog(stamp(tr("No new artifacts detected in working directory.")));
            return;
        }

        appendLog(stamp(tr("Detected %1 artifact file(s):").arg(artifacts.count())));
        for (const QString &file : artifacts) {
            appendLog(QStringLiteral("  - %1").arg(file));
        }

        copyArtifacts(artifacts);
        writeArtifactManifest(artifacts);
    });

    connect(runner, &OHQProcessRunner::runFailed, this, [this](const QString &reason) {
        previewScriptButton->setEnabled(true);
        generateScriptButton->setEnabled(true);
        generateAndRunButton->setEnabled(true);
        runButton->setEnabled(true);
        exportArtifactsButton->setEnabled(true);
        stopButton->setEnabled(false);
        QMessageBox::warning(this, tr("Run failed"), reason);
        appendLog(stamp(tr("Run failed: %1").arg(reason)));
    });

    loadSettings();
    syncEnrichmentPresetForModel();
    refreshPlots();
}

void ModelCreatorWindow::syncEnrichmentPresetForModel()
{
    const QString modelType = modelTypeCombo->currentText().trimmed();
    const QString preset = enrichmentPresetCombo->currentData().toString().trimmed();
    if (preset.isEmpty()) {
        return;
    }

    const bool drywellModel = modelType.compare(QStringLiteral("Drywell"), Qt::CaseInsensitive) == 0;
    const bool bioswaleModel = modelType.compare(QStringLiteral("Bioswale"), Qt::CaseInsensitive) == 0;
    const bool drywellPreset = preset.startsWith(QStringLiteral("Drywell_"));
    const bool bioswalePreset = preset.startsWith(QStringLiteral("Bioswale_"));

    if ((drywellModel && bioswalePreset) || (bioswaleModel && drywellPreset)) {
        enrichmentPresetCombo->setCurrentIndex(0);
        appendLog(stamp(tr("Preset '%1' is incompatible with model type '%2'; reset to None.")
                        .arg(preset, modelType)));
        QMessageBox::information(this,
                                 tr("Preset reset"),
                                 tr("The selected enrichment preset was not compatible with model type '%1' and was reset to None.")
                                     .arg(modelType));
        saveSettings();
    }
}

void ModelCreatorWindow::chooseExecutable()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Select OHQ executable"));
    if (!fileName.isEmpty()) {
        exePathEdit->setText(fileName);
        saveSettings();
    }
}

void ModelCreatorWindow::chooseScript()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Select OHQ script"), {}, tr("OHQ files (*.ohq);;All files (*.*)"));
    if (!fileName.isEmpty()) {
        scriptPathEdit->setText(fileName);
        const QFileInfo info(fileName);
        if (workingDirEdit->text().isEmpty()) {
            workingDirEdit->setText(info.absolutePath());
        }
        saveSettings();
    }
}

void ModelCreatorWindow::chooseWorkingDirectory()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Select working directory"));
    if (!dir.isEmpty()) {
        workingDirEdit->setText(dir);
        saveSettings();
    }
}

void ModelCreatorWindow::chooseArtifactsDirectory()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Select artifacts directory"));
    if (!dir.isEmpty()) {
        artifactsDirEdit->setText(dir);
        saveSettings();
    }
}

void ModelCreatorWindow::chooseTemplateDirectory()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Select OHQ template resources directory"));
    if (!dir.isEmpty()) {
        templateDirEdit->setText(dir);
        saveSettings();
    }
}

void ModelCreatorWindow::chooseGeneratedScriptPath()
{
    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          tr("Save generated starter script"),
                                                          generatedScriptEdit->text(),
                                                          tr("OHQ files (*.ohq);;All files (*.*)"));
    if (!fileName.isEmpty()) {
        generatedScriptEdit->setText(fileName);
        saveSettings();
    }
}

void ModelCreatorWindow::chooseInflowFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Select inflow file"),
                                                          inflowFileEdit->text(),
                                                          tr("Data files (*.csv *.txt);;All files (*.*)"));
    if (!fileName.isEmpty()) {
        inflowFileEdit->setText(fileName);
        saveSettings();
        refreshPlots();
    }
}

void ModelCreatorWindow::chooseObservationFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Select observation file"),
                                                          observationFileEdit->text(),
                                                          tr("Data files (*.csv *.txt);;All files (*.*)"));
    if (!fileName.isEmpty()) {
        observationFileEdit->setText(fileName);
        saveSettings();
        refreshPlots();
    }
}

void ModelCreatorWindow::chooseDepthProfileFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Select depth profile file"),
                                                          depthProfileFileEdit->text(),
                                                          tr("Data files (*.csv *.txt);;All files (*.*)"));
    if (!fileName.isEmpty()) {
        depthProfileFileEdit->setText(fileName);
        saveSettings();
        refreshPlots();
    }
}

void ModelCreatorWindow::loadAdditionalCommandsFromFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Load additional OHQ commands"),
                                                          {},
                                                          tr("Text files (*.txt *.ohq);;All files (*.*)"));
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Load additional commands"), tr("Could not read selected file."));
        return;
    }

    const QString text = QString::fromUtf8(file.readAll()).trimmed();
    if (text.isEmpty()) {
        QMessageBox::information(this, tr("Load additional commands"), tr("Selected file is empty."));
        return;
    }

    QString current = additionalCommandsEdit->toPlainText().trimmed();
    if (!current.isEmpty()) {
        current += "\n";
    }
    current += text;
    additionalCommandsEdit->setPlainText(current);
    saveSettings();
}

void ModelCreatorWindow::previewScript()
{
    StarterScriptOptions options;
    options.templateDirectory = templateDirEdit->text().trimmed();
    options.outputFile = generatedScriptEdit->text().trimmed();
    options.modelType = modelTypeCombo->currentText();
    options.enrichmentPreset = enrichmentPresetCombo->currentData().toString();
    options.inflowFile = inflowFileEdit->text().trimmed();
    options.simulationStart = simulationStartEdit->text().trimmed();
    options.simulationEnd = simulationEndEdit->text().trimmed();
    options.outputSeriesFile = outputSeriesFileEdit->text().trimmed();
    options.observationFile = observationFileEdit->text().trimmed();
    options.observationObject = observationObjectEdit->text().trimmed();
    options.observationExpression = observationExpressionEdit->text().trimmed();
    options.observationName = observationNameEdit->text().trimmed();
    options.additionalCommands = additionalCommandsEdit->toPlainText();

    QString scriptText;
    QString error;
    const bool canBuildDraft = StarterScriptBuilder::BuildText(options, &scriptText, &error);

    if (!canBuildDraft) {
        const QString filePath = scriptPathEdit->text().trimmed();
        if (filePath.isEmpty()) {
            QMessageBox::information(this, tr("No script available"), tr("Could not build draft from current inputs and no script file is selected.\nReason: %1").arg(error));
            return;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this,
                                 tr("Cannot open script"),
                                 tr("Failed to open script file for review.\nReason: %1")
                                     .arg(file.errorString()));
            return;
        }
        scriptText = QString::fromUtf8(file.readAll());
    }

    ScriptEditorDialog dialog(this);
    dialog.setScriptText(scriptText);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString targetFile = options.outputFile;
    if (targetFile.trimmed().isEmpty()) {
        targetFile = scriptPathEdit->text().trimmed();
    }
    if (targetFile.trimmed().isEmpty()) {
        targetFile = QFileDialog::getSaveFileName(this, tr("Save reviewed script"), {}, tr("OHQ files (*.ohq);;All files (*.*)"));
        if (targetFile.trimmed().isEmpty()) return;
    }

    QSaveFile out(targetFile);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Cannot save script"), tr("Failed to open output path for saving."));
        return;
    }
    QTextStream ts(&out);
    ts << dialog.scriptText();
    if (!out.commit()) {
        QMessageBox::warning(this, tr("Cannot save script"), tr("Failed to commit edited script to disk."));
        return;
    }

    scriptPathEdit->setText(targetFile);
    if (workingDirEdit->text().trimmed().isEmpty()) {
        workingDirEdit->setText(QFileInfo(targetFile).absolutePath());
    }
    saveSettings();
    appendLog(stamp(tr("Reviewed/edited script saved: %1").arg(targetFile)));
}

void ModelCreatorWindow::generateStarterScript()
{
    generateStarterScriptInternal();
}

void ModelCreatorWindow::generateAndRunStarterScript()
{
    if (generateStarterScriptInternal()) {
        runScript();
    }
}

bool ModelCreatorWindow::generateStarterScriptInternal()
{
    StarterScriptOptions options;
    options.templateDirectory = templateDirEdit->text().trimmed();
    options.outputFile = generatedScriptEdit->text().trimmed();
    options.modelType = modelTypeCombo->currentText();
    options.enrichmentPreset = enrichmentPresetCombo->currentData().toString();
    options.inflowFile = inflowFileEdit->text().trimmed();
    options.simulationStart = simulationStartEdit->text().trimmed();
    options.simulationEnd = simulationEndEdit->text().trimmed();
    options.outputSeriesFile = outputSeriesFileEdit->text().trimmed();
    options.observationFile = observationFileEdit->text().trimmed();
    options.observationObject = observationObjectEdit->text().trimmed();
    options.observationExpression = observationExpressionEdit->text().trimmed();
    options.observationName = observationNameEdit->text().trimmed();
    options.additionalCommands = additionalCommandsEdit->toPlainText();

    if (options.templateDirectory.isEmpty()) {
        QMessageBox::warning(this, tr("Missing template directory"), tr("Please select the OHQ template resources directory first."));
        return false;
    }

    if (options.outputFile.isEmpty()) {
        QMessageBox::warning(this, tr("Missing output file"), tr("Please choose where to save the generated .ohq script."));
        return false;
    }

    if (options.inflowFile.isEmpty()) {
        QMessageBox::warning(this, tr("Missing inflow file"), tr("Please select an inflow file (.csv/.txt)."));
        return false;
    }

    if (options.outputSeriesFile.isEmpty()) {
        QMessageBox::warning(this, tr("Missing output filename"), tr("Please provide the OHQ output series filename."));
        return false;
    }

    QString error;
    if (!StarterScriptBuilder::Write(options, &error)) {
        QMessageBox::warning(this, tr("Starter script generation failed"), error);
        return false;
    }

    scriptPathEdit->setText(options.outputFile);
    if (workingDirEdit->text().trimmed().isEmpty()) {
        workingDirEdit->setText(QFileInfo(options.outputFile).absolutePath());
    }
    saveSettings();

    appendLog(stamp(tr("Generated %1 starter script: %2").arg(options.modelType, options.outputFile)));
    return true;
}

void ModelCreatorWindow::runScript()
{
    if (runner->isRunning()) {
        QMessageBox::information(this, tr("Already running"), tr("A run is already in progress."));
        return;
    }

    const QFileInfo exeInfo(exePathEdit->text());
    const QFileInfo scriptInfo(scriptPathEdit->text());
    const QFileInfo wdInfo(workingDirEdit->text());

    if (!exeInfo.exists() || !exeInfo.isFile()) {
        QMessageBox::warning(this, tr("Missing executable"), tr("Please select a valid OHQ executable."));
        return;
    }

    if (!scriptInfo.exists() || !scriptInfo.isFile()) {
        QMessageBox::warning(this, tr("Missing script"), tr("Please select a valid .ohq script file."));
        return;
    }

    if (!wdInfo.exists() || !wdInfo.isDir()) {
        QMessageBox::warning(this, tr("Missing directory"), tr("Please select a valid working directory."));
        return;
    }

    if (!artifactsDirEdit->text().trimmed().isEmpty()) {
        const QFileInfo artifactsDirInfo(artifactsDirEdit->text());
        if (!artifactsDirInfo.exists() || !artifactsDirInfo.isDir()) {
            QMessageBox::warning(this, tr("Invalid artifacts directory"), tr("Please select a valid artifacts directory or leave it empty."));
            return;
        }
    }

    saveSettings();

    runner->setExecutablePath(exeInfo.absoluteFilePath());
    appendLog(stamp(tr("Running loaded script without generation: %1").arg(scriptInfo.absoluteFilePath())));
    runner->runScript(scriptInfo.absoluteFilePath(), wdInfo.absoluteFilePath());
}

QVector<QPointF> ModelCreatorWindow::loadSeriesFromFile(const QString &path, QString *errorMessage) const
{
    QVector<QPointF> points;
    QFile file(path);
    if (!file.exists()) {
        if (errorMessage) *errorMessage = tr("File not found: %1").arg(path);
        return points;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) *errorMessage = tr("Unable to open file: %1").arg(path);
        return points;
    }

    QTextStream ts(&file);
    int row = 0;
    while (!ts.atEnd()) {
        const QString line = ts.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        const QStringList tokens = line.split(QRegularExpression("[,;\\t ]+"), Qt::SkipEmptyParts);
        QVector<double> numericValues;
        for (const QString &token : tokens) {
            bool ok = false;
            const double value = token.toDouble(&ok);
            if (ok) {
                numericValues.push_back(value);
            }
        }

        if (numericValues.size() >= 2) {
            points.push_back(QPointF(numericValues[0], numericValues[1]));
            ++row;
        } else if (numericValues.size() == 1) {
            points.push_back(QPointF(row, numericValues[0]));
            ++row;
        }
    }

    if (points.isEmpty() && errorMessage) {
        *errorMessage = tr("No numeric data points found.");
    }
    return points;
}

void ModelCreatorWindow::refreshPlots()
{
    QString inflowError;
    const QVector<QPointF> inflowPoints = loadSeriesFromFile(inflowFileEdit->text().trimmed(), &inflowError);
    inflowPlot->setSeries(inflowPoints);
    inflowPlot->setStatusMessage(inflowError);

    QString outputPath = outputSeriesFileEdit->text().trimmed();
    if (!outputPath.isEmpty() && QFileInfo(outputPath).isRelative()) {
        outputPath = QDir(workingDirEdit->text().trimmed()).filePath(outputPath);
    }
    Q_UNUSED(outputPath);
    updateOutputPlotFromSelection();

    QString observationError;
    const QVector<QPointF> observationPoints = loadSeriesFromFile(observationFileEdit->text().trimmed(), &observationError);
    observationPlot->setSeries(observationPoints);
    observationPlot->setStatusMessage(observationError);

    QString depthProfilePath = depthProfileFileEdit->text().trimmed();
    if (!depthProfilePath.isEmpty() && QFileInfo(depthProfilePath).isRelative()) {
        depthProfilePath = QDir(workingDirEdit->text().trimmed()).filePath(depthProfilePath);
    }
    if (!depthProfilePath.isEmpty()) {
        QString depthError;
        const QVector<QPointF> depthPoints = loadSeriesFromFile(depthProfilePath, &depthError);
        depthProfilePlot->setSeries(depthPoints);
        depthProfilePlot->setStatusMessage(depthError);
    } else {
        computeDepthProfileFromOutput();
    }

    compareOutputVsObservation();
}

void ModelCreatorWindow::compareOutputVsObservation()
{
    QString outputPath = outputSeriesFileEdit->text().trimmed();
    if (!outputPath.isEmpty() && QFileInfo(outputPath).isRelative()) {
        outputPath = QDir(workingDirEdit->text().trimmed()).filePath(outputPath);
    }

    QString outputError;
    const QVector<QPointF> outputPoints = loadSeriesFromFile(outputPath, &outputError);

    QString observationError;
    const QVector<QPointF> observationPoints = loadSeriesFromFile(observationFileEdit->text().trimmed(), &observationError);

    if (outputPoints.isEmpty() || observationPoints.isEmpty()) {
        lastComparisonValid = false;
        comparisonSummaryLabel->setText(tr("Comparison: unavailable (%1 | %2)")
                                            .arg(outputError.isEmpty() ? tr("output missing") : outputError,
                                                 observationError.isEmpty() ? tr("observation missing") : observationError));
        return;
    }

    double sumSquared = 0.0;
    double sumAbs = 0.0;
    double sumBias = 0.0;
    double sumObs = 0.0;
    int n = 0;
    QVector<QPointF> sortedObs = observationPoints;
    std::sort(sortedObs.begin(), sortedObs.end(), [](const QPointF &a, const QPointF &b) {
        return a.x() < b.x();
    });
    for (const QPointF &pt : outputPoints) {
        double obsInterpolated = 0.0;
        if (!InterpolateYSorted(sortedObs, pt.x(), &obsInterpolated)) {
            continue;
        }
        const double residual = pt.y() - obsInterpolated;
        sumSquared += residual * residual;
        sumAbs += std::abs(residual);
        sumBias += residual;
        sumObs += obsInterpolated;
        ++n;
    }

    if (n < 2) {
        lastComparisonValid = false;
        comparisonSummaryLabel->setText(tr("Comparison: need at least 2 overlapping points on X-axis."));
        return;
    }

    const double obsMean = sumObs / static_cast<double>(n);

    double ssTot = 0.0;
    for (const QPointF &pt : outputPoints) {
        double obsInterpolated = 0.0;
        if (!InterpolateYSorted(sortedObs, pt.x(), &obsInterpolated)) {
            continue;
        }
        const double delta = obsInterpolated - obsMean;
        ssTot += delta * delta;
    }

    const double rmse = std::sqrt(sumSquared / static_cast<double>(n));
    const double mae = sumAbs / static_cast<double>(n);
    const double bias = sumBias / static_cast<double>(n);
    const double r2 = (ssTot > 0.0) ? (1.0 - (sumSquared / ssTot)) : 0.0;

    lastComparisonValid = true;
    lastComparisonN = n;
    lastComparisonRmse = rmse;
    lastComparisonMae = mae;
    lastComparisonBias = bias;
    lastComparisonR2 = r2;

    const QString summary = tr("Comparison (n=%1): RMSE=%2, MAE=%3, Bias=%4, R²=%5")
                                .arg(n)
                                .arg(rmse, 0, 'g', 6)
                                .arg(mae, 0, 'g', 6)
                                .arg(bias, 0, 'g', 6)
                                .arg(r2, 0, 'g', 6);
    comparisonSummaryLabel->setText(summary);
    appendComparisonHistory();
}

void ModelCreatorWindow::loadOutputParams()
{
    QString error;
    if (!loadOutputColumns(&error)) {
        appendLog(stamp(tr("Failed to load output params: %1").arg(error)));
        QMessageBox::warning(this, tr("Load output params"), error);
        outputPlot->setSeries({});
        outputPlot->setStatusMessage(error);
        return;
    }

    outputXAxisCombo->blockSignals(true);
    outputYAxisCombo->blockSignals(true);
    depthColumnCombo->blockSignals(true);
    outputXAxisCombo->clear();
    outputYAxisCombo->clear();
    depthColumnCombo->clear();
    outputXAxisCombo->addItems(outputNumericHeaders);
    outputYAxisCombo->addItems(outputNumericHeaders);
    depthColumnCombo->addItems(outputNumericHeaders);
    outputXAxisCombo->setCurrentIndex(0);
    outputYAxisCombo->setCurrentIndex(qMin(1, outputYAxisCombo->count() - 1));
    depthColumnCombo->setCurrentIndex(qMin(2, depthColumnCombo->count() - 1));
    outputXAxisCombo->blockSignals(false);
    outputYAxisCombo->blockSignals(false);
    depthColumnCombo->blockSignals(false);

    appendLog(stamp(tr("Loaded %1 output parameter columns from %2.")
                    .arg(outputNumericHeaders.size())
                    .arg(outputSeriesFileEdit->text().trimmed())));
    updateOutputPlotFromSelection();
}

bool ModelCreatorWindow::loadOutputColumns(QString *errorMessage)
{
    outputNumericColumns.clear();
    outputNumericHeaders.clear();

    QString outputPath = outputSeriesFileEdit->text().trimmed();
    if (!outputPath.isEmpty() && QFileInfo(outputPath).isRelative()) {
        outputPath = QDir(workingDirEdit->text().trimmed()).filePath(outputPath);
    }
    if (outputPath.isEmpty()) {
        if (errorMessage) *errorMessage = tr("Output series path is empty.");
        return false;
    }

    QFile file(outputPath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) *errorMessage = tr("Unable to open output series: %1").arg(outputPath);
        return false;
    }

    QTextStream ts(&file);
    QStringList headers;
    QVector<QVector<double>> cols;
    bool schemaInitialized = false;
    while (!ts.atEnd()) {
        const QString line = ts.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }
        const QStringList tokens = line.split(QRegularExpression("[,;\\t ]+"), Qt::SkipEmptyParts);
        if (tokens.isEmpty()) {
            continue;
        }

        if (!schemaInitialized) {
            bool allNumeric = true;
            for (const QString &token : tokens) {
                bool ok = false;
                token.toDouble(&ok);
                if (!ok) {
                    allNumeric = false;
                    break;
                }
            }
            if (!allNumeric) {
                headers = tokens;
                continue;
            }

            const int count = tokens.size();
            cols.resize(count);
            if (headers.isEmpty()) {
                for (int i = 0; i < count; ++i) {
                    headers << tr("col_%1").arg(i + 1);
                }
            }
            schemaInitialized = true;
        }

        if (tokens.size() != cols.size()) {
            continue;
        }

        bool rowValid = true;
        QVector<double> rowValues;
        rowValues.reserve(tokens.size());
        for (const QString &token : tokens) {
            bool ok = false;
            const double value = token.toDouble(&ok);
            if (!ok) {
                rowValid = false;
                break;
            }
            rowValues.push_back(value);
        }
        if (!rowValid) {
            continue;
        }

        for (int i = 0; i < cols.size(); ++i) {
            cols[i].push_back(rowValues[i]);
        }
    }

    if (cols.size() < 2) {
        if (errorMessage) *errorMessage = tr("Need at least two numeric output columns.");
        return false;
    }

    outputNumericColumns = cols;
    outputNumericHeaders = headers;
    return true;
}

void ModelCreatorWindow::updateOutputPlotFromSelection()
{
    QString error;
    if (!loadOutputColumns(&error)) {
        outputPlot->setSeries({});
        outputPlot->setStatusMessage(error);
        return;
    }

    if (outputXAxisCombo->count() != outputNumericHeaders.size()
        || outputYAxisCombo->count() != outputNumericHeaders.size()
        || depthColumnCombo->count() != outputNumericHeaders.size()) {
        outputXAxisCombo->blockSignals(true);
        outputYAxisCombo->blockSignals(true);
        depthColumnCombo->blockSignals(true);
        outputXAxisCombo->clear();
        outputYAxisCombo->clear();
        depthColumnCombo->clear();
        outputXAxisCombo->addItems(outputNumericHeaders);
        outputYAxisCombo->addItems(outputNumericHeaders);
        depthColumnCombo->addItems(outputNumericHeaders);
        outputXAxisCombo->setCurrentIndex(0);
        outputYAxisCombo->setCurrentIndex(qMin(1, outputYAxisCombo->count() - 1));
        depthColumnCombo->setCurrentIndex(qMin(2, depthColumnCombo->count() - 1));
        outputXAxisCombo->blockSignals(false);
        outputYAxisCombo->blockSignals(false);
        depthColumnCombo->blockSignals(false);
    }

    const int xIdx = outputXAxisCombo->currentIndex();
    const int yIdx = outputYAxisCombo->currentIndex();
    if (xIdx < 0 || yIdx < 0 || xIdx >= outputNumericColumns.size() || yIdx >= outputNumericColumns.size()) {
        outputPlot->setSeries({});
        outputPlot->setStatusMessage(tr("Select valid X/Y output columns."));
        return;
    }

    const QVector<double> &xCol = outputNumericColumns[xIdx];
    const QVector<double> &yCol = outputNumericColumns[yIdx];
    const int n = qMin(xCol.size(), yCol.size());
    QVector<QPointF> points;
    points.reserve(n);
    for (int i = 0; i < n; ++i) {
        points.push_back(QPointF(xCol[i], yCol[i]));
    }

    outputPlot->setSeries(points);
    outputPlot->setStatusMessage(tr("Plotting %1 vs %2 (%3 points)")
                                 .arg(outputNumericHeaders.value(yIdx),
                                      outputNumericHeaders.value(xIdx))
                                 .arg(points.size()));
    computeDepthProfileFromOutput();
}

void ModelCreatorWindow::computeDepthProfileFromOutput()
{
    if (outputNumericColumns.isEmpty()) {
        depthProfilePlot->setSeries({});
        depthProfilePlot->setStatusMessage(tr("Load output params first."));
        return;
    }

    const int xIdx = outputXAxisCombo->currentIndex();
    const int yIdx = outputYAxisCombo->currentIndex();
    const int depthIdx = depthColumnCombo->currentIndex();
    if (xIdx < 0 || yIdx < 0 || depthIdx < 0
        || xIdx >= outputNumericColumns.size()
        || yIdx >= outputNumericColumns.size()
        || depthIdx >= outputNumericColumns.size()) {
        depthProfilePlot->setSeries({});
        depthProfilePlot->setStatusMessage(tr("Select valid X/Y/Depth columns."));
        return;
    }

    bool ok = false;
    double targetX = sliceXEdit->text().trimmed().toDouble(&ok);
    const QVector<double> &xCol = outputNumericColumns[xIdx];
    const QVector<double> &dCol = outputNumericColumns[depthIdx];
    const QVector<double> &yCol = outputNumericColumns[yIdx];
    const int n = qMin(xCol.size(), qMin(dCol.size(), yCol.size()));
    if (n < 2) {
        depthProfilePlot->setSeries({});
        depthProfilePlot->setStatusMessage(tr("Need at least two rows for depth interpolation."));
        return;
    }

    if (!ok) {
        double minX = xCol[0];
        double maxX = xCol[0];
        for (double x : xCol) {
            minX = qMin(minX, x);
            maxX = qMax(maxX, x);
        }
        targetX = 0.5 * (minX + maxX);
        sliceXEdit->setText(QString::number(targetX, 'g', 6));
    }

    const QVector<QPointF> depthProfile = computeDepthSliceSeries(targetX, xIdx, yIdx, depthIdx);

    depthProfilePlot->setSeries(depthProfile);
    depthProfilePlot->setStatusMessage(tr("%1 through depth at %2=%3 (%4 depth points)")
                                       .arg(outputNumericHeaders.value(yIdx),
                                            outputNumericHeaders.value(xIdx),
                                            QString::number(targetX, 'g', 6))
                                       .arg(depthProfile.size()));
}

QVector<QPointF> ModelCreatorWindow::computeDepthSliceSeries(double targetX, int xIdx, int yIdx, int depthIdx) const
{
    QVector<QPointF> depthProfile;
    if (xIdx < 0 || yIdx < 0 || depthIdx < 0
        || xIdx >= outputNumericColumns.size()
        || yIdx >= outputNumericColumns.size()
        || depthIdx >= outputNumericColumns.size()) {
        return depthProfile;
    }

    const QVector<double> &xCol = outputNumericColumns[xIdx];
    const QVector<double> &dCol = outputNumericColumns[depthIdx];
    const QVector<double> &yCol = outputNumericColumns[yIdx];
    const int n = qMin(xCol.size(), qMin(dCol.size(), yCol.size()));
    if (n < 2) {
        return depthProfile;
    }

    QMap<double, QVector<QPointF>> byDepth; // depth -> [(x, y)]
    for (int i = 0; i < n; ++i) {
        byDepth[dCol[i]].push_back(QPointF(xCol[i], yCol[i]));
    }

    depthProfile.reserve(byDepth.size());
    for (auto it = byDepth.begin(); it != byDepth.end(); ++it) {
        QVector<QPointF> pairs = it.value();
        std::sort(pairs.begin(), pairs.end(), [](const QPointF &a, const QPointF &b) {
            return a.x() < b.x();
        });

        double interpolated = pairs.first().y();
        if (targetX <= pairs.first().x()) {
            interpolated = pairs.first().y();
        } else if (targetX >= pairs.last().x()) {
            interpolated = pairs.last().y();
        } else {
            for (int j = 1; j < pairs.size(); ++j) {
                const double x0 = pairs[j - 1].x();
                const double x1 = pairs[j].x();
                if (targetX >= x0 && targetX <= x1 && x1 != x0) {
                    const double y0 = pairs[j - 1].y();
                    const double y1 = pairs[j].y();
                    const double w = (targetX - x0) / (x1 - x0);
                    interpolated = y0 + w * (y1 - y0);
                    break;
                }
            }
        }

        depthProfile.push_back(QPointF(it.key(), interpolated));
    }
    return depthProfile;
}

void ModelCreatorWindow::appendComparisonHistory() const
{
    if (!lastComparisonValid) {
        return;
    }

    const QString targetDir = artifactsDirEdit->text().trimmed();
    if (targetDir.isEmpty()) {
        return;
    }

    QDir dir(targetDir);
    if (!dir.exists()) {
        return;
    }

    const QString signature = QString("%1|%2|%3|%4|%5|%6|%7|%8|%9")
                                  .arg(lastComparisonN)
                                  .arg(lastComparisonRmse, 0, 'g', 12)
                                  .arg(lastComparisonMae, 0, 'g', 12)
                                  .arg(lastComparisonBias, 0, 'g', 12)
                                  .arg(lastComparisonR2, 0, 'g', 12)
                                  .arg(outputXAxisCombo->currentText())
                                  .arg(outputYAxisCombo->currentText())
                                  .arg(depthColumnCombo->currentText())
                                  .arg(sliceXEdit->text().trimmed());
    if (signature == lastComparisonHistorySignature) {
        return;
    }

    const QString filePath = dir.filePath("comparison_history.csv");
    QFile file(filePath);
    const bool existed = file.exists();
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
        return;
    }

    QTextStream ts(&file);
    if (!existed) {
        ts << "timestamp_utc,n,rmse,mae,bias,r2,x_column,y_column,depth_column,slice_x\n";
    }
    ts << QDateTime::currentDateTimeUtc().toString(Qt::ISODate) << ","
       << lastComparisonN << ","
       << lastComparisonRmse << ","
       << lastComparisonMae << ","
       << lastComparisonBias << ","
       << lastComparisonR2 << ","
       << outputXAxisCombo->currentText() << ","
       << outputYAxisCombo->currentText() << ","
       << depthColumnCombo->currentText() << ","
       << sliceXEdit->text().trimmed() << "\n";
    lastComparisonHistorySignature = signature;
}

void ModelCreatorWindow::clearComparisonHistory()
{
    const QString targetDir = artifactsDirEdit->text().trimmed();
    if (targetDir.isEmpty()) {
        QMessageBox::information(this, tr("Clear history"), tr("Set an artifacts directory first."));
        return;
    }

    const QString filePath = QDir(targetDir).filePath("comparison_history.csv");
    if (!QFileInfo::exists(filePath)) {
        QMessageBox::information(this, tr("Clear history"), tr("No comparison_history.csv found."));
        return;
    }

    if (!QFile::remove(filePath)) {
        QMessageBox::warning(this, tr("Clear history"), tr("Could not delete comparison_history.csv."));
        return;
    }

    lastComparisonHistorySignature.clear();
    appendLog(stamp(tr("Cleared comparison history: %1").arg(filePath)));
}

void ModelCreatorWindow::exportPlotDataCsv()
{
    QString error;
    if (!loadOutputColumns(&error)) {
        QMessageBox::warning(this, tr("Export plot data"), error);
        return;
    }

    const int xIdx = outputXAxisCombo->currentIndex();
    const int yIdx = outputYAxisCombo->currentIndex();
    const int depthIdx = depthColumnCombo->currentIndex();
    if (xIdx < 0 || yIdx < 0 || depthIdx < 0) {
        QMessageBox::warning(this, tr("Export plot data"), tr("Please choose X/Y/Depth columns first."));
        return;
    }

    bool ok = false;
    double targetX = sliceXEdit->text().trimmed().toDouble(&ok);
    if (!ok) {
        const QVector<double> &xCol = outputNumericColumns[xIdx];
        if (xCol.isEmpty()) {
            QMessageBox::warning(this, tr("Export plot data"), tr("Could not derive Slice X/R from empty X column."));
            return;
        }
        double minX = xCol.first();
        double maxX = xCol.first();
        for (double x : xCol) {
            minX = qMin(minX, x);
            maxX = qMax(maxX, x);
        }
        targetX = 0.5 * (minX + maxX);
        sliceXEdit->setText(QString::number(targetX, 'g', 6));
    }

    const QString defaultPath = QDir(artifactsDirEdit->text().trimmed()).filePath("plot_export.csv");
    const QString filePath = QFileDialog::getSaveFileName(this,
                                                          tr("Save plot data"),
                                                          defaultPath,
                                                          tr("CSV files (*.csv);;All files (*.*)"));
    if (filePath.isEmpty()) {
        return;
    }

    const QVector<double> &xCol = outputNumericColumns[xIdx];
    const QVector<double> &yCol = outputNumericColumns[yIdx];
    const int n = qMin(xCol.size(), yCol.size());
    QVector<QPointF> outputSeries;
    outputSeries.reserve(n);
    for (int i = 0; i < n; ++i) {
        outputSeries.push_back(QPointF(xCol[i], yCol[i]));
    }
    const QVector<QPointF> depthSeries = computeDepthSliceSeries(targetX, xIdx, yIdx, depthIdx);

    QSaveFile out(filePath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export plot data"), tr("Could not open destination CSV for writing."));
        return;
    }

    QTextStream ts(&out);
    ts << "# output_plot\n";
    ts << "x_value,y_value,x_column,y_column\n";
    for (const QPointF &pt : outputSeries) {
        ts << pt.x() << "," << pt.y() << ","
           << outputNumericHeaders.value(xIdx) << ","
           << outputNumericHeaders.value(yIdx) << "\n";
    }

    ts << "\n# depth_slice\n";
    ts << "depth_value,parameter_value,target_x,x_column,depth_column,y_column\n";
    for (const QPointF &pt : depthSeries) {
        ts << pt.x() << "," << pt.y() << "," << targetX << ","
           << outputNumericHeaders.value(xIdx) << ","
           << outputNumericHeaders.value(depthIdx) << ","
           << outputNumericHeaders.value(yIdx) << "\n";
    }

    ts << "\n# comparison_summary\n";
    ts << "summary_text\n";
    ts << "\"" << comparisonSummaryLabel->text().replace('"', "\"\"") << "\"\n";

    if (!out.commit()) {
        QMessageBox::warning(this, tr("Export plot data"), tr("Failed to save CSV."));
        return;
    }

    QJsonObject report;
    report["source_output_file"] = outputSeriesFileEdit->text().trimmed();
    report["x_column"] = outputNumericHeaders.value(xIdx);
    report["y_column"] = outputNumericHeaders.value(yIdx);
    report["depth_column"] = outputNumericHeaders.value(depthIdx);
    report["slice_x"] = targetX;
    report["comparison_summary"] = comparisonSummaryLabel->text();
    report["comparison_valid"] = lastComparisonValid;
    if (lastComparisonValid) {
        QJsonObject metrics;
        metrics["n"] = lastComparisonN;
        metrics["rmse"] = lastComparisonRmse;
        metrics["mae"] = lastComparisonMae;
        metrics["bias"] = lastComparisonBias;
        metrics["r2"] = lastComparisonR2;
        report["comparison_metrics"] = metrics;
    }

    QJsonArray outputSeriesArray;
    for (const QPointF &pt : outputSeries) {
        QJsonObject item;
        item["x"] = pt.x();
        item["y"] = pt.y();
        outputSeriesArray.append(item);
    }
    report["output_series"] = outputSeriesArray;

    QJsonArray depthSeriesArray;
    for (const QPointF &pt : depthSeries) {
        QJsonObject item;
        item["depth"] = pt.x();
        item["value"] = pt.y();
        depthSeriesArray.append(item);
    }
    report["depth_slice_series"] = depthSeriesArray;

    const QString jsonPath = QFileInfo(filePath).absolutePath() + "/" + QFileInfo(filePath).completeBaseName() + "_report.json";
    QSaveFile jsonOut(jsonPath);
    if (jsonOut.open(QIODevice::WriteOnly | QIODevice::Text)) {
        jsonOut.write(QJsonDocument(report).toJson(QJsonDocument::Indented));
        jsonOut.commit();
    }

    appendLog(stamp(tr("Exported plot data CSV: %1").arg(filePath)));
    appendLog(stamp(tr("Exported analysis report JSON: %1").arg(jsonPath)));
}

void ModelCreatorWindow::exportAllDepthSlicesCsv()
{
    QString error;
    if (!loadOutputColumns(&error)) {
        QMessageBox::warning(this, tr("Export all depth slices"), error);
        return;
    }

    const int xIdx = outputXAxisCombo->currentIndex();
    const int depthIdx = depthColumnCombo->currentIndex();
    if (xIdx < 0 || depthIdx < 0) {
        QMessageBox::warning(this, tr("Export all depth slices"), tr("Please choose X and Depth columns first."));
        return;
    }

    bool ok = false;
    double targetX = sliceXEdit->text().trimmed().toDouble(&ok);
    if (!ok) {
        const QVector<double> &xCol = outputNumericColumns[xIdx];
        if (xCol.isEmpty()) {
            QMessageBox::warning(this, tr("Export all depth slices"), tr("Could not derive Slice X/R from empty X column."));
            return;
        }
        double minX = xCol.first();
        double maxX = xCol.first();
        for (double x : xCol) {
            minX = qMin(minX, x);
            maxX = qMax(maxX, x);
        }
        targetX = 0.5 * (minX + maxX);
        sliceXEdit->setText(QString::number(targetX, 'g', 6));
    }

    QStringList yHeaders;
    QMap<double, QMap<QString, double>> table; // depth -> (header -> value)
    for (int yIdx = 0; yIdx < outputNumericColumns.size(); ++yIdx) {
        if (yIdx == xIdx || yIdx == depthIdx) {
            continue;
        }
        const QString yHeader = outputNumericHeaders.value(yIdx);
        yHeaders << yHeader;
        const QVector<QPointF> series = computeDepthSliceSeries(targetX, xIdx, yIdx, depthIdx);
        for (const QPointF &pt : series) {
            table[pt.x()][yHeader] = pt.y();
        }
    }

    if (table.isEmpty() || yHeaders.isEmpty()) {
        QMessageBox::information(this, tr("Export all depth slices"), tr("No depth-slice data available for export."));
        return;
    }

    const QString defaultPath = QDir(artifactsDirEdit->text().trimmed()).filePath("depth_slices_all_params.csv");
    const QString filePath = QFileDialog::getSaveFileName(this,
                                                          tr("Save all depth slices"),
                                                          defaultPath,
                                                          tr("CSV files (*.csv);;All files (*.*)"));
    if (filePath.isEmpty()) {
        return;
    }

    QSaveFile out(filePath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export all depth slices"), tr("Could not open destination CSV for writing."));
        return;
    }

    QTextStream ts(&out);
    ts << "depth";
    for (const QString &header : yHeaders) {
        ts << "," << header;
    }
    ts << ",slice_x,x_column,depth_column\n";

    for (auto it = table.begin(); it != table.end(); ++it) {
        ts << it.key();
        const QMap<QString, double> row = it.value();
        for (const QString &header : yHeaders) {
            if (row.contains(header)) {
                ts << "," << row.value(header);
            } else {
                ts << ",";
            }
        }
        ts << "," << targetX
           << "," << outputNumericHeaders.value(xIdx)
           << "," << outputNumericHeaders.value(depthIdx)
           << "\n";
    }

    if (!out.commit()) {
        QMessageBox::warning(this, tr("Export all depth slices"), tr("Failed to save CSV."));
        return;
    }

    appendLog(stamp(tr("Exported all-parameter depth slices CSV: %1").arg(filePath)));
}

QStringList ModelCreatorWindow::collectExportArtifacts() const
{
    QStringList out;
    const QString workingDirectory = workingDirEdit->text().trimmed();
    if (workingDirectory.isEmpty()) {
        return out;
    }

    QDirIterator it(workingDirectory, QDir::Files, QDirIterator::Subdirectories);
    static const QStringList exportExt = {"vtk", "vtp", "vtu", "csv", "txt"};
    while (it.hasNext()) {
        it.next();
        const QFileInfo fi = it.fileInfo();
        if (exportExt.contains(fi.suffix().toLower())) {
            out << fi.absoluteFilePath();
        }
    }
    out.sort();
    return out;
}

void ModelCreatorWindow::exportArtifacts()
{
    const QStringList files = collectExportArtifacts();
    if (files.isEmpty()) {
        QMessageBox::information(this,
                                 tr("No artifacts"),
                                 tr("No .vtk/.vtp/.vtu/.csv/.txt files were found in the current working directory tree."));
        return;
    }

    QString targetDirPath = artifactsDirEdit->text().trimmed();
    targetDirPath = QFileDialog::getExistingDirectory(this,
                                                      tr("Select artifact export directory"),
                                                      targetDirPath);
    if (targetDirPath.isEmpty()) {
        return;
    }

    QDir targetDir(targetDirPath);
    if (!targetDir.exists()) {
        QMessageBox::warning(this, tr("Invalid export directory"), tr("Selected export directory does not exist."));
        return;
    }

    const QDir workingDir(workingDirEdit->text().trimmed());
    int copied = 0;
    QStringList exportedPaths;
    QStringList exportedSources;
    for (const QString &sourcePath : files) {
        const QFileInfo sourceInfo(sourcePath);
        QString relative = workingDir.relativeFilePath(sourceInfo.absoluteFilePath());
        if (relative.startsWith("..")) {
            relative = sourceInfo.fileName();
        }
        const QString destPath = targetDir.filePath(relative);

        const QFileInfo destInfo(destPath);
        targetDir.mkpath(destInfo.path());

        QFile::remove(destPath);
        if (QFile::copy(sourcePath, destPath)) {
            ++copied;
            exportedPaths << QFileInfo(destPath).absoluteFilePath();
            exportedSources << sourcePath;
        }
    }

    QSaveFile exportManifest(targetDir.filePath("export_manifest.csv"));
    if (exportManifest.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ts(&exportManifest);
        ts << "export_path,file_name,source_file\n";
        for (int i = 0; i < exportedPaths.size(); ++i) {
            const QFileInfo dst(exportedPaths[i]);
            const QString sourcePath = exportedSources.value(i);
            QString escapedSourcePath = sourcePath;
            escapedSourcePath.replace('"', "\"\"");
            ts << '"' << dst.absoluteFilePath().replace('"', "\"\"") << '"' << ','
               << '"' << dst.fileName().replace('"', "\"\"") << '"' << ','
               << '"' << escapedSourcePath << '"' << "\n";
        }
        exportManifest.commit();
    }

    appendLog(stamp(tr("Exported %1/%2 artifact(s) to %3")
                    .arg(copied)
                    .arg(files.size())
                    .arg(targetDir.absolutePath())));

    if (copied > 0) {
        artifactsDirEdit->setText(targetDir.absolutePath());
        saveSettings();
    }
}

void ModelCreatorWindow::appendLog(const QString &text)
{
    if (text.isEmpty()) {
        return;
    }

    const QString normalized = text.endsWith('\n') ? text.left(text.size() - 1) : text;
    for (const QString &line : normalized.split('\n')) {
        if (!line.trimmed().isEmpty()) {
            logView->append(line);
        }
    }
}

void ModelCreatorWindow::loadSettings()
{
    QSettings settings("DryWellScriptGenerator", "ModelCreatorRunner");
    modelTypeCombo->setCurrentText(settings.value("modelType", "Drywell").toString());
    const QString enrichmentPreset = settings.value("enrichmentPreset").toString();
    const int presetIndex = enrichmentPresetCombo->findData(enrichmentPreset);
    enrichmentPresetCombo->setCurrentIndex(presetIndex >= 0 ? presetIndex : 0);
    exePathEdit->setText(settings.value("ohqExecutable").toString());
    scriptPathEdit->setText(settings.value("ohqScript").toString());
    workingDirEdit->setText(settings.value("workingDirectory").toString());
    artifactsDirEdit->setText(settings.value("artifactsDirectory").toString());
    templateDirEdit->setText(settings.value("templateDirectory").toString());
    generatedScriptEdit->setText(settings.value("generatedScriptPath").toString());
    inflowFileEdit->setText(settings.value("inflowFile").toString());
    simulationStartEdit->setText(settings.value("simulationStart", "44435").toString());
    simulationEndEdit->setText(settings.value("simulationEnd", "44438").toString());
    outputSeriesFileEdit->setText(settings.value("outputSeriesFile", "OHQ_output.txt").toString());
    observationFileEdit->setText(settings.value("observationFile").toString());
    depthProfileFileEdit->setText(settings.value("depthProfileFile").toString());
    observationObjectEdit->setText(settings.value("observationObject", "Soil (1$1)").toString());
    observationExpressionEdit->setText(settings.value("observationExpression", "theta").toString());
    observationNameEdit->setText(settings.value("observationName", "Obs_1").toString());
    additionalCommandsEdit->setPlainText(settings.value("additionalCommands").toString());
}

void ModelCreatorWindow::saveSettings() const
{
    QSettings settings("DryWellScriptGenerator", "ModelCreatorRunner");
    settings.setValue("modelType", modelTypeCombo->currentText());
    settings.setValue("enrichmentPreset", enrichmentPresetCombo->currentData().toString());
    settings.setValue("ohqExecutable", exePathEdit->text());
    settings.setValue("ohqScript", scriptPathEdit->text());
    settings.setValue("workingDirectory", workingDirEdit->text());
    settings.setValue("artifactsDirectory", artifactsDirEdit->text());
    settings.setValue("templateDirectory", templateDirEdit->text());
    settings.setValue("generatedScriptPath", generatedScriptEdit->text());
    settings.setValue("inflowFile", inflowFileEdit->text());
    settings.setValue("simulationStart", simulationStartEdit->text());
    settings.setValue("simulationEnd", simulationEndEdit->text());
    settings.setValue("outputSeriesFile", outputSeriesFileEdit->text());
    settings.setValue("observationFile", observationFileEdit->text());
    settings.setValue("depthProfileFile", depthProfileFileEdit->text());
    settings.setValue("observationObject", observationObjectEdit->text());
    settings.setValue("observationExpression", observationExpressionEdit->text());
    settings.setValue("observationName", observationNameEdit->text());
    settings.setValue("additionalCommands", additionalCommandsEdit->toPlainText());
}

QStringList ModelCreatorWindow::collectRunArtifacts() const
{
    QStringList out;
    if (!runStartedAt.isValid()) {
        return out;
    }

    const QString workingDirectory = workingDirEdit->text();
    if (workingDirectory.isEmpty()) {
        return out;
    }

    QDirIterator it(workingDirectory, QDir::Files, QDirIterator::Subdirectories);
    const QDateTime threshold = runStartedAt.addSecs(-1);

    static const QStringList allowedExt = {"txt", "csv", "log", "json", "ohq", "vtk", "vtp", "vtu"};

    while (it.hasNext()) {
        it.next();
        const QFileInfo fi = it.fileInfo();
        const QString ext = fi.suffix().toLower();
        if (fi.lastModified() >= threshold && (ext.isEmpty() || allowedExt.contains(ext))) {
            out << fi.absoluteFilePath();
        }
    }

    out.sort();
    return out;
}

void ModelCreatorWindow::writeArtifactManifest(const QStringList &artifacts)
{
    const QString targetDirPath = artifactsDirEdit->text().trimmed();
    if (targetDirPath.isEmpty()) {
        return;
    }

    QSaveFile manifest(QDir(targetDirPath).filePath("artifact_manifest.csv"));
    if (!manifest.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream ts(&manifest);
    ts << "file_path,file_name,last_modified_utc\n";
    for (const QString &path : artifacts) {
        const QFileInfo fi(path);
        ts << '"' << fi.absoluteFilePath().replace('"', "\"\"") << '"' << ','
           << '"' << fi.fileName().replace('"', "\"\"") << '"' << ','
           << fi.lastModified().toUTC().toString(Qt::ISODate) << "\n";
    }

    if (manifest.commit()) {
        appendLog(stamp(tr("Wrote artifact manifest: %1").arg(QDir(targetDirPath).filePath("artifact_manifest.csv"))));
    }
}

void ModelCreatorWindow::copyArtifacts(const QStringList &artifacts)
{
    const QString targetDirPath = artifactsDirEdit->text().trimmed();
    if (targetDirPath.isEmpty()) {
        return;
    }

    QDir targetDir(targetDirPath);
    if (!targetDir.exists()) {
        return;
    }

    int copied = 0;
    for (const QString &sourcePath : artifacts) {
        const QFileInfo fi(sourcePath);
        const QString targetPath = targetDir.filePath(fi.fileName());
        QFile::remove(targetPath);
        if (QFile::copy(sourcePath, targetPath)) {
            ++copied;
        }
    }

    appendLog(stamp(tr("Copied %1 artifact(s) to %2").arg(copied).arg(targetDir.absolutePath())));
}
