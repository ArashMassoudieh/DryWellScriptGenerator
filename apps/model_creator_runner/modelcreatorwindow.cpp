// NOTE: This file is part of the DryWellSuite/OpenHydroQual codebase.
#include "modelcreatorwindow.h"

#include "ohqprocessrunner.h"
#include "simplelineplotwidget.h"
#include "starter_script_builder.h"
#include "structure_registry.h"
#include "scripteditordialog.h"

#include <QComboBox>
#include <QCheckBox>
#include <QDateTime>
#include <QDialog>
#include <QAbstractItemView>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QMessageBox>
#include <QProcess>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSettings>
#include <QTabWidget>
#include <QTableWidget>
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

QString VnBuildModeFromPresetSelection(const QString &selection)
{
    const QString trimmed = selection.trimmed();
    if (trimmed.startsWith(QStringLiteral("VN_MODE:"), Qt::CaseInsensitive)) {
        return trimmed.mid(QStringLiteral("VN_MODE:").size()).trimmed();
    }
    return QString();
}

QString BuildModeFromPresetSelection(const QString &selection, const QString &prefix)
{
    const QString trimmed = selection.trimmed();
    const QString token = prefix.trimmed() + QStringLiteral(":");
    if (trimmed.startsWith(token, Qt::CaseInsensitive)) {
        return trimmed.mid(token.size()).trimmed();
    }
    return QString();
}

QString ResolveVnBuildModeForUi(const QString &modelType,
                                const QString &presetSelection,
                                const QString &fallbackBuildMode)
{
    const bool vnModel = modelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0;
    if (!vnModel) {
        return fallbackBuildMode;
    }

    const QString modeFromPreset = VnBuildModeFromPresetSelection(presetSelection);
    if (!modeFromPreset.isEmpty()) {
        return modeFromPreset;
    }

    const QString trimmedPreset = presetSelection.trimmed();
    if (trimmedPreset.startsWith(QStringLiteral("VN_"), Qt::CaseInsensitive) || trimmedPreset.isEmpty()) {
        return QStringLiteral("Preset");
    }

    return fallbackBuildMode;
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

QString FindRepoRoot()
{
    QDir dir(QDir::currentPath());
    for (int i = 0; i < 8; ++i) {
        if (QFileInfo::exists(dir.filePath("DryWellScriptGenerator.pro"))) {
            return dir.absolutePath();
        }
        if (!dir.cdUp()) {
            break;
        }
    }
    return QDir::currentPath();
}

QString FirstExistingDirectory(const QStringList &candidates)
{
    for (const QString &path : candidates) {
        if (!path.trimmed().isEmpty() && QFileInfo(path).exists() && QFileInfo(path).isDir()) {
            return QFileInfo(path).absoluteFilePath();
        }
    }
    return QString();
}

QString FirstExistingFile(const QStringList &candidates)
{
    for (const QString &path : candidates) {
        if (!path.trimmed().isEmpty() && QFileInfo(path).exists() && QFileInfo(path).isFile()) {
            return QFileInfo(path).absoluteFilePath();
        }
    }
    return QString();
}

QString FirstExecutableFile(const QStringList &candidates)
{
    // Return the first candidate that is both present and runnable.
    // This keeps discovery deterministic (first-known-good path wins).
    for (const QString &path : candidates) {
        const QFileInfo info(path);
        if (!path.trimmed().isEmpty() && info.exists() && info.isFile() && info.isExecutable()) {
            return info.absoluteFilePath();
        }
    }
    return QString();
}

void AppendUniquePath(QStringList *paths, const QString &path)
{
    if (paths == nullptr) {
        return;
    }
    const QString normalized = QFileInfo(path).absoluteFilePath();
    if (normalized.trimmed().isEmpty()) {
        return;
    }
    if (!paths->contains(normalized)) {
        paths->push_back(normalized);
    }
}

QStringList CandidateOpenHydroQualRoots(const QString &repoRoot, const QStringList &hintRoots = {})
{
    QStringList roots;
    for (const QString &hint : hintRoots) {
        const QFileInfo info(hint);
        if (info.exists()) {
            AppendUniquePath(&roots, info.isDir() ? info.absoluteFilePath() : info.absolutePath());
        }
    }

    const QDir repoDir(repoRoot);
    AppendUniquePath(&roots, repoDir.filePath("OpenHydroQual"));
    AppendUniquePath(&roots, repoDir.filePath("../OpenHydroQual"));
    AppendUniquePath(&roots, repoDir.filePath("../../OpenHydroQual"));
    AppendUniquePath(&roots, QStringLiteral("/mnt/3rd900/Projects/OpenHydroQual"));
    AppendUniquePath(&roots, QStringLiteral("/home/arash/Projects/OpenHydroQual"));
    return roots;
}

QString DetectTemplateDirectory(const QStringList &rootCandidates, const QString &workingDirectory)
{
    QStringList candidates;
    for (const QString &rootPath : rootCandidates) {
        const QDir root(rootPath);
        if (!root.exists()) {
            continue;
        }
        candidates << root.filePath("resources")
                   << root.filePath("templates")
                   << root.filePath("template_resources")
                   << root.filePath("aquifolium/examples/templates")
                   << root.filePath("aquifolium/templates");
    }

    if (!workingDirectory.trimmed().isEmpty()) {
        const QDir wd(workingDirectory);
        candidates << wd.filePath("templates")
                   << wd.filePath("template_resources");
    }
    return FirstExistingDirectory(candidates);
}

QString FindCliExecutableUnderRoot(const QString &rootPath);

QString DetectExecutablePath(const QStringList &rootCandidates)
{
    for (const QString &rootPath : rootCandidates) {
        const QString candidate = FindCliExecutableUnderRoot(rootPath);
        if (!candidate.isEmpty()) {
            return candidate;
        }
    }
    return QString();
}

QString DetectExecutablePathFromContext(const QString &repoRoot,
                                        const QString &workingDirectory,
                                        const QString &scriptPath,
                                        const QString &templateDirectory,
                                        const QString &configuredExecutable)
{
    const QStringList roots = CandidateOpenHydroQualRoots(repoRoot, {
        workingDirectory,
        scriptPath,
        templateDirectory,
        configuredExecutable
    });
    return DetectExecutablePath(roots);
}

bool LooksLikeGuiOpenHydroQualExecutable(const QFileInfo &executableInfo)
{
    const QString baseName = executableInfo.completeBaseName().trimmed();
    return baseName.compare(QStringLiteral("OpenHydroQual"), Qt::CaseInsensitive) == 0;
}

bool IsGuiExecutableOrAlias(const QFileInfo &executableInfo)
{
    if (LooksLikeGuiOpenHydroQualExecutable(executableInfo)) {
        return true;
    }
    const QString canonical = executableInfo.canonicalFilePath();
    if (!canonical.isEmpty()) {
        return LooksLikeGuiOpenHydroQualExecutable(QFileInfo(canonical));
    }
    return false;
}

bool LooksLikeScriptFilePath(const QFileInfo &pathInfo)
{
    return pathInfo.suffix().compare(QStringLiteral("ohq"), Qt::CaseInsensitive) == 0;
}

bool LooksLikeStaticLibraryPath(const QFileInfo &pathInfo)
{
    return pathInfo.suffix().compare(QStringLiteral("a"), Qt::CaseInsensitive) == 0;
}

bool IsVnSoftGridCustomized(const StarterScriptOptions &options)
{
    constexpr int kDefaultGridX = 16;
    constexpr int kDefaultGridY = 15;
    constexpr int kDefaultUwGridX = 16;
    constexpr int kDefaultUwGridY = 12;
    constexpr double kDefaultCellSize = 586.9;
    constexpr double kDefaultUwCellSize = 586.9;
    constexpr double kDefaultGapSize = 0.0;
    constexpr double kDefaultRwG = 1.2192;
    constexpr double kDefaultRwUw = 1.2192;
    constexpr double kDefaultRoi = 20.0;
    constexpr double kDefaultDepthWellC = 4.8768;
    constexpr double kDefaultDepthWellG = 7.3152;
    constexpr double kDefaultDepthToGw = 43.2816;
    constexpr double kDefaultTopElevation = -5.0;
    constexpr double kDefaultLayerThickness = 1.0;
    constexpr double kEpsilon = 1e-9;

    const auto differs = [](double lhs, double rhs) {
        return std::fabs(lhs - rhs) > kEpsilon;
    };

    return options.vnSoftGridXCount != kDefaultGridX
        || options.vnSoftGridYCount != kDefaultGridY
        || options.vnSoftUwGridXCount != kDefaultUwGridX
        || options.vnSoftUwGridYCount != kDefaultUwGridY
        || differs(options.vnSoftCellSize, kDefaultCellSize)
        || differs(options.vnSoftUwCellSize, kDefaultUwCellSize)
        || differs(options.vnSoftGapSize, kDefaultGapSize)
        || differs(options.vnSoftRwG, kDefaultRwG)
        || differs(options.vnSoftRwUw, kDefaultRwUw)
        || differs(options.vnSoftRadiusOfInfluence, kDefaultRoi)
        || differs(options.vnSoftDepthOfWellC, kDefaultDepthWellC)
        || differs(options.vnSoftDepthOfWellG, kDefaultDepthWellG)
        || differs(options.vnSoftDepthToGroundWater, kDefaultDepthToGw)
        || differs(options.vnSoftTopElevation, kDefaultTopElevation)
        || differs(options.vnSoftLayerThickness, kDefaultLayerThickness);
}

QString AutoDetectVnBuildMode(const StarterScriptOptions &options)
{
    if (!options.vnBaseOhqFile.trimmed().isEmpty()) {
        return QStringLiteral("LoadFromOhq");
    }
    if (!options.vnSoilLayersFile.trimmed().isEmpty()
        || !options.vnMoistureLayersFile.trimmed().isEmpty()
        || IsVnSoftGridCustomized(options)) {
        return QStringLiteral("SoftReference");
    }
    return QStringLiteral("FullReference");
}

void AssignIntIfProvided(QLineEdit *edit, int *target)
{
    if (edit == nullptr || target == nullptr) {
        return;
    }
    const QString text = edit->text().trimmed();
    if (text.isEmpty()) {
        return;
    }
    bool ok = false;
    const int value = text.toInt(&ok);
    if (ok) {
        *target = value;
    }
}

void AssignDoubleIfProvided(QLineEdit *edit, double *target)
{
    if (edit == nullptr || target == nullptr) {
        return;
    }
    const QString text = edit->text().trimmed();
    if (text.isEmpty()) {
        return;
    }
    bool ok = false;
    const double value = text.toDouble(&ok);
    if (ok) {
        *target = value;
    }
}

bool LooksLikeCliOhqBinaryName(const QString &fileName)
{
    return fileName.compare(QStringLiteral("OHQ"), Qt::CaseInsensitive) == 0
        || fileName.compare(QStringLiteral("OHQ.exe"), Qt::CaseInsensitive) == 0
        || fileName.startsWith(QStringLiteral("OHQ_"), Qt::CaseInsensitive);
}

QString FindCliExecutableNearGui(const QFileInfo &guiExecutableInfo)
{
    // Heuristic search anchored around the selected GUI binary path.
    // We walk up a few parent folders and probe common OHQ build layouts.
    QStringList roots;
    QDir dir(guiExecutableInfo.absolutePath());
    for (int i = 0; i < 5; ++i) {
        roots << dir.absolutePath();
        if (!dir.cdUp()) {
            break;
        }
    }

    QStringList candidates;
    for (const QString &root : roots) {
        const QDir rootDir(root);
        candidates << rootDir.filePath("OHQ")
                   << rootDir.filePath("OpenHydroQual")
                   << rootDir.filePath("build/Release/OHQ")
                   << rootDir.filePath("build/Release/OpenHydroQual")
                   << rootDir.filePath("build/Debug/OHQ")
                   << rootDir.filePath("build/Debug/OpenHydroQual")
                   << rootDir.filePath("aquifolium/build/OHQ")
                   << rootDir.filePath("aquifolium/build/OpenHydroQual")
                   << rootDir.filePath("aquifolium/bin/OHQ")
                   << rootDir.filePath("aquifolium/bin/OpenHydroQual");
    }

    const QString nearby = FirstExecutableFile(candidates);
    if (!nearby.isEmpty()) {
        return nearby;
    }

    const QStringList fallbackRoots = {
        // Environment-specific fallback roots used in this project.
        QStringLiteral("/mnt/3rd900/Projects/OpenHydroQual"),
        QStringLiteral("/home/arash/Projects/OpenHydroQual")
    };
    for (const QString &root : fallbackRoots) {
        QDir rootDir(root);
        if (!rootDir.exists()) {
            continue;
        }
        QDirIterator it(rootDir.absolutePath(),
                        QDir::Files | QDir::NoSymLinks,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            const QFileInfo fileInfo = it.fileInfo();
            if (LooksLikeCliOhqBinaryName(fileInfo.fileName()) && fileInfo.isExecutable()) {
                return fileInfo.absoluteFilePath();
            }
        }
    }

    return QString();
}

QString FindCliExecutableUnderRoot(const QString &rootPath)
{
    // Root-based discovery used by the "Browse" flow (folder selection).
    // First try common direct locations, then recurse as a safety net.
    const QDir root(rootPath);
    if (!root.exists()) {
        return QString();
    }

    const QString direct = FirstExecutableFile({
        root.filePath("OHQ"),
        root.filePath("OpenHydroQual"),
        root.filePath("build/Release/OHQ"),
        root.filePath("build/Release/OpenHydroQual"),
        root.filePath("build/Debug/OHQ"),
        root.filePath("build/Debug/OpenHydroQual"),
        root.filePath("aquifolium/build/OHQ"),
        root.filePath("aquifolium/build/OpenHydroQual"),
        root.filePath("aquifolium/bin/OHQ"),
        root.filePath("aquifolium/bin/OpenHydroQual")
    });
    if (!direct.isEmpty()) {
        return direct;
    }

    QDirIterator it(root.absolutePath(),
                    QDir::Files | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QFileInfo info = it.fileInfo();
        if (LooksLikeCliOhqBinaryName(info.fileName()) && info.isExecutable()) {
            return info.absoluteFilePath();
        }
    }

    return QString();
}

QStringList BuildExecutableArguments(const QString &argumentTemplate, const QString &scriptPath)
{
    if (argumentTemplate.trimmed().isEmpty()) {
        return QStringList{scriptPath};
    }

    QStringList args = QProcess::splitCommand(argumentTemplate);
    for (QString &arg : args) {
        if (arg.contains(QStringLiteral("{script}"))) {
            arg.replace(QStringLiteral("{script}"), scriptPath);
        }
    }
    return args;
}

bool BuildGuiConfigFromTemplate(const QString &templatePath,
                                const QString &scriptPath,
                                const QString &workingDirectory,
                                QString *generatedConfigPath,
                                QString *errorMessage)
{
    QFile inFile(templatePath);
    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Cannot open GUI config template: %1").arg(templatePath);
        }
        return false;
    }

    QString configText = QString::fromUtf8(inFile.readAll());
    configText.replace(QStringLiteral("{script}"), scriptPath);
    configText.replace(QStringLiteral("{working_dir}"), workingDirectory);

    const QString outPath = QDir(workingDirectory).filePath(QStringLiteral("runner_gui_config.generated.json"));
    QSaveFile outFile(outPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Cannot create generated GUI config: %1").arg(outPath);
        }
        return false;
    }
    QTextStream out(&outFile);
    out << configText;
    if (!outFile.commit()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Failed to write generated GUI config: %1").arg(outPath);
        }
        return false;
    }

    if (generatedConfigPath) {
        *generatedConfigPath = outPath;
    }
    return true;
}

bool BuildDefaultGuiConfig(const QString &scriptPath,
                           const QString &workingDirectory,
                           QString *generatedConfigPath,
                           QString *errorMessage)
{
    QJsonObject root;
    root.insert(QStringLiteral("script"), scriptPath);
    root.insert(QStringLiteral("working_dir"), workingDirectory);
    root.insert(QStringLiteral("run"), true);
    root.insert(QStringLiteral("solve"), true);

    const QString outPath = QDir(workingDirectory).filePath(QStringLiteral("runner_gui_config.auto.json"));
    QSaveFile outFile(outPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Cannot create auto GUI config: %1").arg(outPath);
        }
        return false;
    }
    outFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!outFile.commit()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Failed to write auto GUI config: %1").arg(outPath);
        }
        return false;
    }

    if (generatedConfigPath) {
        *generatedConfigPath = outPath;
    }
    return true;
}

bool IsKnownRuntimeNoiseLine(const QString &line)
{
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    return trimmed.startsWith(QStringLiteral("qt.core.qmetaobject.connectslotsbyname: QMetaObject::connectSlotsByName: No matching signal for on_"))
        || trimmed.startsWith(QStringLiteral("qt.core.qobject.connect: QObject::connect: No such slot "))
        || trimmed.startsWith(QStringLiteral("qt.core.qobject.connect: QObject::connect: No such signal "))
        || trimmed.startsWith(QStringLiteral("qt.core.qobject.connect: QObject::connect:  (sender name:"))
        || trimmed.startsWith(QStringLiteral("qt.core.qobject.connect: QObject::connect:  (receiver name:"));
}

QString FilterRuntimeNoise(const QString &text, int *suppressedLineCount)
{
    if (suppressedLineCount == nullptr) {
        return text;
    }

    const QStringList lines = text.split('\n');
    QStringList kept;
    kept.reserve(lines.size());

    for (const QString &line : lines) {
        if (IsKnownRuntimeNoiseLine(line)) {
            ++(*suppressedLineCount);
            continue;
        }
        kept.push_back(line);
    }

    return kept.join('\n');
}
}

ModelCreatorWindow::ModelCreatorWindow(QWidget *parent)
    : QMainWindow(parent),
      modelTypeCombo(new QComboBox(this)),
      workflowModeCombo(new QComboBox(this)),
      exePathEdit(new QLineEdit(this)),
      exeArgsEdit(new QLineEdit(this)),
      guiConfigTemplateEdit(new QLineEdit(this)),
      scriptPathEdit(new QLineEdit(this)),
      workingDirEdit(new QLineEdit(this)),
      artifactsDirEdit(new QLineEdit(this)),
      templateDirEdit(new QLineEdit(this)),
      generatedScriptEdit(new QLineEdit(this)),
      enrichmentPresetCombo(new QComboBox(this)),
      inflowFileEdit(new QLineEdit(this)),
      simulationStartEdit(new QLineEdit(this)),
      simulationEndEdit(new QLineEdit(this)),
      ksatScaleEdit(new QLineEdit(this)),
      ksatScaleGEdit(new QLineEdit(this)),
      ksatScaleUwEdit(new QLineEdit(this)),
      outputSeriesFileEdit(new QLineEdit(this)),
      observationFileEdit(new QLineEdit(this)),
      depthProfileFileEdit(new QLineEdit(this)),
      vnBaseOhqFileEdit(new QLineEdit(this)),
      vnSoilLayersFileEdit(new QLineEdit(this)),
      vnMoistureLayersFileEdit(new QLineEdit(this)),
      vnBuildModeCombo(new QComboBox(this)),
      vnSoftGridXEdit(new QLineEdit(this)),
      vnSoftGridYEdit(new QLineEdit(this)),
      vnSoftUwGridXEdit(new QLineEdit(this)),
      vnSoftUwGridYEdit(new QLineEdit(this)),
      vnSoftCellSizeEdit(new QLineEdit(this)),
      vnSoftUwCellSizeEdit(new QLineEdit(this)),
      vnSoftGapSizeEdit(new QLineEdit(this)),
      vnSoftRwGEdit(new QLineEdit(this)),
      vnSoftRwUwEdit(new QLineEdit(this)),
      vnSoftRadiusInfluenceEdit(new QLineEdit(this)),
      vnSoftDepthWellCEdit(new QLineEdit(this)),
      vnSoftDepthWellGEdit(new QLineEdit(this)),
      vnSoftDepthToGwEdit(new QLineEdit(this)),
      vnSoftTopElevationEdit(new QLineEdit(this)),
      vnSoftLayerThicknessEdit(new QLineEdit(this)),
      vnSoftSoilKsatOriginalEdit(new QLineEdit(this)),
      vnSoftSoilAlphaEdit(new QLineEdit(this)),
      vnSoftSoilNEdit(new QLineEdit(this)),
      vnSoftSoilThetaSatEdit(new QLineEdit(this)),
      vnSoftSoilThetaResEdit(new QLineEdit(this)),
      vnSoftSoilParamModeCombo(new QComboBox(this)),
      vnSoftSoilParameterFileEdit(new QLineEdit(this)),
      observationObjectEdit(new QLineEdit(this)),
      observationExpressionEdit(new QLineEdit(this)),
      observationNameEdit(new QLineEdit(this)),
      additionalCommandsEdit(new QTextEdit(this)),
      showOptionalFieldsCheck(nullptr),
      allowGuiExecutionCheck(nullptr),
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
      quickRunButton(new QPushButton(tr("Quick Run + Save"), this)),
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
    auto *runTabLayout = new QVBoxLayout(runTab);
    auto *setupScroll = new QScrollArea(runTab);
    setupScroll->setWidgetResizable(true);
    setupScroll->setFrameShape(QFrame::NoFrame);
    runTabLayout->addWidget(setupScroll);
    auto *runContent = new QWidget(setupScroll);
    setupScroll->setWidget(runContent);
    auto *layout = new QVBoxLayout(runContent);
    tabs->addTab(runTab, tr("Setup + Run"));

    workflowModeCombo->addItem(tr("Generate from scratch"), "generate");
    workflowModeCombo->addItem(tr("Load/Edit existing .ohq"), "load");
    modelTypeCombo->addItems(StructureRegistry::ModelTypes());
    syncEnrichmentPresetForModel();

    auto addFileRow = [](QVBoxLayout *targetLayout, const QString &labelText, QLineEdit *edit, const QString &buttonText, auto slot) -> QWidget* {
        auto *container = new QWidget();
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(labelText));
        row->addWidget(edit, 1);
        auto *btn = new QPushButton(buttonText);
        QObject::connect(btn, &QPushButton::clicked, slot);
        row->addWidget(btn);
        targetLayout->addWidget(container);
        return container;
    };

    auto addTextRow = [](QVBoxLayout *targetLayout, const QString &labelText, QWidget *editor) -> QWidget* {
        auto *container = new QWidget();
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(labelText));
        row->addWidget(editor, 1);
        targetLayout->addWidget(container);
        return container;
    };

    addTextRow(layout, tr("Workflow mode"), workflowModeCombo);
    modelTypeRowWidget = addTextRow(layout, tr("Model type"), modelTypeCombo);
    presetRowWidget = addTextRow(layout, tr("Model enrichment preset"), enrichmentPresetCombo);
    addFileRow(layout, tr("OHQ executable (optional)"), exePathEdit, tr("Browse"), [this]() { chooseExecutable(); });
    exePathEdit->setPlaceholderText(tr("Optional: auto-detected from OpenHydroQual roots when empty"));
    exePathEdit->setToolTip(tr("Optional override. Leave blank to auto-detect OHQ from working/script/template locations."));
    addTextRow(layout, tr("Executable args"), exeArgsEdit);
    exeArgsEdit->setPlaceholderText(tr("Optional, e.g. --script {script} --run"));
    exeArgsEdit->setToolTip(tr("Command-line arguments passed to the executable. Use {script} placeholder for the selected .ohq path. If omitted: OHQ CLI gets positional script; OpenHydroQual GUI gets <script> --run; custom executables get no implicit args."));
    guiConfigTemplateRowWidget = addFileRow(layout, tr("GUI config template (optional)"), guiConfigTemplateEdit, tr("Browse"), [this]() { chooseGuiConfigTemplate(); });
    guiConfigTemplateEdit->setPlaceholderText(tr("Optional JSON template for OpenHydroQual GUI (supports {script}, {working_dir})"));
    allowGuiExecutionCheck = new QCheckBox(tr("Allow OpenHydroQual GUI execution fallback"), this);
    allowGuiExecutionCheck->setChecked(false);
    allowGuiExecutionCheck->setToolTip(tr("Recommended OFF. Keep disabled to enforce CLI/internal-solver execution only."));
    layout->addWidget(allowGuiExecutionCheck);
    addFileRow(layout, tr("OHQ script (.ohq)"), scriptPathEdit, tr("Browse"), [this]() { chooseScript(); });
    scriptPathEdit->setToolTip(tr("Select an existing .ohq file if you want to run without generating a new starter script."));
    scriptPathEdit->setPlaceholderText(tr("Suggested: <repo>/hq_drywell.ohq or <repo>/r_bioswale.ohq"));
    addFileRow(layout, tr("Working directory"), workingDirEdit, tr("Browse"), [this]() { chooseWorkingDirectory(); });
    workingDirEdit->setPlaceholderText(tr("Suggested: this repository root"));
    addFileRow(layout, tr("Artifacts directory"), artifactsDirEdit, tr("Browse"), [this]() { chooseArtifactsDirectory(); });
    artifactsDirEdit->setPlaceholderText(tr("Suggested: <working_dir>/artifacts"));
    templateDirRowWidget = addFileRow(layout, tr("Template resources dir"), templateDirEdit, tr("Browse"), [this]() { chooseTemplateDirectory(); });
    templateDirEdit->setPlaceholderText(tr("Suggested: /mnt/3rd900/Projects/OpenHydroQual/resources"));
    generatedScriptRowWidget = addFileRow(layout, tr("Generated script path"), generatedScriptEdit, tr("Browse"), [this]() { chooseGeneratedScriptPath(); });
    generatedScriptEdit->setPlaceholderText(tr("Suggested: <working_dir>/starter_generated.ohq"));
    inflowRowWidget = addFileRow(layout, tr("Inflow file"), inflowFileEdit, tr("Browse"), [this]() { chooseInflowFile(); });
    inflowFileEdit->setPlaceholderText(tr("Suggested: <repo>/inflow.csv"));
    simulationStartRowWidget = addTextRow(layout, tr("Simulation start"), simulationStartEdit);
    simulationStartEdit->setPlaceholderText(tr("Suggested: auto-from-inflow"));
    simulationEndRowWidget = addTextRow(layout, tr("Simulation end"), simulationEndEdit);
    simulationEndEdit->setPlaceholderText(tr("Suggested: auto-from-inflow"));
    auto setupCompactNumericEdit = [](QLineEdit *edit, const QString &placeholder) {
        edit->setPlaceholderText(placeholder);
        edit->setMaximumWidth(100);
    };
    setupCompactNumericEdit(ksatScaleEdit, tr("all"));
    setupCompactNumericEdit(ksatScaleGEdit, tr("g"));
    setupCompactNumericEdit(ksatScaleUwEdit, tr("uw"));
    {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("Ksat scales")));
        row->addWidget(new QLabel(tr("all")));
        row->addWidget(ksatScaleEdit);
        row->addWidget(new QLabel(tr("g")));
        row->addWidget(ksatScaleGEdit);
        row->addWidget(new QLabel(tr("uw")));
        row->addWidget(ksatScaleUwEdit);
        row->addStretch(1);
        layout->addWidget(container);
    }
    outputSeriesRowWidget = addTextRow(layout, tr("Output series file"), outputSeriesFileEdit);
    outputSeriesFileEdit->setPlaceholderText(tr("Suggested: OHQ_output.txt"));
    observationFileRowWidget = addFileRow(layout, tr("Observation file (optional)"), observationFileEdit, tr("Browse"), [this]() { chooseObservationFile(); });
    observationFileEdit->setPlaceholderText(tr("Suggested: <repo>/observation.csv"));
    depthProfileRowWidget = addFileRow(layout, tr("Depth profile file (optional)"), depthProfileFileEdit, tr("Browse"), [this]() { chooseDepthProfileFile(); });
    depthProfileFileEdit->setPlaceholderText(tr("Suggested: <repo>/depth_profile.csv"));
    vnBaseRowWidget = addFileRow(layout, tr("VN base .ohq (optional)"), vnBaseOhqFileEdit, tr("Browse"), [this]() { chooseVnBaseOhqFile(); });
    vnBaseOhqFileEdit->setPlaceholderText(tr("Optional: load whole VN OHQ script as generation baseline"));
    vnSoilRowWidget = addFileRow(layout, tr("VN soil layers snippet (optional)"), vnSoilLayersFileEdit, tr("Browse"), [this]() { chooseVnSoilLayersFile(); });
    vnSoilLayersFileEdit->setPlaceholderText(tr("Optional: .txt/.ohq/.csv with VN soil-layer commands"));
    vnMoistureRowWidget = addFileRow(layout, tr("VN moisture layers snippet (optional)"), vnMoistureLayersFileEdit, tr("Browse"), [this]() { chooseVnMoistureLayersFile(); });
    vnMoistureLayersFileEdit->setPlaceholderText(tr("Optional: .txt/.ohq/.csv with VN moisture-layer commands"));
    vnBuildModeCombo->addItem(tr("SoftReference"), QStringLiteral("SoftReference"));
    vnBuildModeCombo->addItem(tr("FullReference"), QStringLiteral("FullReference"));
    vnBuildModeCombo->addItem(tr("LoadFromOhq"), QStringLiteral("LoadFromOhq"));
    vnBuildModeCombo->addItem(tr("Preset"), QStringLiteral("Preset"));
    vnBuildModeCombo->setToolTip(tr("SoftReference is the editable VN mode and is intended to reproduce FullReference exactly when the defaults remain unchanged. FullReference uses the embedded canonical VN reference. LoadFromOhq uses the selected VN base script. Preset uses the simple preset path."));
    vnBuildModeRowWidget = addTextRow(layout, tr("VN build mode"), vnBuildModeCombo);
    setupCompactNumericEdit(vnSoftGridXEdit, tr("16"));
    setupCompactNumericEdit(vnSoftGridYEdit, tr("15"));
    setupCompactNumericEdit(vnSoftUwGridXEdit, tr("16"));
    setupCompactNumericEdit(vnSoftUwGridYEdit, tr("12"));
    setupCompactNumericEdit(vnSoftCellSizeEdit, tr("586.9"));
    setupCompactNumericEdit(vnSoftUwCellSizeEdit, tr("586.9"));
    setupCompactNumericEdit(vnSoftGapSizeEdit, tr("0.0"));
    setupCompactNumericEdit(vnSoftRwGEdit, tr("1.2192"));
    setupCompactNumericEdit(vnSoftRwUwEdit, tr("1.2192"));
    setupCompactNumericEdit(vnSoftRadiusInfluenceEdit, tr("20.0"));
    setupCompactNumericEdit(vnSoftDepthWellCEdit, tr("4.8768"));
    setupCompactNumericEdit(vnSoftDepthWellGEdit, tr("7.3152"));
    setupCompactNumericEdit(vnSoftDepthToGwEdit, tr("43.2816"));
    setupCompactNumericEdit(vnSoftTopElevationEdit, tr("-5.0"));
    setupCompactNumericEdit(vnSoftLayerThicknessEdit, tr("1.0"));
    setupCompactNumericEdit(vnSoftSoilKsatOriginalEdit, tr("1.05196"));
    setupCompactNumericEdit(vnSoftSoilAlphaEdit, tr("3.47536"));
    setupCompactNumericEdit(vnSoftSoilNEdit, tr("1.74582"));
    setupCompactNumericEdit(vnSoftSoilThetaSatEdit, tr("0.39"));
    setupCompactNumericEdit(vnSoftSoilThetaResEdit, tr("0.049"));
    vnSoftSoilParamModeCombo->addItem(tr("VN Ref defaults"), QStringLiteral("VnReferenceDefaults"));
    vnSoftSoilParamModeCombo->addItem(tr("Manual"), QStringLiteral("Manual"));
    vnSoftSoilParamModeCombo->addItem(tr("ModelCreator defaults"), QStringLiteral("ModelCreatorDefaults"));
    vnSoftSoilParamModeCombo->addItem(tr("File (depth profile)"), QStringLiteral("File"));
    vnSoftSoilParameterFileEdit->setPlaceholderText(tr("Optional: CSV depth profile for Ksat/alpha/n/theta_s/theta_r"));
    {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("VN soft Soil-g")));
        row->addWidget(new QLabel(tr("nr_g")));
        row->addWidget(vnSoftGridXEdit);
        row->addWidget(new QLabel(tr("nz_g")));
        row->addWidget(vnSoftGridYEdit);
        row->addWidget(new QLabel(tr("cell[m]")));
        row->addWidget(vnSoftCellSizeEdit);
        row->addStretch(1);
        layout->addWidget(container);
        vnSoftGridXRowWidget = container;
        vnSoftGridYRowWidget = container;
        vnSoftCellSizeRowWidget = container;
    }
    {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("VN soft Soil-uw")));
        row->addWidget(new QLabel(tr("nr_uw")));
        row->addWidget(vnSoftUwGridXEdit);
        row->addWidget(new QLabel(tr("nz_uw")));
        row->addWidget(vnSoftUwGridYEdit);
        row->addWidget(new QLabel(tr("cell[m]")));
        row->addWidget(vnSoftUwCellSizeEdit);
        row->addWidget(new QLabel(tr("gap[m]")));
        row->addWidget(vnSoftGapSizeEdit);
        row->addStretch(1);
        layout->addWidget(container);
        vnSoftUwGridXRowWidget = container;
        vnSoftUwGridYRowWidget = container;
        vnSoftUwCellSizeRowWidget = container;
        vnSoftGapSizeRowWidget = container;
    }
    {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("VN soft radii [m]")));
        row->addWidget(new QLabel(tr("rw_g")));
        row->addWidget(vnSoftRwGEdit);
        row->addWidget(new QLabel(tr("rw_uw")));
        row->addWidget(vnSoftRwUwEdit);
        row->addWidget(new QLabel(tr("ROI")));
        row->addWidget(vnSoftRadiusInfluenceEdit);
        row->addStretch(1);
        layout->addWidget(container);
        vnSoftRadiusRowWidget = container;
    }
    {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("VN soft depths [m]")));
        row->addWidget(new QLabel(tr("well_c")));
        row->addWidget(vnSoftDepthWellCEdit);
        row->addWidget(new QLabel(tr("well_g")));
        row->addWidget(vnSoftDepthWellGEdit);
        row->addWidget(new QLabel(tr("to_gw")));
        row->addWidget(vnSoftDepthToGwEdit);
        row->addStretch(1);
        layout->addWidget(container);
        vnSoftDepthRowWidget = container;
    }
    {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("VN soft z")));
        row->addWidget(new QLabel(tr("top[m]")));
        row->addWidget(vnSoftTopElevationEdit);
        row->addWidget(new QLabel(tr("dz[m]")));
        row->addWidget(vnSoftLayerThicknessEdit);
        row->addStretch(1);
        layout->addWidget(container);
        vnSoftTopElevationRowWidget = container;
        vnSoftLayerThicknessRowWidget = container;
    }
    {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("VN soft soil")));
        row->addWidget(new QLabel(tr("mode")));
        row->addWidget(vnSoftSoilParamModeCombo);
        row->addWidget(new QLabel(tr("Ksat")));
        row->addWidget(vnSoftSoilKsatOriginalEdit);
        row->addWidget(new QLabel(tr("alpha")));
        row->addWidget(vnSoftSoilAlphaEdit);
        row->addWidget(new QLabel(tr("n")));
        row->addWidget(vnSoftSoilNEdit);
        row->addWidget(new QLabel(tr("theta_sat")));
        row->addWidget(vnSoftSoilThetaSatEdit);
        row->addWidget(new QLabel(tr("theta_res")));
        row->addWidget(vnSoftSoilThetaResEdit);
        row->addWidget(new QLabel(tr("file")));
        row->addWidget(vnSoftSoilParameterFileEdit);
        auto *soilFileBrowseButton = new QPushButton(tr("Browse"), container);
        connect(soilFileBrowseButton, &QPushButton::clicked, this, &ModelCreatorWindow::chooseVnSoftSoilParameterFile);
        row->addWidget(soilFileBrowseButton);
        auto *vnRefTableButton = new QPushButton(tr("Soil params table"), container);
        connect(vnRefTableButton, &QPushButton::clicked, this, [this]() { showVnReferenceDefaultsTable(); });
        row->addWidget(vnRefTableButton);
        row->addStretch(1);
        layout->addWidget(container);
        vnSoftSoilParamsRowWidget = container;
    }
    observationObjectEdit->setPlaceholderText(tr("e.g. Soil (1$1)"));
    observationObjectEdit->setToolTip(tr("Target soil/layer object used for observation extraction in generated script."));
    observationExpressionEdit->setPlaceholderText(tr("e.g. theta"));
    observationExpressionEdit->setToolTip(tr("Observed quantity/expression, e.g. moisture variable theta."));
    observationNameEdit->setPlaceholderText(tr("e.g. Obs_1"));
    observationObjectRowWidget = addTextRow(layout, tr("Soil layer/object (observation target)"), observationObjectEdit);
    observationExpressionRowWidget = addTextRow(layout, tr("Moisture/expression (observation quantity)"), observationExpressionEdit);
    observationNameRowWidget = addTextRow(layout, tr("Observation series name"), observationNameEdit);
    showOptionalFieldsCheck = new QCheckBox(tr("Show optional fields"), this);
    showOptionalFieldsCheck->setChecked(false);
    layout->addWidget(showOptionalFieldsCheck);
    additionalCommandsEdit->setPlaceholderText(tr("Optional additional OHQ commands, one per line..."));
    auto *additionalRow = new QHBoxLayout();
    additionalRow->addWidget(new QLabel(tr("Additional OHQ commands")));
    additionalRow->addWidget(additionalCommandsEdit, 1);
    auto *loadCommandsButton = new QPushButton(tr("Load file"), this);
    loadCommandsButton->setToolTip(
        tr("Load VN layer/moisture snippets (.csv/.txt) or a full .ohq script into Additional OHQ commands."));
    connect(loadCommandsButton, &QPushButton::clicked, this, &ModelCreatorWindow::loadAdditionalCommandsFromFile);
    additionalRow->addWidget(loadCommandsButton);
    auto *additionalContainer = new QWidget(this);
    additionalContainer->setLayout(additionalRow);
    layout->addWidget(additionalContainer);
    additionalCommandsRowWidget = additionalContainer;
    auto *suggestedDefaultsButton = new QPushButton(tr("Apply suggested defaults"), this);
    suggestedDefaultsButton->setToolTip(tr("Fill empty setup fields using repository/OpenHydroQual path suggestions."));
    connect(suggestedDefaultsButton, &QPushButton::clicked, this, &ModelCreatorWindow::applySuggestedDefaults);
    layout->addWidget(suggestedDefaultsButton);

    auto *runSectionLabel = new QLabel(tr("OHQ run controls"), this);
    QFont runSectionFont = runSectionLabel->font();
    runSectionFont.setBold(true);
    runSectionLabel->setFont(runSectionFont);
    layout->addWidget(runSectionLabel);
    auto *actions = new QHBoxLayout();
    actions->addWidget(previewScriptButton);
    actions->addWidget(quickRunButton);
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
    resize(1000, 640);

    stopButton->setEnabled(false);

    connect(previewScriptButton, &QPushButton::clicked, this, &ModelCreatorWindow::previewScript);
    connect(quickRunButton, &QPushButton::clicked, this, &ModelCreatorWindow::quickGenerateRunAndSave);
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
    connect(modelTypeCombo, &QComboBox::currentTextChanged, this, [this]() { updateFieldVisibilityForContext(); });
    connect(workflowModeCombo, &QComboBox::currentTextChanged, this, [this]() { saveSettings(); updateFieldVisibilityForContext(); });
    connect(enrichmentPresetCombo, &QComboBox::currentTextChanged, this, [this]() { saveSettings(); });
    connect(enrichmentPresetCombo, &QComboBox::currentTextChanged, this, [this]() { updateFieldVisibilityForContext(); });
    connect(vnBuildModeCombo, &QComboBox::currentTextChanged, this, [this]() { updateFieldVisibilityForContext(); saveSettings(); });
    connect(showOptionalFieldsCheck, &QCheckBox::toggled, this, [this]() { updateFieldVisibilityForContext(); saveSettings(); });
    connect(allowGuiExecutionCheck, &QCheckBox::toggled, this, [this]() { saveSettings(); });

    const auto saveOnEdit = [this](QLineEdit *edit) {
        connect(edit, &QLineEdit::editingFinished, this, [this]() { saveSettings(); });
    };
    saveOnEdit(exePathEdit);
    saveOnEdit(exeArgsEdit);
    saveOnEdit(guiConfigTemplateEdit);
    saveOnEdit(scriptPathEdit);
    saveOnEdit(workingDirEdit);
    saveOnEdit(artifactsDirEdit);
    saveOnEdit(templateDirEdit);
    saveOnEdit(generatedScriptEdit);
    saveOnEdit(inflowFileEdit);
    saveOnEdit(simulationStartEdit);
    saveOnEdit(simulationEndEdit);
    saveOnEdit(ksatScaleEdit);
    saveOnEdit(ksatScaleGEdit);
    saveOnEdit(ksatScaleUwEdit);
    saveOnEdit(outputSeriesFileEdit);
    saveOnEdit(observationFileEdit);
    saveOnEdit(depthProfileFileEdit);
    saveOnEdit(vnBaseOhqFileEdit);
    saveOnEdit(vnSoilLayersFileEdit);
    saveOnEdit(vnMoistureLayersFileEdit);
    saveOnEdit(vnSoftGridXEdit);
    saveOnEdit(vnSoftGridYEdit);
    saveOnEdit(vnSoftUwGridXEdit);
    saveOnEdit(vnSoftUwGridYEdit);
    saveOnEdit(vnSoftCellSizeEdit);
    saveOnEdit(vnSoftUwCellSizeEdit);
    saveOnEdit(vnSoftGapSizeEdit);
    saveOnEdit(vnSoftRwGEdit);
    saveOnEdit(vnSoftRwUwEdit);
    saveOnEdit(vnSoftRadiusInfluenceEdit);
    saveOnEdit(vnSoftDepthWellCEdit);
    saveOnEdit(vnSoftDepthWellGEdit);
    saveOnEdit(vnSoftDepthToGwEdit);
    saveOnEdit(vnSoftTopElevationEdit);
    saveOnEdit(vnSoftLayerThicknessEdit);
    saveOnEdit(vnSoftSoilKsatOriginalEdit);
    saveOnEdit(vnSoftSoilAlphaEdit);
    saveOnEdit(vnSoftSoilNEdit);
    saveOnEdit(vnSoftSoilThetaSatEdit);
    saveOnEdit(vnSoftSoilThetaResEdit);
    saveOnEdit(vnSoftSoilParameterFileEdit);
    connect(vnSoftSoilParamModeCombo, &QComboBox::currentTextChanged, this, [this]() { saveSettings(); });
    auto updateVnSoftSoilModeUi = [this]() {
        const QString mode = vnSoftSoilParamModeCombo->currentData().toString().trimmed();
        const bool fileMode = mode.compare(QStringLiteral("File"), Qt::CaseInsensitive) == 0;
        vnSoftSoilParameterFileEdit->setEnabled(fileMode);
        vnSoftSoilKsatOriginalEdit->setEnabled(!fileMode);
        vnSoftSoilAlphaEdit->setEnabled(!fileMode);
        vnSoftSoilNEdit->setEnabled(!fileMode);
        vnSoftSoilThetaSatEdit->setEnabled(!fileMode);
        vnSoftSoilThetaResEdit->setEnabled(!fileMode);
    };
    connect(vnSoftSoilParamModeCombo, &QComboBox::currentTextChanged, this, updateVnSoftSoilModeUi);
    updateVnSoftSoilModeUi();
    saveOnEdit(observationObjectEdit);
    saveOnEdit(observationExpressionEdit);
    saveOnEdit(observationNameEdit);
    connect(additionalCommandsEdit, &QTextEdit::textChanged, this, [this]() { saveSettings(); });
    connect(vnBaseOhqFileEdit, &QLineEdit::editingFinished, this, [this]() { updateFieldVisibilityForContext(); });
    connect(exePathEdit, &QLineEdit::editingFinished, this, [this]() { updateFieldVisibilityForContext(); });

    connect(runner, &OHQProcessRunner::runStarted, this, [this]() {
        runStartedAt = QDateTime::currentDateTime();
        currentRunOutput.clear();
        suppressedRuntimeNoiseLines = 0;
        previewScriptButton->setEnabled(false);
        quickRunButton->setEnabled(false);
        generateScriptButton->setEnabled(false);
        generateAndRunButton->setEnabled(false);
        runButton->setEnabled(false);
        exportArtifactsButton->setEnabled(false);
        stopButton->setEnabled(true);
        appendLog(stamp(tr("Run started.")));
    });

    connect(runner, &OHQProcessRunner::outputReady, this, [this](const QString &text) {
        int suppressed = 0;
        const QString filtered = FilterRuntimeNoise(text, &suppressed);
        suppressedRuntimeNoiseLines += suppressed;
        currentRunOutput += filtered;
        if (!filtered.trimmed().isEmpty()) {
            appendLog(filtered);
        }
    });

    connect(runner, &OHQProcessRunner::runFinished, this, [this](int exitCode) {
        previewScriptButton->setEnabled(true);
        quickRunButton->setEnabled(true);
        generateScriptButton->setEnabled(true);
        generateAndRunButton->setEnabled(true);
        runButton->setEnabled(true);
        exportArtifactsButton->setEnabled(true);
        stopButton->setEnabled(false);
        if (suppressedRuntimeNoiseLines > 0) {
            appendLog(stamp(tr("Suppressed %1 known Qt runtime warning line(s).").arg(suppressedRuntimeNoiseLines)));
        }
        appendLog(stamp(tr("Run finished with exit code %1").arg(exitCode)));
        const bool parseConfigError = currentRunOutput.contains(QStringLiteral("Failed to parse configuration"), Qt::CaseInsensitive);
        if (parseConfigError && !pendingGuiRetryArgs.isEmpty()) {
            const QStringList retryArgs = pendingGuiRetryArgs.takeFirst();
            appendLog(stamp(tr("Detected configuration-parse error. Retrying GUI launch with args: %1")
                            .arg(retryArgs.join(' '))));
            runner->setExecutablePath(pendingGuiRetryExecutable);
            runner->runScript(pendingGuiRetryScript, pendingGuiRetryWorkingDirectory, retryArgs);
            return;
        }
        if (parseConfigError) {
            pendingGuiRetryArgs.clear();
            QMessageBox::warning(this,
                                 tr("Simulation did not start"),
                                 tr("OpenHydroQual reported a configuration parse error for all attempted argument patterns.\n\n"
                                    "This usually means the selected executable expects a JSON configuration file interface rather than direct .ohq execution.\n\n"
                                    "Try one of the following:\n"
                                    "1) Select an OHQ CLI solver binary if available.\n"
                                    "2) Provide explicit executable args required by your OpenHydroQual build.\n"
                                    "3) Use your server/worker runner flow (e.g., HQ_DrywellDT) for this build.\n"
                                    "4) Or select a custom internal solver executable (System/Solve main) and leave args empty."));
            appendLog(stamp(tr("Run ended without simulation: OpenHydroQual parse-configuration error persisted after fallback retries.")));
            return;
        } else {
            pendingGuiRetryArgs.clear();
        }
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
        pendingGuiRetryArgs.clear();
        previewScriptButton->setEnabled(true);
        quickRunButton->setEnabled(true);
        generateScriptButton->setEnabled(true);
        generateAndRunButton->setEnabled(true);
        runButton->setEnabled(true);
        exportArtifactsButton->setEnabled(true);
        stopButton->setEnabled(false);
        QString message = reason;
        const QFileInfo exeInfo(exePathEdit->text().trimmed());
        if (reason.contains("not a runnable file", Qt::CaseInsensitive)) {
            if (LooksLikeScriptFilePath(exeInfo)) {
                message += tr("\n\nHint: The executable field is set to a .ohq script. Move that path to 'OHQ script' and set 'OHQ executable' to the OHQ binary.");
            } else if (LooksLikeStaticLibraryPath(exeInfo)) {
                message += tr("\n\nHint: The executable field is set to a static library (.a). Select the OHQ binary executable instead.");
            }
        }
        QMessageBox::warning(this, tr("Run failed"), message);
        appendLog(stamp(tr("Run failed: %1").arg(reason)));
    });

    loadSettings();
    syncEnrichmentPresetForModel();
    updateFieldVisibilityForContext();
    refreshPlots();
}

void ModelCreatorWindow::syncEnrichmentPresetForModel()
{
    const QString modelType = modelTypeCombo->currentText().trimmed();
    const QString previousPreset = enrichmentPresetCombo->currentData().toString().trimmed();
    const QSignalBlocker blocker(enrichmentPresetCombo);
    enrichmentPresetCombo->clear();
    enrichmentPresetCombo->addItem(tr("None"), "");
    const auto options = StructureRegistry::PresetOptionsForModel(modelType);
    for (const auto &option : options) {
        enrichmentPresetCombo->addItem(option.first, option.second);
    }

    const int index = enrichmentPresetCombo->findData(previousPreset);
    enrichmentPresetCombo->setCurrentIndex(index >= 0 ? index : 0);
    if (index < 0 && !previousPreset.isEmpty()) {
        appendLog(stamp(tr("Preset '%1' hidden for model type '%2'; reset to None.")
                        .arg(previousPreset, modelType)));
    }
    saveSettings();
}

void ModelCreatorWindow::updateFieldVisibilityForContext()
{
    const QString workflowMode = workflowModeCombo->currentData().toString().trimmed();
    const bool loadExistingMode = workflowMode == QStringLiteral("load");
    const QString modelType = modelTypeCombo->currentText().trimmed();
    const QString preset = enrichmentPresetCombo->currentData().toString().trimmed();
    const bool vnContext = modelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0
        || preset.startsWith(QStringLiteral("VN_"));
    const bool usingVnBase = vnContext && !vnBaseOhqFileEdit->text().trimmed().isEmpty();
    const QString fallbackBuildMode = vnBuildModeCombo != nullptr
        ? vnBuildModeCombo->currentData().toString().trimmed()
        : QStringLiteral("SoftReference");
    const QString vnBuildMode = ResolveVnBuildModeForUi(modelType, preset, fallbackBuildMode);
    const bool explicitNonSoftMode = vnBuildMode.compare(QStringLiteral("FullReference"), Qt::CaseInsensitive) == 0
        || vnBuildMode.compare(QStringLiteral("LoadFromOhq"), Qt::CaseInsensitive) == 0
        || vnBuildMode.compare(QStringLiteral("Preset"), Qt::CaseInsensitive) == 0;
    const bool showOptional = showOptionalFieldsCheck != nullptr && showOptionalFieldsCheck->isChecked();
    const bool guiFallbackEnabled = allowGuiExecutionCheck != nullptr && allowGuiExecutionCheck->isChecked();
    const bool guiExecutableSelected = LooksLikeGuiOpenHydroQualExecutable(QFileInfo(exePathEdit->text().trimmed()));

    if (modelTypeRowWidget) modelTypeRowWidget->setVisible(!loadExistingMode);
    if (presetRowWidget) presetRowWidget->setVisible(!loadExistingMode);
    if (templateDirRowWidget) templateDirRowWidget->setVisible(!loadExistingMode && !usingVnBase);
    if (generatedScriptRowWidget) generatedScriptRowWidget->setVisible(!loadExistingMode);

    if (vnBuildModeRowWidget) vnBuildModeRowWidget->setVisible(false);
    if (vnBaseRowWidget) vnBaseRowWidget->setVisible(!loadExistingMode && vnContext);
    if (vnSoilRowWidget) vnSoilRowWidget->setVisible(!loadExistingMode && vnContext);
    if (vnMoistureRowWidget) vnMoistureRowWidget->setVisible(!loadExistingMode && vnContext);
    const bool showSoftRows = !loadExistingMode && vnContext && !explicitNonSoftMode;
    if (vnSoftGridXRowWidget) vnSoftGridXRowWidget->setVisible(showSoftRows);
    if (vnSoftGridYRowWidget) vnSoftGridYRowWidget->setVisible(showSoftRows);
    if (vnSoftCellSizeRowWidget) vnSoftCellSizeRowWidget->setVisible(showSoftRows);
    if (vnSoftUwGridXRowWidget) vnSoftUwGridXRowWidget->setVisible(showSoftRows);
    if (vnSoftUwGridYRowWidget) vnSoftUwGridYRowWidget->setVisible(showSoftRows);
    if (vnSoftUwCellSizeRowWidget) vnSoftUwCellSizeRowWidget->setVisible(showSoftRows);
    if (vnSoftGapSizeRowWidget) vnSoftGapSizeRowWidget->setVisible(showSoftRows);
    if (vnSoftRadiusRowWidget) vnSoftRadiusRowWidget->setVisible(showSoftRows);
    if (vnSoftDepthRowWidget) vnSoftDepthRowWidget->setVisible(showSoftRows);
    if (vnSoftTopElevationRowWidget) vnSoftTopElevationRowWidget->setVisible(showSoftRows);
    if (vnSoftLayerThicknessRowWidget) vnSoftLayerThicknessRowWidget->setVisible(showSoftRows);
    if (vnSoftSoilParamsRowWidget) vnSoftSoilParamsRowWidget->setVisible(showSoftRows);

    if (observationFileRowWidget) observationFileRowWidget->setVisible(showOptional);
    if (depthProfileRowWidget) depthProfileRowWidget->setVisible(showOptional);
    if (observationObjectRowWidget) observationObjectRowWidget->setVisible(showOptional);
    if (observationExpressionRowWidget) observationExpressionRowWidget->setVisible(showOptional);
    if (observationNameRowWidget) observationNameRowWidget->setVisible(showOptional);
    if (additionalCommandsRowWidget) additionalCommandsRowWidget->setVisible(showOptional);
    if (guiConfigTemplateRowWidget) guiConfigTemplateRowWidget->setVisible(guiFallbackEnabled || guiExecutableSelected || showOptional);

    // When a VN base script is provided, these generated-field rows are not required.
    if (inflowRowWidget) inflowRowWidget->setVisible(!loadExistingMode && !usingVnBase);
    if (simulationStartRowWidget) simulationStartRowWidget->setVisible(!loadExistingMode && !usingVnBase);
    if (simulationEndRowWidget) simulationEndRowWidget->setVisible(!loadExistingMode && !usingVnBase);
    if (outputSeriesRowWidget) outputSeriesRowWidget->setVisible(!loadExistingMode && !usingVnBase);
}

void ModelCreatorWindow::chooseExecutable()
{
    // Intentionally folder-based selection: users commonly picked non-executable
    // files when selecting "any file". We now ask for a root and auto-find OHQ.
    const QString startDir = exePathEdit->text().trimmed().isEmpty()
        ? FindRepoRoot()
        : QFileInfo(exePathEdit->text().trimmed()).absolutePath();
    const QString dir = QFileDialog::getExistingDirectory(this,
                                                          tr("Select OpenHydroQual folder (search OHQ CLI)"),
                                                          startDir);
    if (dir.isEmpty()) return;

    const QString cliPath = FindCliExecutableUnderRoot(dir);
    if (cliPath.isEmpty()) {
        QMessageBox::warning(this,
                             tr("OHQ CLI not found"),
                             tr("Could not find an executable named OHQ under:\n%1").arg(dir));
        appendLog(stamp(tr("No OHQ CLI executable found under: %1").arg(dir)));
        return;
    }

    exePathEdit->setText(cliPath);
    if (workingDirEdit->text().trimmed().isEmpty()) {
        workingDirEdit->setText(FindRepoRoot());
    }
    if (templateDirEdit->text().trimmed().isEmpty()) {
        const QStringList rootCandidates = CandidateOpenHydroQualRoots(FindRepoRoot(), {dir, cliPath});
        const QString detectedTemplate = DetectTemplateDirectory(rootCandidates, workingDirEdit->text().trimmed());
        if (!detectedTemplate.isEmpty()) {
            templateDirEdit->setText(detectedTemplate);
            appendLog(stamp(tr("Auto-detected template resources directory: %1").arg(detectedTemplate)));
        }
    }
    saveSettings();
    appendLog(stamp(tr("Selected OHQ CLI executable: %1").arg(cliPath)));
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
        const QStringList rootCandidates = CandidateOpenHydroQualRoots(FindRepoRoot(), {dir, exePathEdit->text().trimmed()});
        if (exePathEdit->text().trimmed().isEmpty()) {
            const QString detectedExecutable = DetectExecutablePath(rootCandidates);
            if (!detectedExecutable.isEmpty()) {
                exePathEdit->setText(detectedExecutable);
                appendLog(stamp(tr("Auto-detected OHQ executable from selected working directory: %1")
                                .arg(detectedExecutable)));
            }
        }
        if (templateDirEdit->text().trimmed().isEmpty()) {
            const QString detectedTemplate = DetectTemplateDirectory(rootCandidates, dir);
            if (!detectedTemplate.isEmpty()) {
                templateDirEdit->setText(detectedTemplate);
                appendLog(stamp(tr("Auto-detected template resources from selected working directory: %1")
                                .arg(detectedTemplate)));
            }
        }
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

void ModelCreatorWindow::applySuggestedDefaults()
{
    const QString repoRoot = FindRepoRoot();
    const QString suggestedWorkingDirectory = repoRoot;
    const QString suggestedArtifactsDirectory = QDir(suggestedWorkingDirectory).filePath("artifacts");
    const QStringList rootCandidates = CandidateOpenHydroQualRoots(repoRoot, {
        workingDirEdit->text().trimmed(),
        exePathEdit->text().trimmed(),
        templateDirEdit->text().trimmed()
    });
    const QString suggestedTemplateDirectory = DetectTemplateDirectory(rootCandidates, suggestedWorkingDirectory);
    const QString suggestedGeneratedScriptPath = QDir(suggestedWorkingDirectory).filePath("starter_generated.ohq");
    const QString suggestedExecutablePath = DetectExecutablePath(rootCandidates);
    const QString suggestedScriptPath = FirstExistingFile({
        QDir(suggestedWorkingDirectory).filePath("hq_drywell.ohq"),
        QDir(suggestedWorkingDirectory).filePath("vn_drywell.ohq"),
        QDir(suggestedWorkingDirectory).filePath("r_bioswale.ohq"),
        QDir(suggestedWorkingDirectory).filePath("examples/hq_drywell.ohq"),
        QDir(suggestedWorkingDirectory).filePath("examples/vn_drywell.ohq"),
        QDir(suggestedWorkingDirectory).filePath("examples/r_bioswale.ohq")
    });

    auto applyIfEmpty = [](QLineEdit *edit, const QString &value) {
        if (edit->text().trimmed().isEmpty() && !value.trimmed().isEmpty()) {
            edit->setText(value);
        }
    };

    applyIfEmpty(exePathEdit, suggestedExecutablePath);
    if (!suggestedExecutablePath.isEmpty()) {
        const QFileInfo currentExe(exePathEdit->text().trimmed());
        if (LooksLikeScriptFilePath(currentExe) || LooksLikeStaticLibraryPath(currentExe) || !currentExe.isExecutable()) {
            exePathEdit->setText(suggestedExecutablePath);
            appendLog(stamp(tr("Replaced invalid executable path with suggested OHQ binary: %1")
                            .arg(suggestedExecutablePath)));
        }
    }
    applyIfEmpty(scriptPathEdit, suggestedScriptPath);
    applyIfEmpty(workingDirEdit, suggestedWorkingDirectory);
    applyIfEmpty(artifactsDirEdit, suggestedArtifactsDirectory);
    applyIfEmpty(templateDirEdit, suggestedTemplateDirectory);
    applyIfEmpty(generatedScriptEdit, suggestedGeneratedScriptPath);
    applyIfEmpty(outputSeriesFileEdit, QStringLiteral("OHQ_output.txt"));
    applyIfEmpty(simulationStartEdit, QStringLiteral("44435"));
    applyIfEmpty(simulationEndEdit, QStringLiteral("44438"));

    saveSettings();
    appendLog(stamp(tr("Applied suggested defaults to empty setup fields.")));
}

void ModelCreatorWindow::quickGenerateRunAndSave()
{
    const bool loadExistingMode = workflowModeCombo->currentData().toString() == QStringLiteral("load");
    if (loadExistingMode) {
        if (artifactsDirEdit->text().trimmed().isEmpty()) {
            const QString fallbackArtifacts = QDir(workingDirEdit->text().trimmed()).filePath("artifacts");
            artifactsDirEdit->setText(fallbackArtifacts);
        }
        QDir().mkpath(artifactsDirEdit->text().trimmed());
        saveSettings();
        runScript();
        return;
    }

    applySuggestedDefaults();

    if (artifactsDirEdit->text().trimmed().isEmpty()) {
        const QString fallbackArtifacts = QDir(workingDirEdit->text().trimmed()).filePath("artifacts");
        artifactsDirEdit->setText(fallbackArtifacts);
    }
    QDir().mkpath(artifactsDirEdit->text().trimmed());
    saveSettings();

    if (!generateStarterScriptInternal()) {
        return;
    }
    runScript();
}

void ModelCreatorWindow::chooseGuiConfigTemplate()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Select OpenHydroQual GUI config template"),
                                                          guiConfigTemplateEdit->text(),
                                                          tr("JSON files (*.json);;All files (*.*)"));
    if (!fileName.isEmpty()) {
        guiConfigTemplateEdit->setText(fileName);
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
        suggestSimulationWindowFromInflow(fileName);
        saveSettings();
        refreshPlots();
    }
}

void ModelCreatorWindow::suggestSimulationWindowFromInflow(const QString &path)
{
    QString error;
    const QVector<QPointF> points = loadSeriesFromFile(path, &error);
    if (points.isEmpty()) {
        return;
    }

    double minX = points.first().x();
    double maxX = points.first().x();
    for (const QPointF &pt : points) {
        minX = qMin(minX, pt.x());
        maxX = qMax(maxX, pt.x());
    }

    const QString currentStart = simulationStartEdit->text().trimmed();
    const QString currentEnd = simulationEndEdit->text().trimmed();
    const bool usingDefaults = (currentStart.isEmpty() && currentEnd.isEmpty())
        || (currentStart == "44435" && currentEnd == "44438");
    if (!usingDefaults) {
        return;
    }

    simulationStartEdit->setText(QString::number(minX, 'g', 12));
    simulationEndEdit->setText(QString::number(maxX, 'g', 12));
    appendLog(stamp(tr("Suggested simulation window from inflow file: start=%1, end=%2")
                    .arg(simulationStartEdit->text(), simulationEndEdit->text())));
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

void ModelCreatorWindow::chooseVnBaseOhqFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Select VN base OHQ script"),
                                                          vnBaseOhqFileEdit->text(),
                                                          tr("OHQ/Text files (*.ohq *.txt);;All files (*.*)"));
    if (!fileName.isEmpty()) {
        vnBaseOhqFileEdit->setText(fileName);
        saveSettings();
    }
}

void ModelCreatorWindow::chooseVnSoilLayersFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Select VN soil layers snippet"),
                                                          vnSoilLayersFileEdit->text(),
                                                          tr("Supported files (*.ohq *.txt *.csv);;All files (*.*)"));
    if (!fileName.isEmpty()) {
        vnSoilLayersFileEdit->setText(fileName);
        saveSettings();
    }
}

void ModelCreatorWindow::chooseVnMoistureLayersFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Select VN moisture layers snippet"),
                                                          vnMoistureLayersFileEdit->text(),
                                                          tr("Supported files (*.ohq *.txt *.csv);;All files (*.*)"));
    if (!fileName.isEmpty()) {
        vnMoistureLayersFileEdit->setText(fileName);
        saveSettings();
    }
}

void ModelCreatorWindow::chooseVnSoftSoilParameterFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Select VN soft soil parameter profile CSV"),
                                                          vnSoftSoilParameterFileEdit->text(),
                                                          tr("CSV files (*.csv);;Text files (*.txt);;All files (*.*)"));
    if (!fileName.isEmpty()) {
        vnSoftSoilParameterFileEdit->setText(fileName);
        saveSettings();
    }
}

void ModelCreatorWindow::showVnReferenceDefaultsTable()
{
    const QString currentMode = vnSoftSoilParamModeCombo->currentData().toString().trimmed();
    const QString compactMode = currentMode.toLower().remove(' ').remove('_').remove('-');
    const bool fileMode = currentMode.compare(QStringLiteral("File"), Qt::CaseInsensitive) == 0
        || compactMode == QStringLiteral("file")
        || compactMode == QStringLiteral("filedepthprofile");
    const bool vnRefMode = currentMode.compare(QStringLiteral("VnReferenceDefaults"), Qt::CaseInsensitive) == 0
        || compactMode == QStringLiteral("vnrefdefaults")
        || compactMode == QStringLiteral("vnreferencedefaults");
    const bool modelCreatorDefaults =
        currentMode.compare(QStringLiteral("ModelCreatorDefaults"), Qt::CaseInsensitive) == 0
        || compactMode == QStringLiteral("modelcreatordefaults");

    QString csv;
    if (fileMode
        && !vnSoftSoilParameterFileEdit->text().trimmed().isEmpty()) {
        QFile file(vnSoftSoilParameterFileEdit->text().trimmed());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            csv = QString::fromUtf8(file.readAll());
        }
    } else if (vnRefMode) {
        csv = StarterScriptBuilder::VnReferenceSoilProfileCsv();
    } else {
        const double ksat = modelCreatorDefaults ? 1.05196 : vnSoftSoilKsatOriginalEdit->text().toDouble();
        const double alpha = modelCreatorDefaults ? 3.47536 : vnSoftSoilAlphaEdit->text().toDouble();
        const double n = modelCreatorDefaults ? 1.74582 : vnSoftSoilNEdit->text().toDouble();
        const double thetaSat = modelCreatorDefaults ? 0.39 : vnSoftSoilThetaSatEdit->text().toDouble();
        const double thetaRes = modelCreatorDefaults ? 0.049 : vnSoftSoilThetaResEdit->text().toDouble();
        csv = QStringLiteral("zone,act_Y,depth_m,Ksat,alpha,n,theta_sat,theta_res\n"
                             "Soil-g,-5.0,5.0,%1,%2,%3,%4,%5\n"
                             "Soil-uw,-15.0,15.0,%1,%2,%3,%4,%5\n")
                  .arg(ksat, 0, 'g', 10)
                  .arg(alpha, 0, 'g', 10)
                  .arg(n, 0, 'g', 10)
                  .arg(thetaSat, 0, 'g', 10)
                  .arg(thetaRes, 0, 'g', 10);
    }

    if (csv.trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Soil params table"), tr("Could not load soil-parameter profile for current mode."));
        return;
    }

    const QStringList lines = csv.split('\n', Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        QMessageBox::warning(this, tr("Soil params table"), tr("Soil-parameter table is empty."));
        return;
    }

    const QStringList headers = lines.first().split(',', Qt::KeepEmptyParts);
    auto *dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Soil params matrix (%1)").arg(currentMode.isEmpty() ? QStringLiteral("Manual") : currentMode));
    dialog->resize(760, 520);
    auto *layout = new QVBoxLayout(dialog);
    auto *table = new QTableWidget(dialog);
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);

    int rowIndex = 0;
    for (int i = 1; i < lines.size(); ++i) {
        const QString line = lines.at(i).trimmed();
        if (line.isEmpty()) {
            continue;
        }
        const QStringList cells = line.split(',', Qt::KeepEmptyParts);
        table->insertRow(rowIndex);
        for (int c = 0; c < headers.size(); ++c) {
            const QString value = c < cells.size() ? cells.at(c).trimmed() : QString();
            table->setItem(rowIndex, c, new QTableWidgetItem(value));
        }
        ++rowIndex;
    }
    table->resizeColumnsToContents();
    layout->addWidget(table);

    auto *buttonsRow = new QHBoxLayout();
    auto *applyManualBtn = new QPushButton(tr("Apply first row to Manual"), dialog);
    connect(applyManualBtn, &QPushButton::clicked, dialog, [this, table]() {
        if (table->rowCount() == 0) {
            return;
        }
        if (table->columnCount() < 8) {
            QMessageBox::warning(table, tr("Apply to Manual"),
                                 tr("Table format requires at least 8 columns: zone, act_Y, depth_m, Ksat, alpha, n, theta_sat, theta_res."));
            return;
        }
        const auto cellText = [table](int row, int col) {
            auto *item = table->item(row, col);
            return item ? item->text().trimmed() : QString();
        };
        bool okKsat = false, okAlpha = false, okN = false, okThetaSat = false, okThetaRes = false;
        const double ksat = cellText(0, 3).toDouble(&okKsat);
        const double alpha = cellText(0, 4).toDouble(&okAlpha);
        const double n = cellText(0, 5).toDouble(&okN);
        const double thetaSat = cellText(0, 6).toDouble(&okThetaSat);
        const double thetaRes = cellText(0, 7).toDouble(&okThetaRes);
        if (!(okKsat && okAlpha && okN && okThetaSat && okThetaRes)) {
            QMessageBox::warning(table, tr("Apply to Manual"), tr("First row has invalid numeric cells."));
            return;
        }
        vnSoftSoilKsatOriginalEdit->setText(QString::number(ksat, 'g', 10));
        vnSoftSoilAlphaEdit->setText(QString::number(alpha, 'g', 10));
        vnSoftSoilNEdit->setText(QString::number(n, 'g', 10));
        vnSoftSoilThetaSatEdit->setText(QString::number(thetaSat, 'g', 10));
        vnSoftSoilThetaResEdit->setText(QString::number(thetaRes, 'g', 10));
        const int manualIndex = vnSoftSoilParamModeCombo->findData(QStringLiteral("Manual"));
        if (manualIndex >= 0) {
            vnSoftSoilParamModeCombo->setCurrentIndex(manualIndex);
        }
        saveSettings();
    });
    buttonsRow->addWidget(applyManualBtn);

    auto *saveAsFileBtn = new QPushButton(tr("Save as File profile"), dialog);
    connect(saveAsFileBtn, &QPushButton::clicked, dialog, [this, table, headers, dialog]() {
        const QString target = QFileDialog::getSaveFileName(dialog,
                                                            tr("Save soil profile CSV"),
                                                            QDir(workingDirEdit->text().trimmed()).filePath("vn_soft_soil_profile.csv"),
                                                            tr("CSV files (*.csv);;All files (*.*)"));
        if (target.isEmpty()) {
            return;
        }
        QSaveFile out(target);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(dialog, tr("Save profile"), tr("Could not open file for writing."));
            return;
        }
        QTextStream ts(&out);
        ts << headers.join(',') << "\n";
        for (int r = 0; r < table->rowCount(); ++r) {
            QStringList rowValues;
            for (int c = 0; c < table->columnCount(); ++c) {
                auto *item = table->item(r, c);
                rowValues << (item ? item->text().trimmed() : QString());
            }
            ts << rowValues.join(',') << "\n";
        }
        if (!out.commit()) {
            QMessageBox::warning(dialog, tr("Save profile"), tr("Could not finalize saved CSV."));
            return;
        }
        vnSoftSoilParameterFileEdit->setText(target);
        const int fileIndex = vnSoftSoilParamModeCombo->findData(QStringLiteral("File"));
        if (fileIndex >= 0) {
            vnSoftSoilParamModeCombo->setCurrentIndex(fileIndex);
        }
        saveSettings();
    });
    buttonsRow->addWidget(saveAsFileBtn);

    auto *closeBtn = new QPushButton(tr("Close"), dialog);
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    buttonsRow->addWidget(closeBtn);
    layout->addLayout(buttonsRow);

    dialog->exec();
}

void ModelCreatorWindow::loadAdditionalCommandsFromFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Load soil/moisture layers or OHQ commands"),
                                                          {},
                                                          tr("Supported files (*.ohq *.txt *.csv);;All files (*.*)"));
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
    QString scriptText;
    const QString selectedScriptPath = scriptPathEdit->text().trimmed();
    if (!selectedScriptPath.isEmpty()) {
        QFile file(selectedScriptPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this,
                                 tr("Cannot open script"),
                                 tr("Failed to open selected script file for review.\nReason: %1")
                                     .arg(file.errorString()));
            return;
        }
        scriptText = QString::fromUtf8(file.readAll());
        appendLog(stamp(tr("Loaded selected script for review: %1").arg(selectedScriptPath)));
    } else {
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
        options.ksatScaleAll = ksatScaleEdit->text().trimmed();
        options.ksatScaleG = ksatScaleGEdit->text().trimmed();
        options.ksatScaleUw = ksatScaleUwEdit->text().trimmed();
        options.vnBaseOhqFile = vnBaseOhqFileEdit->text().trimmed();
        options.vnSoilLayersFile = vnSoilLayersFileEdit->text().trimmed();
        options.vnMoistureLayersFile = vnMoistureLayersFileEdit->text().trimmed();
        AssignIntIfProvided(vnSoftGridXEdit, &options.vnSoftGridXCount);
        AssignIntIfProvided(vnSoftGridYEdit, &options.vnSoftGridYCount);
        AssignIntIfProvided(vnSoftUwGridXEdit, &options.vnSoftUwGridXCount);
        AssignIntIfProvided(vnSoftUwGridYEdit, &options.vnSoftUwGridYCount);
        AssignDoubleIfProvided(vnSoftCellSizeEdit, &options.vnSoftCellSize);
        AssignDoubleIfProvided(vnSoftUwCellSizeEdit, &options.vnSoftUwCellSize);
        AssignDoubleIfProvided(vnSoftGapSizeEdit, &options.vnSoftGapSize);
        AssignDoubleIfProvided(vnSoftRwGEdit, &options.vnSoftRwG);
        AssignDoubleIfProvided(vnSoftRwUwEdit, &options.vnSoftRwUw);
        AssignDoubleIfProvided(vnSoftRadiusInfluenceEdit, &options.vnSoftRadiusOfInfluence);
        AssignDoubleIfProvided(vnSoftDepthWellCEdit, &options.vnSoftDepthOfWellC);
        AssignDoubleIfProvided(vnSoftDepthWellGEdit, &options.vnSoftDepthOfWellG);
        AssignDoubleIfProvided(vnSoftDepthToGwEdit, &options.vnSoftDepthToGroundWater);
        AssignDoubleIfProvided(vnSoftTopElevationEdit, &options.vnSoftTopElevation);
        AssignDoubleIfProvided(vnSoftLayerThicknessEdit, &options.vnSoftLayerThickness);
        AssignDoubleIfProvided(vnSoftSoilKsatOriginalEdit, &options.vnSoftSoilKsatOriginal);
        AssignDoubleIfProvided(vnSoftSoilAlphaEdit, &options.vnSoftSoilAlpha);
        AssignDoubleIfProvided(vnSoftSoilNEdit, &options.vnSoftSoilN);
        AssignDoubleIfProvided(vnSoftSoilThetaSatEdit, &options.vnSoftSoilThetaSat);
        AssignDoubleIfProvided(vnSoftSoilThetaResEdit, &options.vnSoftSoilThetaRes);
        options.vnSoftSoilParamMode = vnSoftSoilParamModeCombo->currentData().toString();
        options.vnSoftSoilParameterFile = vnSoftSoilParameterFileEdit->text().trimmed();
        if (options.modelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0) {
            const QString selectedPreset = options.enrichmentPreset.trimmed();
            const QString selectedVnBuildMode = VnBuildModeFromPresetSelection(selectedPreset);
            if (!selectedVnBuildMode.isEmpty()) {
                options.vnBuildMode = selectedVnBuildMode;
                options.enrichmentPreset.clear();
            } else {
                options.vnBuildMode = QStringLiteral("Preset");
                options.vnPreset = selectedPreset.isEmpty() ? QStringLiteral("VN_Drywell_Pro") : selectedPreset;
            }
        } else if (options.modelType.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0) {
            const QString selectedHqMode = BuildModeFromPresetSelection(options.enrichmentPreset, QStringLiteral("HQ_MODE"));
            if (!selectedHqMode.isEmpty()) {
                options.hqBuildMode = selectedHqMode;
                options.enrichmentPreset.clear();
            } else {
                options.hqBuildMode = QStringLiteral("Preset");
            }
        } else if (options.modelType.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0) {
            const QString selectedRMode = BuildModeFromPresetSelection(options.enrichmentPreset, QStringLiteral("R_MODE"));
            if (!selectedRMode.isEmpty()) {
                options.rBioswaleBuildMode = selectedRMode;
                options.enrichmentPreset.clear();
            } else {
                options.rBioswaleBuildMode = QStringLiteral("Preset");
            }
        }

        QString error;
        const bool canBuildDraft = StarterScriptBuilder::BuildText(options, &scriptText, &error);
        if (!canBuildDraft) {
            QMessageBox::information(this, tr("No script available"),
                                     tr("Could not build draft from current inputs and no script file is selected.\nReason: %1").arg(error));
            return;
        }
    }

    StarterScriptOptions options;
    options.outputFile = generatedScriptEdit->text().trimmed();
    if (options.outputFile.trimmed().isEmpty()) {
        options.outputFile = selectedScriptPath;
    }

    if (scriptText.isEmpty()) {
        const QString filePath = scriptPathEdit->text().trimmed();
        if (filePath.isEmpty()) {
            QMessageBox::information(this, tr("No script available"), tr("No script content is available to review."));
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
    const bool loadExistingMode = workflowModeCombo->currentData().toString() == QStringLiteral("load");
    if (loadExistingMode) {
        QMessageBox::information(this,
                                 tr("Generate from scratch disabled"),
                                 tr("Workflow mode is set to 'Load/Edit existing .ohq'.\n\n"
                                    "Switch to 'Generate from scratch' to build a new starter script."));
        return false;
    }

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
    options.ksatScaleAll = ksatScaleEdit->text().trimmed();
    options.ksatScaleG = ksatScaleGEdit->text().trimmed();
    options.ksatScaleUw = ksatScaleUwEdit->text().trimmed();
    options.vnBaseOhqFile = vnBaseOhqFileEdit->text().trimmed();
    options.vnSoilLayersFile = vnSoilLayersFileEdit->text().trimmed();
    options.vnMoistureLayersFile = vnMoistureLayersFileEdit->text().trimmed();
    AssignIntIfProvided(vnSoftGridXEdit, &options.vnSoftGridXCount);
    AssignIntIfProvided(vnSoftGridYEdit, &options.vnSoftGridYCount);
    AssignIntIfProvided(vnSoftUwGridXEdit, &options.vnSoftUwGridXCount);
    AssignIntIfProvided(vnSoftUwGridYEdit, &options.vnSoftUwGridYCount);
    AssignDoubleIfProvided(vnSoftCellSizeEdit, &options.vnSoftCellSize);
    AssignDoubleIfProvided(vnSoftUwCellSizeEdit, &options.vnSoftUwCellSize);
    AssignDoubleIfProvided(vnSoftGapSizeEdit, &options.vnSoftGapSize);
    AssignDoubleIfProvided(vnSoftRwGEdit, &options.vnSoftRwG);
    AssignDoubleIfProvided(vnSoftRwUwEdit, &options.vnSoftRwUw);
    AssignDoubleIfProvided(vnSoftRadiusInfluenceEdit, &options.vnSoftRadiusOfInfluence);
    AssignDoubleIfProvided(vnSoftDepthWellCEdit, &options.vnSoftDepthOfWellC);
    AssignDoubleIfProvided(vnSoftDepthWellGEdit, &options.vnSoftDepthOfWellG);
    AssignDoubleIfProvided(vnSoftDepthToGwEdit, &options.vnSoftDepthToGroundWater);
    AssignDoubleIfProvided(vnSoftTopElevationEdit, &options.vnSoftTopElevation);
    AssignDoubleIfProvided(vnSoftLayerThicknessEdit, &options.vnSoftLayerThickness);
    AssignDoubleIfProvided(vnSoftSoilKsatOriginalEdit, &options.vnSoftSoilKsatOriginal);
    AssignDoubleIfProvided(vnSoftSoilAlphaEdit, &options.vnSoftSoilAlpha);
    AssignDoubleIfProvided(vnSoftSoilNEdit, &options.vnSoftSoilN);
    AssignDoubleIfProvided(vnSoftSoilThetaSatEdit, &options.vnSoftSoilThetaSat);
    AssignDoubleIfProvided(vnSoftSoilThetaResEdit, &options.vnSoftSoilThetaRes);
    options.vnSoftSoilParamMode = vnSoftSoilParamModeCombo->currentData().toString();
    options.vnSoftSoilParameterFile = vnSoftSoilParameterFileEdit->text().trimmed();
    if (options.modelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0) {
        const QString selectedPreset = options.enrichmentPreset.trimmed();
        const QString selectedVnBuildMode = VnBuildModeFromPresetSelection(selectedPreset);
        if (!selectedVnBuildMode.isEmpty()) {
            options.vnBuildMode = selectedVnBuildMode;
            options.enrichmentPreset.clear();
        } else {
            options.vnBuildMode = QStringLiteral("Preset");
            options.vnPreset = selectedPreset.isEmpty() ? QStringLiteral("VN_Drywell_Pro") : selectedPreset;
        }
    } else if (options.modelType.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0) {
        const QString selectedHqMode = BuildModeFromPresetSelection(options.enrichmentPreset, QStringLiteral("HQ_MODE"));
        if (!selectedHqMode.isEmpty()) {
            options.hqBuildMode = selectedHqMode;
            options.enrichmentPreset.clear();
        } else {
            options.hqBuildMode = QStringLiteral("Preset");
        }
    } else if (options.modelType.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0) {
        const QString selectedRMode = BuildModeFromPresetSelection(options.enrichmentPreset, QStringLiteral("R_MODE"));
        if (!selectedRMode.isEmpty()) {
            options.rBioswaleBuildMode = selectedRMode;
            options.enrichmentPreset.clear();
        } else {
            options.rBioswaleBuildMode = QStringLiteral("Preset");
        }
    }
    const bool vnModel = options.modelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0;
    const bool usingExplicitVnBase = vnModel && !options.vnBaseOhqFile.isEmpty();

    if (!usingExplicitVnBase && options.templateDirectory.isEmpty()) {
        QMessageBox::warning(this, tr("Missing template directory"), tr("Please select the OHQ template resources directory first."));
        return false;
    }

    if (options.outputFile.isEmpty()) {
        QMessageBox::warning(this, tr("Missing output file"), tr("Please choose where to save the generated .ohq script."));
        return false;
    }

    if (options.inflowFile.isEmpty()) {
        if (vnModel) {
            options.inflowFile = QStringLiteral("Synthetic_rain_flow.csv");
            appendLog(stamp(tr("VN inflow was empty; using default inflow file: %1").arg(options.inflowFile)));
        } else {
            const bool hqModel = options.modelType.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0;
            const bool rModel = options.modelType.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0;
            const bool hqSoftReference = hqModel && options.hqBuildMode.compare(QStringLiteral("SoftReference"), Qt::CaseInsensitive) == 0;
            const bool rSoftReference = rModel && options.rBioswaleBuildMode.compare(QStringLiteral("SoftReference"), Qt::CaseInsensitive) == 0;
            if (hqSoftReference || rSoftReference) {
                QMessageBox::warning(this, tr("Missing inflow file"), tr("Please select an inflow file (.csv/.txt)."));
                return false;
            }
            appendLog(stamp(tr("%1 inflow was empty; keeping inflow configured in the reference script.")
                                .arg(options.modelType)));
        }
    }

    if (!usingExplicitVnBase && options.outputSeriesFile.isEmpty()) {
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

    const QString configuredExecutable = exePathEdit->text().trimmed();
    const QString autoDetectedExecutable = DetectExecutablePathFromContext(FindRepoRoot(),
                                                                           workingDirEdit->text().trimmed(),
                                                                           scriptPathEdit->text().trimmed(),
                                                                           templateDirEdit->text().trimmed(),
                                                                           configuredExecutable);
    QString resolvedExecutable = configuredExecutable;
    if (resolvedExecutable.isEmpty() && !autoDetectedExecutable.isEmpty()) {
        resolvedExecutable = autoDetectedExecutable;
        exePathEdit->setText(resolvedExecutable);
        appendLog(stamp(tr("Auto-detected OHQ executable: %1").arg(resolvedExecutable)));
    }

    QFileInfo exeInfo(resolvedExecutable);
    const QFileInfo scriptInfo(scriptPathEdit->text());
    const QFileInfo wdInfo(workingDirEdit->text());

    if (!exeInfo.exists() || !exeInfo.isFile()) {
        if (!autoDetectedExecutable.isEmpty()) {
            exeInfo = QFileInfo(autoDetectedExecutable);
            resolvedExecutable = autoDetectedExecutable;
            exePathEdit->setText(resolvedExecutable);
        } else {
            QMessageBox::warning(this,
                                 tr("Missing executable"),
                                 tr("Could not auto-detect an OHQ executable.\n\n"
                                    "Select the OpenHydroQual folder (or OHQ binary) once, or set an explicit executable path."));
            appendLog(stamp(tr("Run cancelled: OHQ executable was empty and auto-detection failed.")));
            return;
        }
    }

    if (!exeInfo.exists() || !exeInfo.isFile()) {
        QMessageBox::warning(this,
                             tr("Missing executable"),
                             tr("Could not find a valid OHQ executable: %1").arg(resolvedExecutable));
        appendLog(stamp(tr("Run cancelled: executable path does not exist: %1").arg(resolvedExecutable)));
        return;
    }

    if (LooksLikeScriptFilePath(exeInfo)) {
        // Frequent misconfiguration: script path copied into executable field.
        QMessageBox::warning(this,
                             tr("Executable path is a script"),
                             tr("The OHQ executable field currently points to a .ohq script file.\n\n"
                                "Please set OHQ executable to the runnable binary (for example, .../OHQ) and keep the script path in the OHQ script field."));
        appendLog(stamp(tr("Run cancelled: executable field points to script file '%1'.").arg(exeInfo.fileName())));
        return;
    }

    if (LooksLikeStaticLibraryPath(exeInfo)) {
        // Frequent misconfiguration: static library path picked as executable.
        QMessageBox::warning(this,
                             tr("Executable path is a static library"),
                             tr("The selected path appears to be a static library (.a), not a runnable executable.\n\n"
                                "Please select the OHQ binary executable."));
        appendLog(stamp(tr("Run cancelled: executable field points to static library '%1'.").arg(exeInfo.fileName())));
        return;
    }

    QString executablePathToRun = exeInfo.absoluteFilePath();
    if (!exeInfo.isExecutable()) {
        // Recover from non-runnable selections by attempting nearby CLI discovery.
        const QString discoveredCliPath = FindCliExecutableNearGui(exeInfo);
        const QFileInfo discoveredCliInfo(discoveredCliPath);
        if (!discoveredCliPath.isEmpty() && !IsGuiExecutableOrAlias(discoveredCliInfo)) {
            executablePathToRun = discoveredCliInfo.absoluteFilePath();
            exePathEdit->setText(executablePathToRun);
            appendLog(stamp(tr("Selected non-runnable path '%1'; auto-switched to CLI binary '%2'.")
                            .arg(exeInfo.fileName(), discoveredCliInfo.fileName())));
        } else {
            QMessageBox::warning(this,
                                 tr("Executable is not runnable"),
                                 tr("Selected path is not an executable file: %1\n\nPlease select the OHQ CLI binary.")
                                    .arg(exeInfo.absoluteFilePath()));
            appendLog(stamp(tr("Run cancelled: selected executable path is not runnable '%1'.")
                            .arg(exeInfo.fileName())));
            return;
        }
    }

    if (LooksLikeGuiOpenHydroQualExecutable(exeInfo)) {
        // GUI binary can be used for custom model-runner apps, but OHQ CLI is preferred.
        // If a nearby OHQ CLI exists, switch to it; otherwise continue with GUI as configured.
        const QString discoveredCliPath = FindCliExecutableNearGui(exeInfo);
        const QFileInfo discoveredCliInfo(discoveredCliPath);
        if (!discoveredCliPath.isEmpty() && !IsGuiExecutableOrAlias(discoveredCliInfo)) {
            executablePathToRun = discoveredCliInfo.absoluteFilePath();
            exePathEdit->setText(executablePathToRun);
            appendLog(stamp(tr("Selected GUI executable '%1'; auto-switched to CLI binary '%2'.")
                            .arg(exeInfo.fileName(), discoveredCliInfo.fileName())));
        } else {
            const bool allowGuiFallback = allowGuiExecutionCheck != nullptr && allowGuiExecutionCheck->isChecked();
            if (!allowGuiFallback) {
                QMessageBox::warning(this,
                                     tr("GUI execution disabled"),
                                     tr("No nearby OHQ CLI solver was found for:\n%1\n\n"
                                        "GUI fallback is disabled.\n"
                                        "Please select an OHQ CLI/internal solver executable (recommended) "
                                        "or enable 'Allow OpenHydroQual GUI execution fallback'.")
                                        .arg(exeInfo.absoluteFilePath()));
                appendLog(stamp(tr("Run cancelled: GUI executable selected and no CLI discovered. GUI fallback is disabled.")));
                return;
            }
            appendLog(stamp(tr("No nearby OHQ CLI discovered for '%1'; proceeding with GUI fallback because it is enabled.")
                            .arg(exeInfo.absoluteFilePath())));
        }
    }

    const QString configuredArgsTemplate = exeArgsEdit->text().trimmed();
    const QFileInfo executableToRunInfo(executablePathToRun);
    const bool executableLooksLikeCli = LooksLikeCliOhqBinaryName(executableToRunInfo.fileName());
    const bool executableLooksLikeGui = LooksLikeGuiOpenHydroQualExecutable(executableToRunInfo);
    const bool templateReferencesScript = configuredArgsTemplate.contains(QStringLiteral("{script}"));
    const bool noTemplateArgsProvided = configuredArgsTemplate.isEmpty();
    const bool passScriptAsPositionalDefault = noTemplateArgsProvided && executableLooksLikeCli;
    const bool passScriptWithRunFlagDefault = noTemplateArgsProvided && executableLooksLikeGui;
    const bool scriptRequired = templateReferencesScript || passScriptAsPositionalDefault || passScriptWithRunFlagDefault;

    if (scriptRequired && (!scriptInfo.exists() || !scriptInfo.isFile())) {
        QMessageBox::warning(this, tr("Missing script"), tr("Please select a valid .ohq script file."));
        appendLog(stamp(tr("Run cancelled: missing script file required by executable args.")));
        return;
    }

    if (!wdInfo.exists() || !wdInfo.isDir()) {
        QMessageBox::warning(this, tr("Missing directory"), tr("Please select a valid working directory."));
        appendLog(stamp(tr("Run cancelled: missing working directory.")));
        return;
    }

    if (!artifactsDirEdit->text().trimmed().isEmpty()) {
        const QString artifactsPath = artifactsDirEdit->text().trimmed();
        if (!QDir(artifactsPath).exists()) {
            if (QDir().mkpath(artifactsPath)) {
                appendLog(stamp(tr("Created artifacts directory: %1").arg(artifactsPath)));
            } else {
                QMessageBox::warning(this, tr("Invalid artifacts directory"), tr("Unable to create artifacts directory: %1").arg(artifactsPath));
                appendLog(stamp(tr("Run cancelled: failed to create artifacts directory '%1'.").arg(artifactsPath)));
                return;
            }
        }
        const QFileInfo artifactsDirInfo(artifactsPath);
        if (!artifactsDirInfo.exists() || !artifactsDirInfo.isDir()) {
            QMessageBox::warning(this, tr("Invalid artifacts directory"), tr("Please select a valid artifacts directory or leave it empty."));
            appendLog(stamp(tr("Run cancelled: artifacts path is not a directory '%1'.").arg(artifactsPath)));
            return;
        }
    }

    saveSettings();

    runner->setExecutablePath(executablePathToRun);
    QStringList executableArgs;
    pendingGuiRetryArgs.clear();
    pendingGuiRetryScript.clear();
    pendingGuiRetryWorkingDirectory.clear();
    pendingGuiRetryExecutable.clear();
    if (passScriptWithRunFlagDefault) {
        const QString guiConfigTemplatePath = guiConfigTemplateEdit->text().trimmed();
        if (!guiConfigTemplatePath.isEmpty()) {
            QString generatedConfigPath;
            QString configError;
            if (!BuildGuiConfigFromTemplate(guiConfigTemplatePath,
                                            scriptInfo.absoluteFilePath(),
                                            wdInfo.absoluteFilePath(),
                                            &generatedConfigPath,
                                            &configError)) {
                QMessageBox::warning(this, tr("GUI config template error"), configError);
                appendLog(stamp(tr("Run cancelled: %1").arg(configError)));
                return;
            }
            executableArgs = QStringList{generatedConfigPath};
            appendLog(stamp(tr("Generated GUI config from template: %1").arg(generatedConfigPath)));
        } else {
            executableArgs = QStringList{
                scriptInfo.absoluteFilePath(),
                QStringLiteral("--run")
            };
            pendingGuiRetryExecutable = executablePathToRun;
            pendingGuiRetryScript = scriptInfo.absoluteFilePath();
            pendingGuiRetryWorkingDirectory = wdInfo.absoluteFilePath();
            QString autoConfigPath;
            QString autoConfigError;
            if (BuildDefaultGuiConfig(scriptInfo.absoluteFilePath(),
                                      wdInfo.absoluteFilePath(),
                                      &autoConfigPath,
                                      &autoConfigError)) {
                pendingGuiRetryArgs << (QStringList{autoConfigPath});
                appendLog(stamp(tr("Prepared auto GUI JSON config candidate: %1").arg(autoConfigPath)));
            } else {
                appendLog(stamp(tr("Auto GUI JSON config was not created: %1").arg(autoConfigError)));
            }
            pendingGuiRetryArgs << (QStringList{QStringLiteral("--run"), scriptInfo.absoluteFilePath()})
                               << (QStringList{scriptInfo.absoluteFilePath()})
                               << (QStringList{QStringLiteral("--script"), scriptInfo.absoluteFilePath(), QStringLiteral("--run")});
        }
    } else if (passScriptAsPositionalDefault) {
        executableArgs = QStringList{scriptInfo.absoluteFilePath()};
    } else if (noTemplateArgsProvided) {
        executableArgs.clear();
    } else {
        executableArgs = BuildExecutableArguments(configuredArgsTemplate,
                                                  scriptInfo.absoluteFilePath());
    }

    auto appendFlagIfPresent = [&executableArgs](const QString &flag, const QString &value) {
        const QString trimmed = value.trimmed();
        if (trimmed.isEmpty()) {
            return;
        }
        for (int i = 0; i < executableArgs.size(); ++i) {
            const QString arg = executableArgs.at(i);
            if (arg == flag || arg.startsWith(flag + "=")) {
                return;
            }
        }
        executableArgs << flag << trimmed;
    };
    appendFlagIfPresent(QStringLiteral("--ksat-scale"), ksatScaleEdit->text());
    appendFlagIfPresent(QStringLiteral("--ksat-scale-g"), ksatScaleGEdit->text());
    appendFlagIfPresent(QStringLiteral("--ksat-scale-uw"), ksatScaleUwEdit->text());
    if (passScriptWithRunFlagDefault) {
        if (guiConfigTemplateEdit->text().trimmed().isEmpty()) {
            appendLog(stamp(tr("Executable looks like OpenHydroQual GUI; using default args: <script> --run")));
        } else {
            appendLog(stamp(tr("Executable looks like OpenHydroQual GUI; using generated JSON config argument.")));
        }
    }

    if (scriptRequired) {
        appendLog(stamp(tr("Running script: %1").arg(scriptInfo.absoluteFilePath())));
    } else {
        appendLog(stamp(tr("Running executable without implicit script argument: %1")
                        .arg(QFileInfo(executablePathToRun).fileName())));
    }
    appendLog(stamp(tr("Executable args: %1")
                    .arg(executableArgs.join(' ').trimmed().isEmpty() ? tr("(none)") : executableArgs.join(' '))));
    runner->runScript(scriptInfo.absoluteFilePath(), wdInfo.absoluteFilePath(), executableArgs);
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
    const auto settingTextOrDefault = [&settings](const QString &key, const QString &fallback) {
        const QString value = settings.value(key, fallback).toString().trimmed();
        return value.isEmpty() ? fallback : value;
    };
    const QString repoRoot = FindRepoRoot();
    const QString defaultWorkingDirectory = repoRoot;
    const QString defaultArtifactsDirectory = QDir(defaultWorkingDirectory).filePath("artifacts");
    const QStringList rootCandidates = CandidateOpenHydroQualRoots(repoRoot);
    const QString defaultTemplateDirectory = DetectTemplateDirectory(rootCandidates, defaultWorkingDirectory);
    const QString defaultGeneratedScriptPath = QDir(defaultWorkingDirectory).filePath("starter_generated.ohq");
    const QString defaultExecutablePath = DetectExecutablePath(rootCandidates);
    const QString defaultScriptPath = FirstExistingFile({
        QDir(defaultWorkingDirectory).filePath("hq_drywell.ohq"),
        QDir(defaultWorkingDirectory).filePath("vn_drywell.ohq"),
        QDir(defaultWorkingDirectory).filePath("r_bioswale.ohq"),
        QDir(defaultWorkingDirectory).filePath("examples/hq_drywell.ohq"),
        QDir(defaultWorkingDirectory).filePath("examples/vn_drywell.ohq"),
        QDir(defaultWorkingDirectory).filePath("examples/r_bioswale.ohq")
    });

    modelTypeCombo->setCurrentText(settings.value("modelType", "HQ_Drywell").toString());
    const int workflowIndex = workflowModeCombo->findData(settings.value("workflowMode", "generate").toString());
    workflowModeCombo->setCurrentIndex(workflowIndex >= 0 ? workflowIndex : 0);
    const QString enrichmentPreset = settings.value("enrichmentPreset").toString().trimmed();
    const int presetIndex = enrichmentPresetCombo->findData(enrichmentPreset);
    enrichmentPresetCombo->setCurrentIndex(presetIndex >= 0 ? presetIndex : 0);
    exePathEdit->setText(settings.value("ohqExecutable", defaultExecutablePath).toString());
    exeArgsEdit->setText(settings.value("ohqExecutableArgs").toString());
    guiConfigTemplateEdit->setText(settings.value("guiConfigTemplate").toString());
    scriptPathEdit->setText(settings.value("ohqScript", defaultScriptPath).toString());
    workingDirEdit->setText(settings.value("workingDirectory", defaultWorkingDirectory).toString());
    artifactsDirEdit->setText(settings.value("artifactsDirectory", defaultArtifactsDirectory).toString());
    templateDirEdit->setText(settings.value("templateDirectory",
                                            defaultTemplateDirectory.isEmpty() ? QDir(defaultWorkingDirectory).filePath("templates")
                                                                               : defaultTemplateDirectory).toString());
    generatedScriptEdit->setText(settings.value("generatedScriptPath", defaultGeneratedScriptPath).toString());
    inflowFileEdit->setText(settings.value("inflowFile").toString());
    simulationStartEdit->setText(settings.value("simulationStart", "44435").toString());
    simulationEndEdit->setText(settings.value("simulationEnd", "44438").toString());
    ksatScaleEdit->setText(settings.value("ksatScale").toString());
    ksatScaleGEdit->setText(settings.value("ksatScaleG").toString());
    ksatScaleUwEdit->setText(settings.value("ksatScaleUw").toString());
    outputSeriesFileEdit->setText(settings.value("outputSeriesFile", "OHQ_output.txt").toString());
    observationFileEdit->setText(settings.value("observationFile").toString());
    depthProfileFileEdit->setText(settings.value("depthProfileFile").toString());
    vnBaseOhqFileEdit->setText(settings.value("vnBaseOhqFile").toString());
    vnSoilLayersFileEdit->setText(settings.value("vnSoilLayersFile").toString());
    vnMoistureLayersFileEdit->setText(settings.value("vnMoistureLayersFile").toString());
    if (vnBuildModeCombo) {
        QString savedVnBuildMode = settings.value("vnBuildMode", "SoftReference").toString().trimmed();
        if (savedVnBuildMode.compare(QStringLiteral("Auto"), Qt::CaseInsensitive) == 0) {
            savedVnBuildMode = QStringLiteral("SoftReference");
        }
        const int vnBuildModeIndex = vnBuildModeCombo->findData(savedVnBuildMode.isEmpty() ? QStringLiteral("SoftReference") : savedVnBuildMode);
        vnBuildModeCombo->setCurrentIndex(vnBuildModeIndex >= 0 ? vnBuildModeIndex : 0);
        const bool vnModel = modelTypeCombo->currentText().compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0;
        const QString currentPreset = enrichmentPresetCombo->currentData().toString().trimmed();
        const bool shouldMigrateLegacyBuildMode = enrichmentPreset.isEmpty();
        if (vnModel
            && shouldMigrateLegacyBuildMode
            && !currentPreset.startsWith(QStringLiteral("VN_MODE:"), Qt::CaseInsensitive)) {
            const QString normalizedBuildMode = savedVnBuildMode.isEmpty() ? QStringLiteral("SoftReference") : savedVnBuildMode;
            if (normalizedBuildMode.compare(QStringLiteral("SoftReference"), Qt::CaseInsensitive) == 0
                || normalizedBuildMode.compare(QStringLiteral("FullReference"), Qt::CaseInsensitive) == 0
                || normalizedBuildMode.compare(QStringLiteral("LoadFromOhq"), Qt::CaseInsensitive) == 0) {
                const QString modePresetValue = QStringLiteral("VN_MODE:%1").arg(normalizedBuildMode);
                const int modePresetIndex = enrichmentPresetCombo->findData(modePresetValue);
                if (modePresetIndex >= 0) {
                    enrichmentPresetCombo->setCurrentIndex(modePresetIndex);
                }
            }
        }

        const QString resolvedBuildMode = ResolveVnBuildModeForUi(modelTypeCombo->currentText().trimmed(),
                                                                  enrichmentPresetCombo->currentData().toString().trimmed(),
                                                                  savedVnBuildMode);
        const int resolvedBuildModeIndex = vnBuildModeCombo->findData(resolvedBuildMode);
        if (resolvedBuildModeIndex >= 0) {
            vnBuildModeCombo->setCurrentIndex(resolvedBuildModeIndex);
        }
    }
    vnSoftGridXEdit->setText(settingTextOrDefault("vnSoftGridXCount", "16"));
    vnSoftGridYEdit->setText(settingTextOrDefault("vnSoftGridYCount", "15"));
    vnSoftUwGridXEdit->setText(settingTextOrDefault("vnSoftUwGridXCount", "16"));
    vnSoftUwGridYEdit->setText(settingTextOrDefault("vnSoftUwGridYCount", "12"));
    vnSoftCellSizeEdit->setText(settingTextOrDefault("vnSoftCellSize", "586.9"));
    vnSoftUwCellSizeEdit->setText(settingTextOrDefault("vnSoftUwCellSize", "586.9"));
    vnSoftGapSizeEdit->setText(settingTextOrDefault("vnSoftGapSize", "0.0"));
    vnSoftRwGEdit->setText(settingTextOrDefault("vnSoftRwG", "1.2192"));
    vnSoftRwUwEdit->setText(settingTextOrDefault("vnSoftRwUw", "1.2192"));
    vnSoftRadiusInfluenceEdit->setText(settingTextOrDefault("vnSoftRadiusOfInfluence", "20.0"));
    vnSoftDepthWellCEdit->setText(settingTextOrDefault("vnSoftDepthOfWellC", "4.8768"));
    vnSoftDepthWellGEdit->setText(settingTextOrDefault("vnSoftDepthOfWellG", "7.3152"));
    vnSoftDepthToGwEdit->setText(settingTextOrDefault("vnSoftDepthToGroundWater", "43.2816"));
    vnSoftTopElevationEdit->setText(settingTextOrDefault("vnSoftTopElevation", "-5.0"));
    vnSoftLayerThicknessEdit->setText(settingTextOrDefault("vnSoftLayerThickness", "1.0"));
    vnSoftSoilKsatOriginalEdit->setText(settingTextOrDefault("vnSoftSoilKsatOriginal", "1.05196"));
    vnSoftSoilAlphaEdit->setText(settingTextOrDefault("vnSoftSoilAlpha", "3.47536"));
    vnSoftSoilNEdit->setText(settingTextOrDefault("vnSoftSoilN", "1.74582"));
    vnSoftSoilThetaSatEdit->setText(settingTextOrDefault("vnSoftSoilThetaSat", "0.39"));
    vnSoftSoilThetaResEdit->setText(settingTextOrDefault("vnSoftSoilThetaRes", "0.049"));
    const QString vnSoftSoilParamMode = settingTextOrDefault("vnSoftSoilParamMode", "VnReferenceDefaults");
    const int vnSoftSoilParamModeIndex = vnSoftSoilParamModeCombo->findData(vnSoftSoilParamMode);
    vnSoftSoilParamModeCombo->setCurrentIndex(vnSoftSoilParamModeIndex >= 0 ? vnSoftSoilParamModeIndex : 0);
    vnSoftSoilParameterFileEdit->setText(settings.value("vnSoftSoilParameterFile").toString());
    if (showOptionalFieldsCheck) {
        showOptionalFieldsCheck->setChecked(settings.value("showOptionalFields", false).toBool());
    }
    if (allowGuiExecutionCheck) {
        allowGuiExecutionCheck->setChecked(settings.value("allowGuiExecutionFallback", false).toBool());
    }
    observationObjectEdit->setText(settings.value("observationObject", "Soil (1$1)").toString());
    observationExpressionEdit->setText(settings.value("observationExpression", "theta").toString());
    observationNameEdit->setText(settings.value("observationName", "Obs_1").toString());
    additionalCommandsEdit->setPlainText(settings.value("additionalCommands").toString());
}

void ModelCreatorWindow::saveSettings() const
{
    QSettings settings("DryWellScriptGenerator", "ModelCreatorRunner");
    settings.setValue("modelType", modelTypeCombo->currentText());
    settings.setValue("workflowMode", workflowModeCombo->currentData().toString());
    settings.setValue("enrichmentPreset", enrichmentPresetCombo->currentData().toString());
    settings.setValue("ohqExecutable", exePathEdit->text());
    settings.setValue("ohqExecutableArgs", exeArgsEdit->text());
    settings.setValue("guiConfigTemplate", guiConfigTemplateEdit->text());
    settings.setValue("ohqScript", scriptPathEdit->text());
    settings.setValue("workingDirectory", workingDirEdit->text());
    settings.setValue("artifactsDirectory", artifactsDirEdit->text());
    settings.setValue("templateDirectory", templateDirEdit->text());
    settings.setValue("generatedScriptPath", generatedScriptEdit->text());
    settings.setValue("inflowFile", inflowFileEdit->text());
    settings.setValue("simulationStart", simulationStartEdit->text());
    settings.setValue("simulationEnd", simulationEndEdit->text());
    settings.setValue("ksatScale", ksatScaleEdit->text());
    settings.setValue("ksatScaleG", ksatScaleGEdit->text());
    settings.setValue("ksatScaleUw", ksatScaleUwEdit->text());
    settings.setValue("outputSeriesFile", outputSeriesFileEdit->text());
    settings.setValue("observationFile", observationFileEdit->text());
    settings.setValue("depthProfileFile", depthProfileFileEdit->text());
    settings.setValue("vnBaseOhqFile", vnBaseOhqFileEdit->text());
    settings.setValue("vnSoilLayersFile", vnSoilLayersFileEdit->text());
    settings.setValue("vnMoistureLayersFile", vnMoistureLayersFileEdit->text());
    if (vnBuildModeCombo) {
        const QString modelType = modelTypeCombo->currentText().trimmed();
        const QString preset = enrichmentPresetCombo->currentData().toString().trimmed();
        const QString fallbackBuildMode = vnBuildModeCombo->currentData().toString().trimmed();
        const QString resolvedBuildMode = ResolveVnBuildModeForUi(modelType, preset, fallbackBuildMode);
        settings.setValue("vnBuildMode", resolvedBuildMode);
    }
    settings.setValue("vnSoftGridXCount", vnSoftGridXEdit->text());
    settings.setValue("vnSoftGridYCount", vnSoftGridYEdit->text());
    settings.setValue("vnSoftUwGridXCount", vnSoftUwGridXEdit->text());
    settings.setValue("vnSoftUwGridYCount", vnSoftUwGridYEdit->text());
    settings.setValue("vnSoftCellSize", vnSoftCellSizeEdit->text());
    settings.setValue("vnSoftUwCellSize", vnSoftUwCellSizeEdit->text());
    settings.setValue("vnSoftGapSize", vnSoftGapSizeEdit->text());
    settings.setValue("vnSoftRwG", vnSoftRwGEdit->text());
    settings.setValue("vnSoftRwUw", vnSoftRwUwEdit->text());
    settings.setValue("vnSoftRadiusOfInfluence", vnSoftRadiusInfluenceEdit->text());
    settings.setValue("vnSoftDepthOfWellC", vnSoftDepthWellCEdit->text());
    settings.setValue("vnSoftDepthOfWellG", vnSoftDepthWellGEdit->text());
    settings.setValue("vnSoftDepthToGroundWater", vnSoftDepthToGwEdit->text());
    settings.setValue("vnSoftTopElevation", vnSoftTopElevationEdit->text());
    settings.setValue("vnSoftLayerThickness", vnSoftLayerThicknessEdit->text());
    settings.setValue("vnSoftSoilKsatOriginal", vnSoftSoilKsatOriginalEdit->text());
    settings.setValue("vnSoftSoilAlpha", vnSoftSoilAlphaEdit->text());
    settings.setValue("vnSoftSoilN", vnSoftSoilNEdit->text());
    settings.setValue("vnSoftSoilThetaSat", vnSoftSoilThetaSatEdit->text());
    settings.setValue("vnSoftSoilThetaRes", vnSoftSoilThetaResEdit->text());
    settings.setValue("vnSoftSoilParamMode", vnSoftSoilParamModeCombo->currentData().toString());
    settings.setValue("vnSoftSoilParameterFile", vnSoftSoilParameterFileEdit->text());
    if (showOptionalFieldsCheck) {
        settings.setValue("showOptionalFields", showOptionalFieldsCheck->isChecked());
    }
    if (allowGuiExecutionCheck) {
        settings.setValue("allowGuiExecutionFallback", allowGuiExecutionCheck->isChecked());
    }
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
