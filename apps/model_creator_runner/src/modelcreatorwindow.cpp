// NOTE: This file is part of the DryWellSuite/OpenHydroQual codebase.
#include "modelcreatorwindow.h"

#include "ohqprocessrunner.h"
#include "simplelineplotwidget.h"
#include "starter_script_builder.h"
#include "structure_registry.h"
#include "scripteditordialog.h"
#include "hq_drywell_builder.h"
#include "jm_bioretention_builder.h"
#include "r_bioswale_builder.h"
#include "vn_drywell_builder.h"

#include <QComboBox>
#include <QCheckBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
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
#include <QSet>
#include <functional>
#include <QSettings>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextStream>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
QString stamp(const QString &message)
{
    return QString("[%1] %2")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODate), message);
}

QString SpreadsheetSerialToIsoString(double serialDay)
{
    if (!std::isfinite(serialDay)) {
        return QString();
    }
    const int wholeDays = static_cast<int>(std::floor(serialDay));
    const double frac = serialDay - static_cast<double>(wholeDays);
    const int secs = qBound(0, static_cast<int>(std::round(frac * 86400.0)), 86399);
    const QDate base(1899, 12, 30);
    const QDate date = base.addDays(wholeDays);
    if (!date.isValid()) {
        return QString();
    }
    return QDateTime(date, QTime(0, 0).addSecs(secs), Qt::UTC).toString(Qt::ISODate);
}

void UpdateSimulationDateTooltip(QLineEdit *edit)
{
    if (edit == nullptr) {
        return;
    }
    bool ok = false;
    const double serial = edit->text().trimmed().toDouble(&ok);
    if (!ok) {
        edit->setToolTip(QString());
        return;
    }
    const QString iso = SpreadsheetSerialToIsoString(serial);
    if (iso.isEmpty()) {
        edit->setToolTip(QString());
        return;
    }
    edit->setToolTip(QObject::tr("Approx. UTC date-time: %1").arg(iso));
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
    if (trimmedPreset.isEmpty()) {
        return QStringLiteral("SoftReference");
    }
    if (trimmedPreset.startsWith(QStringLiteral("VN_"), Qt::CaseInsensitive)) {
        return QStringLiteral("SoftReference");
    }

    return fallbackBuildMode;
}

QString InferErtBoreholeName(double radiusM)
{
    if (!std::isfinite(radiusM)) {
        return QStringLiteral("ERT");
    }
    if (std::fabs(radiusM - 3.0) <= 0.25) {
        return QStringLiteral("ERT-3");
    }
    if (std::fabs(radiusM - 5.0) <= 0.25) {
        return QStringLiteral("ERT-5");
    }
    return QStringLiteral("R_%1m").arg(QString::number(radiusM, 'g', 6));
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

QString DetectLatestTerminalBuildExecutable(const QString &rootPath)
{
    const QDir root(rootPath);
    if (!root.exists()) {
        return QString();
    }

    const QDir terminalDir(root.filePath("terminal"));
    if (!terminalDir.exists()) {
        return QString();
    }

    QFileInfo newestMatch;
    QDirIterator it(terminalDir.absolutePath(),
                    QDir::Files | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QFileInfo info = it.fileInfo();
        if (!info.isExecutable()) {
            continue;
        }
        const QString fileName = info.fileName();
        if (fileName.compare(QStringLiteral("OpenHydroQual-Console"), Qt::CaseInsensitive) != 0
            && fileName.compare(QStringLiteral("OpenHydroQual-Console.exe"), Qt::CaseInsensitive) != 0
            && fileName.compare(QStringLiteral("OHQ"), Qt::CaseInsensitive) != 0
            && fileName.compare(QStringLiteral("OHQ.exe"), Qt::CaseInsensitive) != 0) {
            continue;
        }
        const QString absPath = info.absoluteFilePath();
        if (!absPath.contains(QStringLiteral("/build"), Qt::CaseInsensitive)) {
            continue;
        }
        if (!newestMatch.exists() || info.lastModified() > newestMatch.lastModified()) {
            newestMatch = info;
        }
    }

    return newestMatch.exists() ? newestMatch.absoluteFilePath() : QString();
}

QString DetectExecutablePath(const QStringList &rootCandidates)
{
    QFileInfo newestTerminalBuildExecutable;
    for (const QString &rootPath : rootCandidates) {
        const QString terminalCandidate = DetectLatestTerminalBuildExecutable(rootPath);
        if (terminalCandidate.isEmpty()) {
            continue;
        }
        const QFileInfo info(terminalCandidate);
        if (!newestTerminalBuildExecutable.exists()
            || info.lastModified() > newestTerminalBuildExecutable.lastModified()) {
            newestTerminalBuildExecutable = info;
        }
    }
    if (newestTerminalBuildExecutable.exists()) {
        return newestTerminalBuildExecutable.absoluteFilePath();
    }

    for (const QString &rootPath : rootCandidates) {
        const QString candidate = FindCliExecutableUnderRoot(rootPath);
        if (!candidate.isEmpty()) {
            return candidate;
        }
    }
    return QString();
}

QStringList CandidateProjectRootsFromTemplateDirectoryUi(const QString &templateDirectory)
{
    QStringList roots;
    const QFileInfo templateInfo(templateDirectory);
    if (templateInfo.exists()) {
        QDir dir = templateInfo.isDir() ? QDir(templateInfo.absoluteFilePath())
                                        : templateInfo.absoluteDir();
        if (dir.dirName().compare(QStringLiteral("resources"), Qt::CaseInsensitive) == 0) {
            dir.cdUp();
        }
        if (dir.dirName().compare(QStringLiteral("OpenHydroQual"), Qt::CaseInsensitive) == 0) {
            dir.cdUp();
            const QString inferredRoot = dir.absolutePath();
            if (!inferredRoot.trimmed().isEmpty()) {
                roots.prepend(inferredRoot);
            }
        }
    }
    roots.removeDuplicates();
    return roots;
}

bool IsKnownReferenceInflowForOtherModelUi(const QString &inflowPath, const QString &targetModel)
{
    const auto extractValue = [](const QString &line, const QString &key) -> QString {
        const QString token = key + QStringLiteral("=");
        const int start = line.indexOf(token, 0, Qt::CaseInsensitive);
        if (start < 0) {
            return {};
        }
        const int valueStart = start + token.size();
        int end = line.indexOf(',', valueStart);
        if (end < 0) {
            end = line.size();
        }
        return line.mid(valueStart, end - valueStart).trimmed();
    };
    const auto embeddedInflow = [&](const QString &model) -> QString {
        QString script;
        QString target;
        if (model.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0) {
            script = HqDrywellBuilder::FullReferenceScript();
            target = HqDrywellBuilder::InflowTargetObject();
        } else if (model.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0) {
            script = RBioswaleBuilder::FullReferenceScript();
            target = RBioswaleBuilder::InflowTargetObject();
        } else {
            script = VnDrywellBuilder::VnFullReferenceScript();
            target = VnDrywellBuilder::InflowTargetObject();
        }
        const QStringList lines = script.split('\n', Qt::SkipEmptyParts);
        for (const QString &rawLine : lines) {
            const QString line = rawLine.trimmed();
            if (line.contains(QStringLiteral("quantity=inflow"), Qt::CaseInsensitive)
                && line.contains(QStringLiteral("object=%1").arg(target), Qt::CaseInsensitive)) {
                const QString value = extractValue(line, QStringLiteral("value"));
                if (!value.trimmed().isEmpty()) {
                    return value.trimmed();
                }
            }
            if (line.startsWith(QStringLiteral("create block;"), Qt::CaseInsensitive)
                && line.contains(QStringLiteral("name=%1").arg(target), Qt::CaseInsensitive)
                && line.contains(QStringLiteral("inflow="), Qt::CaseInsensitive)) {
                const QString value = extractValue(line, QStringLiteral("inflow"));
                if (!value.trimmed().isEmpty()) {
                    return value.trimmed();
                }
            }
        }
        return QString();
    };

    const QString p = inflowPath.trimmed();
    if (p.isEmpty()) {
        return false;
    }
    const QString vnRef = embeddedInflow(QStringLiteral("VN_Drywell"));
    const QString hqRef = embeddedInflow(QStringLiteral("HQ_Drywell"));
    const QString rRef = embeddedInflow(QStringLiteral("R_Bioswale"));
    const QString pName = QFileInfo(p).fileName();
    const QString vnName = vnRef.isEmpty() ? QStringLiteral("LA_Precipitaion (5 yr new).csv") : QFileInfo(vnRef).fileName();
    const QString vnLegacyName = QStringLiteral("Synthetic_rain_flow.csv");
    const QString hqName = hqRef.isEmpty() ? QStringLiteral("Inflow_Corrected_New_Khiem.csv") : QFileInfo(hqRef).fileName();
    const QString rName = rRef.isEmpty() ? QStringLiteral("Inflow_Rosemead_August.txt") : QFileInfo(rRef).fileName();
    const bool isVnRef = (!vnRef.isEmpty() && p.compare(vnRef, Qt::CaseInsensitive) == 0)
        || pName.compare(vnName, Qt::CaseInsensitive) == 0
        || pName.compare(vnLegacyName, Qt::CaseInsensitive) == 0;
    const bool isHqRef = (!hqRef.isEmpty() && p.compare(hqRef, Qt::CaseInsensitive) == 0)
        || pName.compare(hqName, Qt::CaseInsensitive) == 0;
    const bool isRRef = (!rRef.isEmpty() && p.compare(rRef, Qt::CaseInsensitive) == 0)
        || pName.compare(rName, Qt::CaseInsensitive) == 0;
    if (targetModel.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0) {
        return isHqRef || isRRef;
    }
    if (targetModel.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0) {
        return isVnRef || isRRef;
    }
    if (targetModel.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0) {
        return isVnRef || isHqRef;
    }
    return false;
}

bool IsAutoSuggestedField(const QLineEdit *edit)
{
    return edit && edit->property("autoSuggested").toBool();
}

void SetAutoSuggestedField(QLineEdit *edit, bool autoSuggested)
{
    if (edit) {
        edit->setProperty("autoSuggested", autoSuggested);
    }
}

bool ApplySuggestedFieldValue(QLineEdit *edit, const QString &value)
{
    if (!edit || value.trimmed().isEmpty()) {
        return false;
    }
    if (edit->text().trimmed().isEmpty() || IsAutoSuggestedField(edit)) {
        edit->setText(value);
        SetAutoSuggestedField(edit, true);
        return true;
    }
    return false;
}

QString DetectSuggestedInflowFile(const QString &modelType, const QString &templateDirectory = QString())
{
    const auto extractValue = [](const QString &line, const QString &key) -> QString {
        const QString token = key + QStringLiteral("=");
        const int start = line.indexOf(token, 0, Qt::CaseInsensitive);
        if (start < 0) {
            return {};
        }
        const int valueStart = start + token.size();
        int end = line.indexOf(',', valueStart);
        if (end < 0) {
            end = line.size();
        }
        return line.mid(valueStart, end - valueStart).trimmed();
    };
    const auto embeddedInflow = [&](const QString &model) -> QString {
        QString script;
        QString target;
        if (model.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0) {
            script = HqDrywellBuilder::FullReferenceScript();
            target = HqDrywellBuilder::InflowTargetObject();
        } else if (model.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0) {
            script = RBioswaleBuilder::FullReferenceScript();
            target = RBioswaleBuilder::InflowTargetObject();
        } else {
            script = VnDrywellBuilder::VnFullReferenceScript();
            target = VnDrywellBuilder::InflowTargetObject();
        }
        const QStringList lines = script.split('\n', Qt::SkipEmptyParts);
        for (const QString &rawLine : lines) {
            const QString line = rawLine.trimmed();
            if (line.contains(QStringLiteral("quantity=inflow"), Qt::CaseInsensitive)
                && line.contains(QStringLiteral("object=%1").arg(target), Qt::CaseInsensitive)) {
                const QString value = extractValue(line, QStringLiteral("value"));
                if (!value.trimmed().isEmpty()) {
                    return value.trimmed();
                }
            }
            if (line.startsWith(QStringLiteral("create block;"), Qt::CaseInsensitive)
                && line.contains(QStringLiteral("name=%1").arg(target), Qt::CaseInsensitive)
                && line.contains(QStringLiteral("inflow="), Qt::CaseInsensitive)) {
                const QString value = extractValue(line, QStringLiteral("inflow"));
                if (!value.trimmed().isEmpty()) {
                    return value.trimmed();
                }
            }
        }
        return QString();
    };

    const QString normalizedModel = modelType.trimmed();
    QStringList candidates;
    const QStringList projectRoots = CandidateProjectRootsFromTemplateDirectoryUi(templateDirectory);
    const QString embeddedDefault = embeddedInflow(normalizedModel);
    if (!embeddedDefault.trimmed().isEmpty()) {
        return embeddedDefault.trimmed();
    }
    if (normalizedModel.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0) {
        for (const QString &root : projectRoots) {
            candidates << QDir(root).filePath(QStringLiteral("LA Project/Data/Inflow_Corrected_New_Khiem.csv"));
        }
    } else if (normalizedModel.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0) {
        for (const QString &root : projectRoots) {
            candidates << QDir(root).filePath(QStringLiteral("LA Project/Data/Inflow_Rosemead_August.txt"));
        }
    } else if (normalizedModel.compare(QStringLiteral("JM_Bioretention"), Qt::CaseInsensitive) == 0) {
        return QString();
    } else {
        for (const QString &root : projectRoots) {
            candidates << QDir(root).filePath(QStringLiteral("VN Drywell_Models/LA_Precipitaion (5 yr new).csv"));
        }
    }

    const QString detected = FirstExistingFile(candidates);
    if (!detected.isEmpty()) {
        return detected;
    }

    // Candidate roots are inferred from the repository/template layout.  A
    // relocated standalone runner can legitimately have no matching root (and
    // an older or incomplete embedded reference may not provide an inflow
    // either).  Do not call front() on the resulting empty list: selecting a
    // structure must still be safe, and the normal validation flow can ask the
    // user for an inflow file when one is required.
    return candidates.isEmpty() ? QString() : candidates.constFirst();
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

bool IsVnSoftCustomizationRequested(const StarterScriptOptions &options)
{
    const StarterScriptOptions defaults;
    constexpr double kEpsilon = 1e-9;
    const auto differs = [](double lhs, double rhs) {
        return std::fabs(lhs - rhs) > kEpsilon;
    };
    const QString mode = options.vnSoftSoilParamMode.trimmed();
    const QString defaultMode = defaults.vnSoftSoilParamMode.trimmed();
    return IsVnSoftGridCustomized(options)
        || !options.vnSoilLayersFile.trimmed().isEmpty()
        || !options.vnMoistureLayersFile.trimmed().isEmpty()
        || !options.vnSoftSoilParameterFile.trimmed().isEmpty()
        || mode.compare(defaultMode, Qt::CaseInsensitive) != 0
        || differs(options.vnSoftSoilKsatOriginal, defaults.vnSoftSoilKsatOriginal)
        || differs(options.vnSoftSoilAlpha, defaults.vnSoftSoilAlpha)
        || differs(options.vnSoftSoilN, defaults.vnSoftSoilN)
        || differs(options.vnSoftSoilThetaSat, defaults.vnSoftSoilThetaSat)
        || differs(options.vnSoftSoilThetaRes, defaults.vnSoftSoilThetaRes);
}

bool IsHqSoftCustomizationRequested(const StarterScriptOptions &options)
{
    const StarterScriptOptions defaults;
    constexpr double kEpsilon = 1e-9;
    const auto differs = [](double lhs, double rhs) {
        return std::fabs(lhs - rhs) > kEpsilon;
    };
    const QString mode = options.vnSoftSoilParamMode.trimmed();
    const QString defaultMode = defaults.vnSoftSoilParamMode.trimmed();
    return !options.hqSoilPropsFile.trimmed().isEmpty()
        || !options.vnSoftSoilParameterFile.trimmed().isEmpty()
        || mode.compare(defaultMode, Qt::CaseInsensitive) != 0
        || options.hqSoftWellDepth > 0.0
        || options.hqSoftWellRadius > 0.0
        || options.hqSoftPondRadius > 0.0
        || options.hqSoftSurfaceElevation > 0.0
        || differs(options.vnSoftSoilKsatOriginal, defaults.vnSoftSoilKsatOriginal)
        || differs(options.vnSoftSoilAlpha, defaults.vnSoftSoilAlpha)
        || differs(options.vnSoftSoilN, defaults.vnSoftSoilN)
        || differs(options.vnSoftSoilThetaSat, defaults.vnSoftSoilThetaSat)
        || differs(options.vnSoftSoilThetaRes, defaults.vnSoftSoilThetaRes);
}

bool IsRBioswaleSoftCustomizationRequested(const StarterScriptOptions &options)
{
    const StarterScriptOptions defaults;
    constexpr double kEpsilon = 1e-9;
    const auto differs = [](double lhs, double rhs) {
        return std::fabs(lhs - rhs) > kEpsilon;
    };
    if (IsHqSoftCustomizationRequested(options)) {
        return true;
    }
    return !options.rSoilPropsFile.trimmed().isEmpty()
        || differs(options.rBioSwaleWidth, defaults.rBioSwaleWidth)
        || differs(options.rSystemWidth, defaults.rSystemWidth)
        || differs(options.rBioSwaleDepth, defaults.rBioSwaleDepth)
        || differs(options.rLength, defaults.rLength)
        || options.rLateralCells != defaults.rLateralCells
        || differs(options.rStreetWidth, defaults.rStreetWidth)
        || options.rStreetCells != defaults.rStreetCells
        || options.rVerticalLayers != defaults.rVerticalLayers
        || options.rEngineeredSoilNz != defaults.rEngineeredSoilNz
        || options.rNativeSoilNz != defaults.rNativeSoilNz
        || differs(options.rAnisoRatio, defaults.rAnisoRatio);
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

bool LooksLikeInternalSolverBinaryName(const QString &fileName)
{
    const QString base = QFileInfo(fileName).completeBaseName().trimmed();
    if (base.isEmpty()) {
        return false;
    }
    if (base.compare(QStringLiteral("OpenHydroQual"), Qt::CaseInsensitive) == 0) {
        return false;
    }
    return base.contains(QStringLiteral("solver"), Qt::CaseInsensitive)
        || base.contains(QStringLiteral("solve"), Qt::CaseInsensitive)
        || base.contains(QStringLiteral("internal"), Qt::CaseInsensitive)
        || base.contains(QStringLiteral("ohq"), Qt::CaseInsensitive)
        || base.contains(QStringLiteral("hydroqual"), Qt::CaseInsensitive);
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

    // Search near GUI roots for custom internal solver executables.
    // This catches non-standard names in local build trees.
    for (const QString &root : roots) {
        QDirIterator it(root,
                        QDir::Files | QDir::NoSymLinks,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            const QFileInfo fileInfo = it.fileInfo();
            if (!fileInfo.isExecutable()) {
                continue;
            }
            if (IsGuiExecutableOrAlias(fileInfo)) {
                continue;
            }
            if (LooksLikeCliOhqBinaryName(fileInfo.fileName())
                || LooksLikeInternalSolverBinaryName(fileInfo.fileName())) {
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
        if (!info.isExecutable()) {
            continue;
        }
        if (IsGuiExecutableOrAlias(info)) {
            continue;
        }
        if (LooksLikeCliOhqBinaryName(info.fileName())
            || LooksLikeInternalSolverBinaryName(info.fileName())) {
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
        if (arg.compare(QStringLiteral("script"), Qt::CaseInsensitive) == 0
            || arg.compare(QStringLiteral("%script%"), Qt::CaseInsensitive) == 0
            || arg.compare(QStringLiteral("$script"), Qt::CaseInsensitive) == 0) {
            arg = QStringLiteral("{script}");
        }
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
    QJsonParseError parseError;
    const QJsonDocument parsed = QJsonDocument::fromJson(configText.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || (!parsed.isObject() && !parsed.isArray())) {
        if (errorMessage) {
            *errorMessage = QObject::tr("GUI config template did not produce valid JSON: %1").arg(templatePath);
            if (parseError.error != QJsonParseError::NoError) {
                *errorMessage += QObject::tr(" (%1 at offset %2)").arg(parseError.errorString()).arg(parseError.offset);
            }
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

bool HasSimulationProgressOutput(const QString &runOutput)
{
    if (runOutput.trimmed().isEmpty()) {
        return false;
    }
    static const QStringList kProgressMarkers = {
        QStringLiteral("Creating model"),
        QStringLiteral("Model build complete"),
        QStringLiteral("Saving model files"),
        QStringLiteral("CalcAllInitialValues"),
        QStringLiteral("Solving daily period"),
        QStringLiteral("Running from time"),
        QStringLiteral("Simulation complete"),
        QStringLiteral("Simulation finished"),
        QStringLiteral("Writing VTP"),
        QStringLiteral("Writing output"),
        QStringLiteral("Saved output")
    };
    for (const QString &marker : kProgressMarkers) {
        if (runOutput.contains(marker, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

QString FirstSimulationProgressMarker(const QString &runOutput)
{
    if (runOutput.trimmed().isEmpty()) {
        return QString();
    }
    static const QStringList kProgressMarkers = {
        QStringLiteral("Creating model"),
        QStringLiteral("Model build complete"),
        QStringLiteral("Saving model files"),
        QStringLiteral("CalcAllInitialValues"),
        QStringLiteral("Solving daily period"),
        QStringLiteral("Running from time"),
        QStringLiteral("Simulation complete"),
        QStringLiteral("Simulation finished"),
        QStringLiteral("Writing VTP"),
        QStringLiteral("Writing output"),
        QStringLiteral("Saved output")
    };
    for (const QString &marker : kProgressMarkers) {
        if (runOutput.contains(marker, Qt::CaseInsensitive)) {
            return marker;
        }
    }
    return QString();
}

QStringList RuntimeHighlightMarkers()
{
    return {
        QStringLiteral("Creating model"),
        QStringLiteral("Model build complete"),
        QStringLiteral("Saving model files"),
        QStringLiteral("CalcAllInitialValues"),
        QStringLiteral("Solving daily period"),
        QStringLiteral("Running from time"),
        QStringLiteral("Simulation complete"),
        QStringLiteral("Simulation finished"),
        QStringLiteral("Writing VTP"),
        QStringLiteral("Writing output"),
        QStringLiteral("Saved output")
    };
}

QStringList NewlySeenRuntimeHighlights(const QString &previousOutput, const QString &newOutputChunk)
{
    QStringList hits;
    if (newOutputChunk.trimmed().isEmpty()) {
        return hits;
    }
    const QString combined = previousOutput + newOutputChunk;
    for (const QString &marker : RuntimeHighlightMarkers()) {
        const bool alreadySeen = previousOutput.contains(marker, Qt::CaseInsensitive);
        const bool seenNow = combined.contains(marker, Qt::CaseInsensitive);
        if (!alreadySeen && seenNow) {
            hits << marker;
        }
    }
    return hits;
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


bool ReadJsonObjectFile(const QString &path,
                        QJsonDocument *document,
                        QString *errorMessage)
{
    if (path.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("JSON path is empty.");
        }
        return false;
    }

    QFile inFile(path);
    if (!inFile.exists()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("JSON file does not exist: %1").arg(path);
        }
        return false;
    }
    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Cannot open JSON file: %1").arg(path);
        }
        return false;
    }

    const QByteArray bytes = inFile.readAll();
    if (bytes.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("JSON file is empty: %1").arg(path);
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument parsed = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Invalid JSON in %1 at offset %2: %3")
                                .arg(path)
                                .arg(parseError.offset)
                                .arg(parseError.errorString());
        }
        return false;
    }
    if (!parsed.isObject() && !parsed.isArray()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("JSON root must be an object or array: %1").arg(path);
        }
        return false;
    }

    if (document) {
        *document = parsed;
    }
    return true;
}

QString DescribeJsonFileForLog(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists()) {
        return QObject::tr("%1 (missing)").arg(path);
    }
    return QObject::tr("%1 (%2 bytes)").arg(path).arg(info.size());
}

void RemoveStaleRunnerGuiConfigs(const QString &workingDirectory,
                                 const QStringList &preservePaths,
                                 QStringList *removedPaths,
                                 QStringList *failedPaths)
{
    if (workingDirectory.trimmed().isEmpty()) {
        return;
    }

    const QDir dir(workingDirectory);
    const QStringList candidateNames = {
        QStringLiteral("runner_gui_config.auto.json"),
        QStringLiteral("runner_gui_config.generated.json")
    };

    QSet<QString> preserveCanonical;
    for (const QString &path : preservePaths) {
        const QString trimmed = path.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        QFileInfo info(trimmed);
        preserveCanonical.insert(info.canonicalFilePath().isEmpty() ? info.absoluteFilePath() : info.canonicalFilePath());
    }

    for (const QString &name : candidateNames) {
        const QString absPath = dir.filePath(name);
        QFileInfo info(absPath);
        if (!info.exists() || !info.isFile()) {
            continue;
        }

        const QString canonical = info.canonicalFilePath().isEmpty() ? info.absoluteFilePath() : info.canonicalFilePath();
        if (preserveCanonical.contains(canonical)) {
            continue;
        }

        QFile file(absPath);
        if (file.remove()) {
            if (removedPaths) {
                removedPaths->push_back(absPath);
            }
        } else if (failedPaths) {
            failedPaths->push_back(absPath);
        }
    }
}

void LogRuntimeJsonCandidates(const QString &workingDirectory,
                              const QStringList &extraPaths,
                              std::function<void(const QString&)> logFn)
{
    if (!logFn) {
        return;
    }

    QStringList paths;
    if (!workingDirectory.trimmed().isEmpty()) {
        const QDir dir(workingDirectory);
        const QFileInfoList infos = dir.entryInfoList(QStringList() << QStringLiteral("*.json"),
                                                      QDir::Files | QDir::NoSymLinks,
                                                      QDir::Name);
        for (const QFileInfo &info : infos) {
            paths << info.absoluteFilePath();
        }
    }
    for (const QString &path : extraPaths) {
        if (!path.trimmed().isEmpty()) {
            paths << path.trimmed();
        }
    }
    paths.removeDuplicates();

    if (paths.isEmpty()) {
        logFn(QObject::tr("Runtime JSON candidates: none"));
        return;
    }

    QStringList visiblePaths;
    for (const QString &path : paths) {
        const QFileInfo info(path);
        if (info.fileName().endsWith(QStringLiteral(".csv.json"), Qt::CaseInsensitive)) {
            continue;
        }
        visiblePaths << path;
    }
    if (visiblePaths.isEmpty()) {
        logFn(QObject::tr("Runtime JSON candidates: none"));
        return;
    }

    logFn(QObject::tr("Runtime JSON candidates:"));
    for (const QString &path : visiblePaths) {
        logFn(QObject::tr("  - %1").arg(DescribeJsonFileForLog(path)));
    }
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

bool IsPositiveDoubleText(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    bool ok = false;
    const double value = trimmed.toDouble(&ok);
    return ok && std::isfinite(value) && value > 0.0;
}

bool IsNonNegativeIntegerText(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    bool ok = false;
    const int value = trimmed.toInt(&ok);
    return ok && value >= 0;
}

bool IsPositiveIntegerText(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    bool ok = false;
    const int value = trimmed.toInt(&ok);
    return ok && value > 0;
}

bool WriteJsonFile(const QString &path, const QJsonObject &object)
{
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    out.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    return out.commit();
}

struct VnVtkBlockPoint
{
    QString name;
    double x = 0.0;
    double y = 0.0;
};

QString VnXmlEscape(QString text)
{
    text.replace('&', QStringLiteral("&amp;"));
    text.replace('<', QStringLiteral("&lt;"));
    text.replace('>', QStringLiteral("&gt;"));
    text.replace('"', QStringLiteral("&quot;"));
    text.replace('\'', QStringLiteral("&apos;"));
    return text;
}

QString VnScriptCommandValue(const QString &line, const QString &key)
{
    const QRegularExpression re(QStringLiteral("(?:^|[;,\\s])%1=([^,;\\r\\n]*)").arg(QRegularExpression::escape(key)),
                                QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = re.match(line);
    return match.hasMatch() ? match.captured(1).trimmed() : QString();
}

bool VnIsResultGridSoilBlockName(const QString &name)
{
    return name.startsWith(QStringLiteral("Soil-g"), Qt::CaseInsensitive)
        || name.startsWith(QStringLiteral("Soil-uw"), Qt::CaseInsensitive);
}

QVector<VnVtkBlockPoint> VnReadSoilBlockGeometryForVtk(const QString &scriptPath)
{
    QVector<VnVtkBlockPoint> blocks;
    QFile file(scriptPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return blocks;
    }

    QTextStream in(&file);
    QSet<QString> seenNames;
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (!line.startsWith(QStringLiteral("create block"), Qt::CaseInsensitive)
            || !line.contains(QStringLiteral("type=Soil"), Qt::CaseInsensitive)) {
            continue;
        }

        const QString name = VnScriptCommandValue(line, QStringLiteral("name"));
        if (!VnIsResultGridSoilBlockName(name) || seenNames.contains(name)) {
            continue;
        }

        bool okX = false;
        bool okY = false;
        const double x = VnScriptCommandValue(line, QStringLiteral("act_X")).toDouble(&okX);
        const double y = VnScriptCommandValue(line, QStringLiteral("act_Y")).toDouble(&okY);
        if (!okX || !okY || !std::isfinite(x) || !std::isfinite(y)) {
            continue;
        }

        VnVtkBlockPoint pt;
        pt.name = name;
        pt.x = x;
        pt.y = y;
        blocks.push_back(pt);
        seenNames.insert(name);
    }
    return blocks;
}

QStringList VnSplitDelimitedLine(const QString &line)
{
    if (line.contains(',')) {
        return line.split(',', Qt::KeepEmptyParts);
    }
    if (line.contains('\t')) {
        return line.split('\t', Qt::KeepEmptyParts);
    }
    return line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
}

int VnFindTimeColumn(const QStringList &header)
{
    for (int i = 0; i < header.size(); ++i) {
        const QString h = header.at(i).trimmed();
        if (h.compare(QStringLiteral("time"), Qt::CaseInsensitive) == 0
            || h.compare(QStringLiteral("t"), Qt::CaseInsensitive) == 0
            || h.contains(QStringLiteral("time"), Qt::CaseInsensitive)) {
            return i;
        }
    }
    return header.isEmpty() ? -1 : 0;
}

struct VnDelaunayTriangle
{
    int a = -1;
    int b = -1;
    int c = -1;
};

double VnTriangleArea2(const QVector<VnVtkBlockPoint> &pts, int a, int b, int c)
{
    return (pts.at(b).x - pts.at(a).x) * (pts.at(c).y - pts.at(a).y)
         - (pts.at(b).y - pts.at(a).y) * (pts.at(c).x - pts.at(a).x);
}

bool VnCircumcircleContains(const QVector<VnVtkBlockPoint> &pts,
                            const VnDelaunayTriangle &tri,
                            const VnVtkBlockPoint &p)
{
    const double ax = pts.at(tri.a).x - p.x;
    const double ay = pts.at(tri.a).y - p.y;
    const double bx = pts.at(tri.b).x - p.x;
    const double by = pts.at(tri.b).y - p.y;
    const double cx = pts.at(tri.c).x - p.x;
    const double cy = pts.at(tri.c).y - p.y;

    double det = (ax * ax + ay * ay) * (bx * cy - by * cx)
               - (bx * bx + by * by) * (ax * cy - ay * cx)
               + (cx * cx + cy * cy) * (ax * by - ay * bx);
    if (VnTriangleArea2(pts, tri.a, tri.b, tri.c) < 0.0) {
        det = -det;
    }
    return det > 1.0e-10;
}

QString VnEdgeKey(int a, int b)
{
    if (a > b) std::swap(a, b);
    return QString::number(a) + QLatin1Char('|') + QString::number(b);
}

QVector<VnDelaunayTriangle> VnBuildDelaunayTriangles(const QVector<VnVtkBlockPoint> &inputPoints)
{
    QVector<VnDelaunayTriangle> result;
    const int n = inputPoints.size();
    if (n < 3) {
        return result;
    }

    QVector<VnVtkBlockPoint> pts = inputPoints;
    double minX = pts.at(0).x;
    double maxX = pts.at(0).x;
    double minY = pts.at(0).y;
    double maxY = pts.at(0).y;
    for (const auto &pt : pts) {
        minX = std::min(minX, pt.x);
        maxX = std::max(maxX, pt.x);
        minY = std::min(minY, pt.y);
        maxY = std::max(maxY, pt.y);
    }

    const double dx = std::max(1.0, maxX - minX);
    const double dy = std::max(1.0, maxY - minY);
    const double delta = std::max(dx, dy) * 32.0;
    const double cx = 0.5 * (minX + maxX);
    const double cy = 0.5 * (minY + maxY);

    VnVtkBlockPoint s1; s1.x = cx - 2.0 * delta; s1.y = cy - delta;
    VnVtkBlockPoint s2; s2.x = cx;               s2.y = cy + 2.0 * delta;
    VnVtkBlockPoint s3; s3.x = cx + 2.0 * delta; s3.y = cy - delta;
    const int si1 = pts.size(); pts.push_back(s1);
    const int si2 = pts.size(); pts.push_back(s2);
    const int si3 = pts.size(); pts.push_back(s3);

    QVector<VnDelaunayTriangle> triangles;
    triangles.push_back({si1, si2, si3});

    for (int pi = 0; pi < n; ++pi) {
        QVector<VnDelaunayTriangle> kept;
        QMap<QString, QPair<int, int>> edgeByKey;
        QMap<QString, int> edgeCount;

        for (const auto &tri : triangles) {
            if (VnCircumcircleContains(pts, tri, pts.at(pi))) {
                const QPair<int, int> edges[3] = {
                    qMakePair(tri.a, tri.b),
                    qMakePair(tri.b, tri.c),
                    qMakePair(tri.c, tri.a)
                };
                for (const auto &edge : edges) {
                    const QString key = VnEdgeKey(edge.first, edge.second);
                    edgeByKey.insert(key, edge);
                    edgeCount.insert(key, edgeCount.value(key, 0) + 1);
                }
            } else {
                kept.push_back(tri);
            }
        }

        for (auto it = edgeCount.constBegin(); it != edgeCount.constEnd(); ++it) {
            if (it.value() != 1) {
                continue;
            }
            const QPair<int, int> edge = edgeByKey.value(it.key());
            VnDelaunayTriangle tri{edge.first, edge.second, pi};
            if (std::abs(VnTriangleArea2(pts, tri.a, tri.b, tri.c)) < 1.0e-12) {
                continue;
            }
            if (VnTriangleArea2(pts, tri.a, tri.b, tri.c) < 0.0) {
                std::swap(tri.a, tri.b);
            }
            kept.push_back(tri);
        }
        triangles = kept;
    }

    QSet<QString> unique;
    for (auto tri : triangles) {
        if (tri.a >= n || tri.b >= n || tri.c >= n) {
            continue;
        }
        if (std::abs(VnTriangleArea2(inputPoints, tri.a, tri.b, tri.c)) < 1.0e-12) {
            continue;
        }
        if (VnTriangleArea2(inputPoints, tri.a, tri.b, tri.c) < 0.0) {
            std::swap(tri.a, tri.b);
        }
        QVector<int> sorted{tri.a, tri.b, tri.c};
        std::sort(sorted.begin(), sorted.end());
        const QString key = QString::number(sorted.at(0)) + QLatin1Char('|')
                          + QString::number(sorted.at(1)) + QLatin1Char('|')
                          + QString::number(sorted.at(2));
        if (unique.contains(key)) {
            continue;
        }
        unique.insert(key);
        result.push_back(tri);
    }
    return result;
}

bool VnWriteDelaunayVtp(const QString &path,
                        const QString &scalarName,
                        const QVector<VnVtkBlockPoint> &points,
                        const QVector<double> &values,
                        QString *errorMessage)
{
    if (points.isEmpty() || points.size() != values.size()) {
        if (errorMessage) *errorMessage = QObject::tr("Invalid VTP point/value array sizes.");
        return false;
    }

    const QVector<VnDelaunayTriangle> triangles = VnBuildDelaunayTriangles(points);

    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) *errorMessage = QObject::tr("Unable to open VTP output: %1").arg(path);
        return false;
    }

    QTextStream ts(&out);
    ts.setRealNumberPrecision(15);
    const int n = points.size();
    const int nVerts = triangles.isEmpty() ? n : 0;
    const int nPolys = triangles.size();

    ts << "<?xml version=\"1.0\"?>\n";
    ts << "<VTKFile type=\"PolyData\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
    ts << "  <PolyData>\n";
    ts << "    <Piece NumberOfPoints=\"" << n
       << "\" NumberOfVerts=\"" << nVerts
       << "\" NumberOfLines=\"0\" NumberOfStrips=\"0\" NumberOfPolys=\"" << nPolys << "\">\n";
    ts << "      <PointData Scalars=\"" << VnXmlEscape(scalarName) << "\">\n";
    ts << "        <DataArray type=\"Float32\" Name=\"" << VnXmlEscape(scalarName) << "\" format=\"ascii\">\n          ";
    for (double value : values) {
        ts << static_cast<float>(std::isfinite(value) ? value : 0.0) << ' ';
    }
    ts << "\n        </DataArray>\n";
    ts << "      </PointData>\n";
    ts << "      <Points>\n";
    ts << "        <DataArray type=\"Float32\" Name=\"Points\" NumberOfComponents=\"3\" format=\"ascii\">\n          ";
    for (const auto &pt : points) {
        ts << static_cast<float>(pt.x) << ' ' << static_cast<float>(pt.y) << " 0 ";
    }
    ts << "\n        </DataArray>\n";
    ts << "      </Points>\n";

    if (!triangles.isEmpty()) {
        ts << "      <Polys>\n";
        ts << "        <DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">\n          ";
        for (const auto &tri : triangles) {
            ts << tri.a << ' ' << tri.b << ' ' << tri.c << ' ';
        }
        ts << "\n        </DataArray>\n";
        ts << "        <DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">\n          ";
        int offset = 0;
        for (int i = 0; i < triangles.size(); ++i) {
            offset += 3;
            ts << offset << ' ';
        }
        ts << "\n        </DataArray>\n";
        ts << "      </Polys>\n";
    } else {
        ts << "      <Verts>\n";
        ts << "        <DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">\n          ";
        for (int i = 0; i < n; ++i) ts << i << ' ';
        ts << "\n        </DataArray>\n";
        ts << "        <DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">\n          ";
        for (int i = 0; i < n; ++i) ts << (i + 1) << ' ';
        ts << "\n        </DataArray>\n";
        ts << "      </Verts>\n";
    }

    ts << "    </Piece>\n";
    ts << "  </PolyData>\n";
    ts << "</VTKFile>\n";

    if (!out.commit()) {
        if (errorMessage) *errorMessage = QObject::tr("Unable to finalize VTP output: %1").arg(path);
        return false;
    }
    return true;
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
      inflowUseButton(new QPushButton(tr("Use"), this)),
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
      hqSoftRadialCellsEdit(new QLineEdit(this)),
      hqSoftShallowLayersEdit(new QLineEdit(this)),
      hqSoftWellDepthEdit(new QLineEdit(this)),
      hqSoftWellRadiusEdit(new QLineEdit(this)),
      hqSoftPondRadiusEdit(new QLineEdit(this)),
      hqSoftSurfaceElevationEdit(new QLineEdit(this)),
      hqSoilPropsFileEdit(new QLineEdit(this)),
      rBioSwaleWidthEdit(new QLineEdit(this)),
      rSystemWidthEdit(new QLineEdit(this)),
      rBioSwaleDepthEdit(new QLineEdit(this)),
      rSoilPropsFileEdit(new QLineEdit(this)),
      rVerticalLayersEdit(new QLineEdit(this)),
      rEngineeredSoilNzEdit(new QLineEdit(this)),
      rNativeSoilNzEdit(new QLineEdit(this)),
      rLateralCellsEdit(new QLineEdit(this)),
      rLengthEdit(new QLineEdit(this)),
      rStreetWidthEdit(new QLineEdit(this)),
      rStreetCellsEdit(new QLineEdit(this)),
      rAnisoRatioEdit(new QLineEdit(this)),
      jmNativeHorizontalCellsEdit(new QLineEdit(this)),
      jmNativeVerticalLayersEdit(new QLineEdit(this)),
      vnInitThetaModeCombo(new QComboBox(this)),
      vnFieldPointsEdit(new QLineEdit(this)),
      vnFieldSeedEdit(new QLineEdit(this)),
      vnFieldDxEdit(new QLineEdit(this)),
      vnFieldPdfModeCombo(new QComboBox(this)),
      vnSoilProfileExportEdit(new QLineEdit(this)),
      vnDepthSliceExportEdit(new QLineEdit(this)),
      vnErtSnapshotExportEdit(new QLineEdit(this)),
      vtkInventoryExportEdit(new QLineEdit(this)),
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
      exportVnSoilProfileButton(new QPushButton(tr("Export VN soil profile"), this)),
      exportVnDepthSliceButton(new QPushButton(tr("Export VN depth slice"), this)),
      exportVnMetadataButton(new QPushButton(tr("Export VN metadata JSON"), this)),
      exportVnErtSnapshotButton(new QPushButton(tr("Export ERT-ready CSV"), this)),
      exportVtkInventoryButton(new QPushButton(tr("Export VTK inventory"), this)),
      exportVnVtkSnapshotsButton(new QPushButton(tr("Export VN VTK"), this)),
      saveVnGeneratedFieldButton(new QPushButton(tr("Save field file"), this)),
      useVnGeneratedFieldButton(new QPushButton(tr("Use field file"), this)),
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

    auto addToggleFileRow = [](QVBoxLayout *targetLayout, const QString &labelText, QLineEdit *edit,
                               QPushButton *useButton, const QString &buttonText, auto slot) -> QWidget* {
        auto *container = new QWidget();
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(labelText));
        useButton->setCheckable(true);
        useButton->setChecked(true);
        useButton->setText(QObject::tr("Use"));
        useButton->setToolTip(QObject::tr("Toggle this file assignment on/off without clearing the selected path."));
        row->addWidget(useButton);
        row->addWidget(edit, 1);
        auto *btn = new QPushButton(buttonText);
        QObject::connect(btn, &QPushButton::clicked, slot);
        row->addWidget(btn);
        QObject::connect(useButton, &QPushButton::toggled, edit, [edit, useButton, btn](bool checked) {
            useButton->setText(checked ? QObject::tr("Use") : QObject::tr("No file"));
            edit->setEnabled(checked);
            btn->setEnabled(checked);
        });
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
    exeArgsEdit->setPlaceholderText(tr("Default: {script} (or e.g. --script {script} --run)"));
    exeArgsEdit->setToolTip(tr("Command-line arguments passed to the executable. Use {script} placeholder for the selected .ohq path. "
                               "Default is {script}; for OpenHydroQual GUI this is normalized to {script} --run. "
                               "If left empty: OHQ CLI gets positional script; OpenHydroQual GUI gets <script> --run; "
                               "custom executables get no implicit args."));
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
    workingDirEdit->setPlaceholderText(tr("Suggested: <repo>/Models"));
    addFileRow(layout, tr("Artifacts directory"), artifactsDirEdit, tr("Browse"), [this]() { chooseArtifactsDirectory(); });
    artifactsDirEdit->setPlaceholderText(tr("Suggested: <working_dir>/artifacts"));
    templateDirRowWidget = nullptr;
    templateDirEdit->setPlaceholderText(tr("Auto-detected from OpenHydroQual roots"));
    generatedScriptRowWidget = addFileRow(layout, tr("Generated script path"), generatedScriptEdit, tr("Browse"), [this]() { chooseGeneratedScriptPath(); });
    generatedScriptEdit->setPlaceholderText(tr("Suggested: <working_dir>/starter_generated.ohq"));
    inflowRowWidget = addToggleFileRow(layout, tr("Inflow file"), inflowFileEdit, inflowUseButton, tr("Browse"), [this]() { chooseInflowFile(); });
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
    outputSeriesFileEdit->setPlaceholderText(tr("Suggested: <working_dir>/OHQ_output.txt"));
    observationFileRowWidget = addFileRow(layout, tr("Observation file (optional)"), observationFileEdit, tr("Browse"), [this]() { chooseObservationFile(); });
    observationFileEdit->setPlaceholderText(tr("Suggested: <repo>/observation.csv"));
    depthProfileRowWidget = addFileRow(layout, tr("Depth profile file (optional)"), depthProfileFileEdit, tr("Browse"), [this]() { chooseDepthProfileFile(); });
    depthProfileFileEdit->setPlaceholderText(tr("Suggested: <repo>/depth_profile.csv"));
    vnBaseRowWidget = nullptr;
    vnSoilRowWidget = addFileRow(layout, tr("Soil layers snippet (optional)"), vnSoilLayersFileEdit, tr("Browse"), [this]() { chooseVnSoilLayersFile(); });
    vnSoilLayersFileEdit->setPlaceholderText(tr("Optional: .txt/.ohq/.csv with soil-layer commands"));
    vnMoistureRowWidget = addFileRow(layout, tr("Moisture layers snippet (optional)"), vnMoistureLayersFileEdit, tr("Browse"), [this]() { chooseVnMoistureLayersFile(); });
    vnMoistureLayersFileEdit->setPlaceholderText(tr("Optional: .txt/.ohq/.csv with moisture-layer commands"));
    vnBuildModeCombo->addItem(tr("SoftReference"), QStringLiteral("SoftReference"));
    vnBuildModeCombo->addItem(tr("FullReference"), QStringLiteral("FullReference"));
    vnBuildModeCombo->addItem(tr("LoadFromOhq"), QStringLiteral("LoadFromOhq"));
    vnBuildModeCombo->setToolTip(tr("SoftReference is the editable VN mode and is intended to reproduce FullReference exactly when the defaults remain unchanged. FullReference uses the embedded canonical VN reference. LoadFromOhq uses the selected VN base script."));
    vnBuildModeRowWidget = addTextRow(layout, tr("Build mode"), vnBuildModeCombo);
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
    vnSoftSoilParamModeCombo->addItem(tr("VN reference defaults"), QStringLiteral("VnReferenceDefaults"));
    vnSoftSoilParamModeCombo->addItem(tr("Manual"), QStringLiteral("Manual"));
    vnSoftSoilParamModeCombo->addItem(tr("ModelCreator defaults"), QStringLiteral("ModelCreatorDefaults"));
    vnSoftSoilParamModeCombo->addItem(tr("File (depth profile)"), QStringLiteral("File"));
    vnSoftSoilParamModeCombo->setToolTip(tr("Soil-parameter source for VN SoftReference. HQ and R have their own soil-file rows below; manual values can still be reused by builders where supported."));
    vnSoftSoilParameterFileEdit->setPlaceholderText(tr("Optional VN CSV depth profile for Ksat/alpha/n/theta_s/theta_r"));
    setupCompactNumericEdit(rBioSwaleWidthEdit, tr("0.6096"));
    setupCompactNumericEdit(rSystemWidthEdit, tr("3"));
    setupCompactNumericEdit(rBioSwaleDepthEdit, tr("0.9144"));
    setupCompactNumericEdit(rLateralCellsEdit, tr("6"));
    setupCompactNumericEdit(rLengthEdit, tr("8"));
    setupCompactNumericEdit(rStreetWidthEdit, tr("5"));
    setupCompactNumericEdit(rStreetCellsEdit, tr("10"));
    setupCompactNumericEdit(rVerticalLayersEdit, tr("Auto"));
    setupCompactNumericEdit(rEngineeredSoilNzEdit, tr("Auto"));
    setupCompactNumericEdit(rNativeSoilNzEdit, tr("Auto"));
    setupCompactNumericEdit(rAnisoRatioEdit, tr("5"));
    setupCompactNumericEdit(jmNativeHorizontalCellsEdit, tr("4"));
    setupCompactNumericEdit(jmNativeVerticalLayersEdit, tr("3"));
    jmNativeHorizontalCellsEdit->setToolTip(tr("JM centered native-soil cells horizontally (nx). Use 1 for one centered block across the facility."));
    jmNativeVerticalLayersEdit->setToolTip(tr("JM centered native-soil layers vertically (nz). Use 1 for one native-soil row."));
    hqSoilPropsFileEdit->setPlaceholderText(tr("Optional HQ soil layer file (*.txt, *.csv)"));
    hqSoilPropsFileEdit->setToolTip(tr("Optional HQ/DryWell soil layer table. If provided, HQ SoftReference uses these per-layer soil parameters while keeping HQ geometry controls."));
    rSoilPropsFileEdit->setPlaceholderText(tr("Optional R/Rosemead soil layer file (*.txt, *.csv)"));
    rVerticalLayersEdit->setPlaceholderText(tr("Auto"));
    rVerticalLayersEdit->setToolTip(tr("Legacy total R/Rosemead nz. Leave Auto when using separate engineered/native nz fields."));
    rEngineeredSoilNzEdit->setPlaceholderText(tr("Auto"));
    rEngineeredSoilNzEdit->setToolTip(tr("Engineered/top soil nz. If set with native soil nz, total nz = engineered nz + native nz."));
    rNativeSoilNzEdit->setPlaceholderText(tr("Auto"));
    rNativeSoilNzEdit->setToolTip(tr("Native/bottom soil nz. The last native row is connected to fixed-head GW."));
    {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("Soil-g")));
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
        row->addWidget(new QLabel(tr("Soil-uw")));
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
        row->addWidget(new QLabel(tr("Radii [m]")));
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
        row->addWidget(new QLabel(tr("Depths [m]")));
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
        row->addWidget(new QLabel(tr("z")));
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
        row->addWidget(new QLabel(tr("VN soil props")));
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
        auto *vnRefTableButton = new QPushButton(tr("Check"), container);
        connect(vnRefTableButton, &QPushButton::clicked, this, &ModelCreatorWindow::showVnSoilPropsTable);
        row->addWidget(vnRefTableButton);
        row->addStretch(1);
        layout->addWidget(container);
        vnSoftSoilParamsRowWidget = container;
    }
    {
        setupCompactNumericEdit(hqSoftRadialCellsEdit, tr("10"));
        setupCompactNumericEdit(hqSoftShallowLayersEdit, tr("34"));
        setupCompactNumericEdit(hqSoftWellDepthEdit, tr("20"));
        setupCompactNumericEdit(hqSoftWellRadiusEdit, tr("0.381"));
        setupCompactNumericEdit(hqSoftPondRadiusEdit, tr("6"));
        setupCompactNumericEdit(hqSoftSurfaceElevationEdit, tr("140"));
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("HQ soft geometry")));
        row->addWidget(new QLabel(tr("nr")));
        row->addWidget(hqSoftRadialCellsEdit);
        row->addWidget(new QLabel(tr("layers")));
        row->addWidget(hqSoftShallowLayersEdit);
        row->addWidget(new QLabel(tr("well_depth[m]")));
        row->addWidget(hqSoftWellDepthEdit);
        row->addWidget(new QLabel(tr("well_r[m]")));
        row->addWidget(hqSoftWellRadiusEdit);
        row->addWidget(new QLabel(tr("pond_r[m]")));
        row->addWidget(hqSoftPondRadiusEdit);
        row->addWidget(new QLabel(tr("surface_z[m]")));
        row->addWidget(hqSoftSurfaceElevationEdit);
        row->addStretch(1);
        layout->addWidget(container);
        hqSoftGeometryRowWidget = container;
    }
    {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("HQ soil file")));
        row->addWidget(hqSoilPropsFileEdit, 1);
        auto *browseBtn = new QPushButton(tr("Browse"), container);
        connect(browseBtn, &QPushButton::clicked, this, &ModelCreatorWindow::chooseHqSoilPropsFile);
        row->addWidget(browseBtn);
        auto *checkBtn = new QPushButton(tr("Check"), container);
        connect(checkBtn, &QPushButton::clicked, this, &ModelCreatorWindow::showHqSoilPropsTable);
        row->addWidget(checkBtn);
        row->addStretch(1);
        layout->addWidget(container);
        hqSoilControlsRowWidget = container;
    }
    {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("R soil blocks")));
        row->addWidget(new QLabel(tr("bioswale_w[m]")));
        row->addWidget(rBioSwaleWidthEdit);
        row->addWidget(new QLabel(tr("ext_left[m]")));
        row->addWidget(rSystemWidthEdit);
        row->addWidget(new QLabel(tr("depth[m]")));
        row->addWidget(rBioSwaleDepthEdit);
        row->addWidget(new QLabel(tr("length[m]")));
        row->addWidget(rLengthEdit);
        row->addStretch(1);
        layout->addWidget(container);
        rSoilGeometryRowWidget = container;
    }
    {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("R domain")));
        row->addWidget(new QLabel(tr("left_cells")));
        row->addWidget(rLateralCellsEdit);
        row->addWidget(new QLabel(tr("street_w[m]")));
        row->addWidget(rStreetWidthEdit);
        row->addWidget(new QLabel(tr("street_cells")));
        row->addWidget(rStreetCellsEdit);
        row->addWidget(new QLabel(tr("total_nz")));
        row->addWidget(rVerticalLayersEdit);
        row->addWidget(new QLabel(tr("eng_nz")));
        row->addWidget(rEngineeredSoilNzEdit);
        row->addWidget(new QLabel(tr("native_nz")));
        row->addWidget(rNativeSoilNzEdit);
        row->addWidget(new QLabel(tr("aniso")));
        row->addWidget(rAnisoRatioEdit);
        row->addStretch(1);
        layout->addWidget(container);
        rSoilDomainRowWidget = container;
    }
    {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("JM centered native soil")));
        row->addWidget(new QLabel(tr("horizontal nx")));
        row->addWidget(jmNativeHorizontalCellsEdit);
        row->addWidget(new QLabel(tr("vertical nz")));
        row->addWidget(jmNativeVerticalLayersEdit);
        row->addWidget(new QLabel(tr("Examples: 4x1 = one row; 1x1 = one block")));
        row->addStretch(1);
        layout->addWidget(container);
        jmNativeDomainRowWidget = container;
    }
    {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("R soil file")));
        row->addWidget(rSoilPropsFileEdit, 1);
        auto *browseBtn = new QPushButton(tr("Browse"), container);
        connect(browseBtn, &QPushButton::clicked, this, &ModelCreatorWindow::chooseRBioswaleSoilPropsFile);
        row->addWidget(browseBtn);
        auto *checkBtn = new QPushButton(tr("Check"), container);
        connect(checkBtn, &QPushButton::clicked, this, &ModelCreatorWindow::showRBioswaleSoilPropsTable);
        row->addWidget(checkBtn);
        row->addStretch(1);
        layout->addWidget(container);
        rSoilControlsRowWidget = container;
    }
    vnInitThetaModeCombo->addItem(tr("Default"), QStringLiteral("Default"));
    vnInitThetaModeCombo->addItem(tr("ERT-3 only"), QStringLiteral("ERT3_Only"));
    vnInitThetaModeCombo->addItem(tr("ERT-5 only"), QStringLiteral("ERT5_Only"));
    vnInitThetaModeCombo->addItem(tr("ERT IDW_R"), QStringLiteral("ERT_IDW_R"));
    vnInitThetaModeCombo->addItem(tr("ERT R_Avg"), QStringLiteral("ERT_R_Avg"));
    vnInitThetaModeCombo->setToolTip(tr("VN-only metadata/control for the initial-theta strategy used by the separate VN ModelCreator pipeline."));
    vnInitThetaRowWidget = addTextRow(layout, tr("VN init-theta mode"), vnInitThetaModeCombo);
    setupCompactNumericEdit(vnFieldPointsEdit, tr("200"));
    setupCompactNumericEdit(vnFieldSeedEdit, tr("42"));
    setupCompactNumericEdit(vnFieldDxEdit, tr("0.5"));
    vnFieldPdfModeCombo->addItem(tr("Parametric"), QStringLiteral("parametric"));
    vnFieldPdfModeCombo->addItem(tr("Nonparametric"), QStringLiteral("nonparametric"));
    vnFieldPdfModeCombo->setToolTip(tr("VN-only metadata/control for the separate FieldGenerator preprocessing workflow."));
    {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("VN field generator")));
        row->addWidget(new QLabel(tr("points")));
        row->addWidget(vnFieldPointsEdit);
        row->addWidget(new QLabel(tr("seed")));
        row->addWidget(vnFieldSeedEdit);
        row->addWidget(new QLabel(tr("dx[m]")));
        row->addWidget(vnFieldDxEdit);
        row->addWidget(new QLabel(tr("pdf")));
        row->addWidget(vnFieldPdfModeCombo);
        row->addStretch(1);
        layout->addWidget(container);
        vnFieldGeneratorRowWidget = container;
    }
    vnSoilProfileExportEdit->setPlaceholderText(tr("Suggested: <working_dir>/vn_soil_profile.csv"));
    {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("VN soil tool")));
        row->addWidget(vnSoilProfileExportEdit, 1);
        auto *browseBtn = new QPushButton(tr("Browse"), container);
        connect(browseBtn, &QPushButton::clicked, this, [this]() {
            const QString suggested = QDir(workingDirEdit->text().trimmed()).filePath(QStringLiteral("vn_soil_profile.csv"));
            const QString fileName = QFileDialog::getSaveFileName(this, tr("Save VN soil profile CSV"), suggested, tr("CSV files (*.csv);;All files (*.*)"));
            if (!fileName.isEmpty()) {
                vnSoilProfileExportEdit->setText(fileName);
                saveSettings();
            }
        });
        row->addWidget(browseBtn);
        row->addWidget(exportVnSoilProfileButton);
        row->addWidget(saveVnGeneratedFieldButton);
        row->addWidget(useVnGeneratedFieldButton);
        row->addStretch(1);
        layout->addWidget(container);
        vnSoilToolRowWidget = container;
    }
    vnDepthSliceExportEdit->setPlaceholderText(tr("Suggested: <working_dir>/vn_depth_slice.csv"));
    {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("Output tool")));
        row->addWidget(vnDepthSliceExportEdit, 1);
        auto *browseBtn = new QPushButton(tr("Browse"), container);
        connect(browseBtn, &QPushButton::clicked, this, [this]() {
            const QString suggested = QDir(workingDirEdit->text().trimmed()).filePath(QStringLiteral("vn_depth_slice.csv"));
            const QString fileName = QFileDialog::getSaveFileName(this, tr("Save depth slice CSV"), suggested, tr("CSV files (*.csv);;All files (*.*)"));
            if (!fileName.isEmpty()) {
                vnDepthSliceExportEdit->setText(fileName);
                saveSettings();
            }
        });
        row->addWidget(browseBtn);
        row->addWidget(exportVnDepthSliceButton);
        row->addWidget(exportVnMetadataButton);
        row->addStretch(1);
        layout->addWidget(container);
        vnOutputToolRowWidget = container;
    }
    vnErtSnapshotExportEdit->setPlaceholderText(tr("Suggested: <working_dir>/vn_ert_snapshot.csv"));
    {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("ERT tool")));
        row->addWidget(vnErtSnapshotExportEdit, 1);
        auto *browseBtn = new QPushButton(tr("Browse"), container);
        connect(browseBtn, &QPushButton::clicked, this, [this]() {
            const QString suggested = QDir(workingDirEdit->text().trimmed()).filePath(QStringLiteral("vn_ert_snapshot.csv"));
            const QString fileName = QFileDialog::getSaveFileName(this, tr("Save ERT-ready CSV"), suggested, tr("CSV files (*.csv);;All files (*.*)"));
            if (!fileName.isEmpty()) {
                vnErtSnapshotExportEdit->setText(fileName);
                saveSettings();
            }
        });
        row->addWidget(browseBtn);
        row->addWidget(exportVnErtSnapshotButton);
        row->addStretch(1);
        layout->addWidget(container);
    }
    vtkInventoryExportEdit->setPlaceholderText(tr("Suggested: <working_dir>/vtk_inventory.csv"));
    {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(tr("VTK tool")));
        row->addWidget(vtkInventoryExportEdit, 1);
        auto *browseBtn = new QPushButton(tr("Browse"), container);
        connect(browseBtn, &QPushButton::clicked, this, [this]() {
            const QString suggested = QDir(workingDirEdit->text().trimmed()).filePath(QStringLiteral("vtk_inventory.csv"));
            const QString fileName = QFileDialog::getSaveFileName(this, tr("Save VTK inventory CSV"), suggested, tr("CSV files (*.csv);;All files (*.*)"));
            if (!fileName.isEmpty()) {
                vtkInventoryExportEdit->setText(fileName);
                saveSettings();
            }
        });
        row->addWidget(browseBtn);
        row->addWidget(exportVtkInventoryButton);
        row->addWidget(exportVnVtkSnapshotsButton);
        row->addStretch(1);
        layout->addWidget(container);
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
    connect(exportVnSoilProfileButton, &QPushButton::clicked, this, &ModelCreatorWindow::exportVnSoilProfileCsv);
    connect(exportVnDepthSliceButton, &QPushButton::clicked, this, &ModelCreatorWindow::exportVnDepthSliceCsv);
    connect(exportVnMetadataButton, &QPushButton::clicked, this, &ModelCreatorWindow::exportVnMetadataJson);
    connect(exportVnErtSnapshotButton, &QPushButton::clicked, this, &ModelCreatorWindow::exportVnErtSnapshotCsv);
    connect(exportVtkInventoryButton, &QPushButton::clicked, this, &ModelCreatorWindow::exportVtkInventoryCsv);
    connect(exportVnVtkSnapshotsButton, &QPushButton::clicked, this, &ModelCreatorWindow::exportVnVtkSnapshots);
    connect(saveVnGeneratedFieldButton, &QPushButton::clicked, this, &ModelCreatorWindow::saveVnGeneratedFieldFile);
    connect(useVnGeneratedFieldButton, &QPushButton::clicked, this, &ModelCreatorWindow::useVnGeneratedFieldFile);
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
    connect(workingDirEdit, &QLineEdit::textChanged, this, [this](const QString &) { syncVnToolDefaultPaths(); });
    connect(depthProfileFileEdit, &QLineEdit::editingFinished, this, &ModelCreatorWindow::refreshPlots);
    connect(modelTypeCombo, &QComboBox::currentTextChanged, this, &ModelCreatorWindow::syncEnrichmentPresetForModel);
    connect(modelTypeCombo, &QComboBox::currentTextChanged, this, [this]() { saveSettings(); });
    connect(modelTypeCombo, &QComboBox::currentTextChanged, this, [this]() { updateFieldVisibilityForContext(); });
    connect(modelTypeCombo, &QComboBox::currentTextChanged, this, [this](const QString &newModelType) {
        const QString previousModelType = lastSelectedModelType.trimmed();
        bool executableUpdated = false;
        bool argsUpdated = false;
        bool inflowUpdated = false;
        bool simulationWindowUpdated = false;
        const QString suggestedExecutable = DetectExecutablePathFromContext(FindRepoRoot(),
                                                                            workingDirEdit->text().trimmed(),
                                                                            scriptPathEdit->text().trimmed(),
                                                                            templateDirEdit->text().trimmed(),
                                                                            exePathEdit->text().trimmed());
        executableUpdated = ApplySuggestedFieldValue(exePathEdit, suggestedExecutable);
        argsUpdated = ApplySuggestedFieldValue(exeArgsEdit, QStringLiteral("{script}"));
        const QString currentInflow = inflowFileEdit->text().trimmed();
        if (currentInflow.isEmpty() || inflowAutoSuggested || IsKnownReferenceInflowForOtherModelUi(currentInflow, newModelType)) {
            const QString suggested = DetectSuggestedInflowFile(newModelType, templateDirEdit->text().trimmed());
            if (!suggested.isEmpty()) {
                inflowFileEdit->setText(suggested);
                inflowAutoSuggested = true;
                suggestSimulationWindowFromInflow(suggested, true);
                simulationWindowUpdated = true;
                inflowUpdated = true;
                appendLog(stamp(tr("Updated inflow default for %1: %2").arg(newModelType, suggested)));
            }
        }
        if (!previousModelType.isEmpty() && previousModelType.compare(newModelType, Qt::CaseInsensitive) != 0) {
            QStringList updatedFields;
            if (executableUpdated) updatedFields << tr("executable");
            if (argsUpdated) updatedFields << tr("args");
            if (inflowUpdated) updatedFields << tr("inflow");
            if (simulationWindowUpdated) updatedFields << tr("simulation window");
            appendLog(stamp(tr("Structure switched: %1 → %2. Auto-updated: %3.")
                            .arg(previousModelType,
                                 newModelType,
                                 updatedFields.isEmpty() ? tr("none") : updatedFields.join(tr(", ")))));
        }
        lastSelectedModelType = newModelType;
    });
    connect(workflowModeCombo, &QComboBox::currentTextChanged, this, [this]() { saveSettings(); updateFieldVisibilityForContext(); });
    connect(enrichmentPresetCombo, &QComboBox::currentTextChanged, this, [this]() { saveSettings(); });
    connect(enrichmentPresetCombo, &QComboBox::currentTextChanged, this, [this]() { updateFieldVisibilityForContext(); });
    connect(vnBuildModeCombo, &QComboBox::currentTextChanged, this, [this]() { updateFieldVisibilityForContext(); saveSettings(); });
    connect(showOptionalFieldsCheck, &QCheckBox::toggled, this, [this]() { updateFieldVisibilityForContext(); saveSettings(); });
    connect(allowGuiExecutionCheck, &QCheckBox::toggled, this, [this]() { saveSettings(); });

    const auto saveOnEdit = [this](QLineEdit *edit) {
        connect(edit, &QLineEdit::textEdited, this, [edit]() { SetAutoSuggestedField(edit, false); });
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
    if (inflowUseButton) {
        connect(inflowUseButton, &QPushButton::toggled, this, [this](bool checked) {
            inflowUseButton->setText(checked ? tr("Use") : tr("No file"));
            saveSettings();
        });
    }
    connect(inflowFileEdit, &QLineEdit::textEdited, this, [this]() { inflowAutoSuggested = false; });
    saveOnEdit(simulationStartEdit);
    saveOnEdit(simulationEndEdit);
    connect(simulationStartEdit, &QLineEdit::textChanged, this, [this]() { UpdateSimulationDateTooltip(simulationStartEdit); });
    connect(simulationEndEdit, &QLineEdit::textChanged, this, [this]() { UpdateSimulationDateTooltip(simulationEndEdit); });
    connect(simulationStartEdit, &QLineEdit::textEdited, this, [this]() { simulationWindowAutoSuggested = false; });
    connect(simulationEndEdit, &QLineEdit::textEdited, this, [this]() { simulationWindowAutoSuggested = false; });
    UpdateSimulationDateTooltip(simulationStartEdit);
    UpdateSimulationDateTooltip(simulationEndEdit);
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
    saveOnEdit(hqSoftRadialCellsEdit);
    saveOnEdit(hqSoftShallowLayersEdit);
    saveOnEdit(hqSoftWellDepthEdit);
    saveOnEdit(hqSoftWellRadiusEdit);
    saveOnEdit(hqSoftPondRadiusEdit);
    saveOnEdit(hqSoftSurfaceElevationEdit);
    saveOnEdit(hqSoilPropsFileEdit);
    saveOnEdit(rBioSwaleWidthEdit);
    saveOnEdit(rSystemWidthEdit);
    saveOnEdit(rBioSwaleDepthEdit);
    saveOnEdit(rSoilPropsFileEdit);
    saveOnEdit(rLateralCellsEdit);
    saveOnEdit(rLengthEdit);
    saveOnEdit(rStreetWidthEdit);
    saveOnEdit(rStreetCellsEdit);
    saveOnEdit(rVerticalLayersEdit);
    saveOnEdit(rEngineeredSoilNzEdit);
    saveOnEdit(rNativeSoilNzEdit);
    saveOnEdit(rAnisoRatioEdit);
    saveOnEdit(jmNativeHorizontalCellsEdit);
    saveOnEdit(jmNativeVerticalLayersEdit);
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
        solveProgressObserved = false;
        vnResultGridStatus = QStringLiteral("not_run_in_current_app");
        vnErtSnapshotStatus = QStringLiteral("not_run_in_current_app");
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
        const QString previousRunOutput = currentRunOutput;
        suppressedRuntimeNoiseLines += suppressed;
        currentRunOutput += filtered;

        const QStringList newHighlights = NewlySeenRuntimeHighlights(previousRunOutput, filtered);
        for (const QString &marker : newHighlights) {
            appendLog(stamp(tr("Runtime status: %1").arg(marker)));
        }

        if (!solveProgressObserved && HasSimulationProgressOutput(currentRunOutput)) {
            solveProgressObserved = true;
            const QString marker = FirstSimulationProgressMarker(currentRunOutput);
            appendLog(stamp(tr("Solve progress detected (%1).").arg(marker.isEmpty() ? tr("runtime marker") : marker)));
        }

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
        const bool parseConfigLooksFatal = parseConfigError && !HasSimulationProgressOutput(currentRunOutput);
        if (parseConfigError && !parseConfigLooksFatal) {
            appendLog(stamp(tr("Configuration parse warning was detected, but simulation progress output was also detected; continuing.")));
        }
        if (parseConfigLooksFatal && !pendingGuiRetryArgs.isEmpty()) {
            const QStringList retryArgs = pendingGuiRetryArgs.takeFirst();
            appendLog(stamp(tr("Detected configuration-parse error. Retrying GUI launch with args: %1")
                            .arg(retryArgs.join(' '))));
            runner->setExecutablePath(pendingGuiRetryExecutable);
            runner->runScript(pendingGuiRetryScript, pendingGuiRetryWorkingDirectory, retryArgs);
            return;
        }
        if (parseConfigLooksFatal) {
            pendingGuiRetryArgs.clear();
            QMessageBox::warning(this,
                                 tr("Simulation did not start"),
                                 tr("OpenHydroQual reported a configuration parse error for all attempted argument patterns.\n\n"
                                    "This usually means the selected executable expects a JSON configuration file interface rather than direct .ohq execution.\n\n"
                                    "Try one of the following:\n"
                                    "1) Select an OHQ CLI solver binary if available.\n"
                                    "2) Provide explicit executable args required by your OpenHydroQual build.\n"
                                    "3) Use an external runner flow aligned with one of this repo's structures "
                                    "(HQ_Drywell, R_Bioswale, or VN_Drywell).\n"
                                    "4) Or select a custom internal solver executable (System/Solve main) and leave args empty."));
            appendLog(stamp(tr("Run ended without simulation: OpenHydroQual parse-configuration error persisted after fallback retries.")));
            return;
        } else {
            pendingGuiRetryArgs.clear();
        }
        if (solveProgressObserved && exitCode == 0) {
            appendLog(stamp(tr("Solve phase completed; proceeding to plot/artifact refresh.")));
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
                return;
            }
            if (!solveProgressObserved) {
                appendLog(stamp(tr("Run exited with non-zero code before solve progress; artifact scan skipped.")));
                return;
            }
            appendLog(stamp(tr("Run exited with non-zero code after solve progress; continuing to artifact scan.")));
        }

        refreshPlots();

        const QStringList generatedVtkArtifacts = createVnVtkOutputsFromRunArtifacts();
        if (!generatedVtkArtifacts.isEmpty()) {
            appendLog(stamp(tr("VN VTK export refreshed %1 file(s) after run.").arg(generatedVtkArtifacts.size())));
        }

        const QStringList artifacts = collectRunArtifacts();
        if (artifacts.isEmpty()) {
            appendLog(stamp(tr("No new artifacts detected in working directory.")));
            return;
        }

        appendLog(stamp(tr("Detected %1 artifact file(s):").arg(artifacts.count())));
        for (const QString &file : artifacts) {
            appendLog(QStringLiteral("  - %1").arg(file));
        }

        updateVnRuntimeStatusFromArtifacts(artifacts);
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

    QString modePrefix;
    if (modelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0) {
        modePrefix = QStringLiteral("VN_MODE");
    } else if (modelType.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0) {
        modePrefix = QStringLiteral("HQ_MODE");
    } else if (modelType.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0) {
        modePrefix = QStringLiteral("R_MODE");
    } else if (modelType.compare(QStringLiteral("JM_Bioretention"), Qt::CaseInsensitive) == 0) {
        modePrefix = QStringLiteral("JM_MODE");
    }

    const QString defaultModePreset = modePrefix.isEmpty()
        ? QString()
        : QStringLiteral("%1:SoftReference").arg(modePrefix);

    QSet<QString> seenPresetData;
    QSet<QString> seenPresetLabels;
    const auto addUniquePresetItem = [&](const QString &label, const QString &data) {
        const QString normalizedLabel = label.trimmed();
        const QString normalizedData = data.trimmed();
        if ((!normalizedData.isEmpty() && seenPresetData.contains(normalizedData))
            || (!normalizedLabel.isEmpty() && seenPresetLabels.contains(normalizedLabel))) {
            return;
        }
        enrichmentPresetCombo->addItem(label, data);
        if (!normalizedData.isEmpty()) {
            seenPresetData.insert(normalizedData);
        }
        if (!normalizedLabel.isEmpty()) {
            seenPresetLabels.insert(normalizedLabel);
        }
    };

    addUniquePresetItem(tr("None"), "");

    if (!modePrefix.isEmpty()) {
        addUniquePresetItem(tr("SoftReference"), defaultModePreset);
        addUniquePresetItem(tr("LoadFromOhq"), QStringLiteral("%1:LoadFromOhq").arg(modePrefix));
        addUniquePresetItem(tr("FullReference"), QStringLiteral("%1:FullReference").arg(modePrefix));

        if (modelType.compare(QStringLiteral("JM_Bioretention"),
                              Qt::CaseInsensitive) == 0) {
            addUniquePresetItem(tr("Curb Channel"),
                                QStringLiteral("JM_MODE:Channel"));
            addUniquePresetItem(tr("DT Simple"),
                                QStringLiteral("JM_MODE:DTSimple"));
            addUniquePresetItem(tr("DT Simple + Gutters"),
                                QStringLiteral("JM_MODE:DTSimpleGutter"));
            addUniquePresetItem(tr("JM Test (2010 real data)"),
                                QStringLiteral("JM_MODE:JMTest"));
        } else if (modelType.compare(QStringLiteral("R_Bioswale"),
                                     Qt::CaseInsensitive) == 0) {
            addUniquePresetItem(tr("Simple"), QStringLiteral("R_MODE:Simple"));
        }
    }

    const auto options = StructureRegistry::PresetOptionsForModel(modelType);
    for (const auto &option : options) {
        const QString optionLabel = option.first.trimmed();
        const QString optionData = option.second.trimmed();
        const QString modePresetToken = modePrefix.isEmpty()
            ? QString()
            : QStringLiteral("%1:Preset").arg(modePrefix);
        const bool isGenericPresetUiEntry = optionLabel.compare(QStringLiteral("Preset"), Qt::CaseInsensitive) == 0
            || optionData.compare(QStringLiteral("Preset"), Qt::CaseInsensitive) == 0
            || (!modePresetToken.isEmpty() && optionData.compare(modePresetToken, Qt::CaseInsensitive) == 0);
        if (isGenericPresetUiEntry) {
            continue;
        }
        addUniquePresetItem(option.first, option.second);
    }

    int index = -1;
    if (!previousPreset.isEmpty()) {
        index = enrichmentPresetCombo->findData(previousPreset);
    }
    if (index < 0 && !defaultModePreset.isEmpty()) {
        index = enrichmentPresetCombo->findData(defaultModePreset);
    }
    enrichmentPresetCombo->setCurrentIndex(index >= 0 ? index : 0);
    if (index < 0 && !previousPreset.isEmpty()) {
        appendLog(stamp(tr("Preset '%1' hidden for model type '%2'; reset to default mode.")
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
    const QString hqBuildMode = BuildModeFromPresetSelection(preset, QStringLiteral("HQ_MODE"));
    const QString rBuildMode = BuildModeFromPresetSelection(preset, QStringLiteral("R_MODE"));
    const QString jmBuildMode = BuildModeFromPresetSelection(preset, QStringLiteral("JM_MODE"));
    const bool explicitNonSoftMode = vnBuildMode.compare(QStringLiteral("FullReference"), Qt::CaseInsensitive) == 0
        || vnBuildMode.compare(QStringLiteral("LoadFromOhq"), Qt::CaseInsensitive) == 0;
    const bool hqSoftContext = modelType.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0
        && (hqBuildMode.isEmpty() || hqBuildMode.compare(QStringLiteral("SoftReference"), Qt::CaseInsensitive) == 0);
    const bool rSoftContext = modelType.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0
        && (rBuildMode.isEmpty()
            || rBuildMode.compare(QStringLiteral("SoftReference"), Qt::CaseInsensitive) == 0
            || rBuildMode.compare(QStringLiteral("Simple"), Qt::CaseInsensitive) == 0);
    const bool jmContext = modelType.compare(QStringLiteral("JM_Bioretention"), Qt::CaseInsensitive) == 0;
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
    // VN soil properties are structure-based for now.
    // R_Bioswale and HQ_Drywell use their own soil controls below; keep this VN row turned off there.
    if (vnSoftSoilParamsRowWidget) vnSoftSoilParamsRowWidget->setVisible(showSoftRows);
    if (hqSoftGeometryRowWidget) hqSoftGeometryRowWidget->setVisible(!loadExistingMode && hqSoftContext);
    if (hqSoilControlsRowWidget) hqSoilControlsRowWidget->setVisible(!loadExistingMode && hqSoftContext);
    if (rSoilGeometryRowWidget) rSoilGeometryRowWidget->setVisible(!loadExistingMode && rSoftContext);
    // Shared domain row: for JM, total_nz is native nx and native_nz is native nz.
    if (rSoilDomainRowWidget) rSoilDomainRowWidget->setVisible(!loadExistingMode && rSoftContext);
    // JMTest is a fixed 2010 real-data preset. Keep its soil-block topology
    // exactly as defined by the JM builder; only the corrected elevations and
    // test forcing/settings should differ from the regular JM modes.
    const bool jmEditableNativeGrid = jmContext
        && jmBuildMode.compare(QStringLiteral("FullReference"), Qt::CaseInsensitive) != 0
        && jmBuildMode.compare(QStringLiteral("LoadFromOhq"), Qt::CaseInsensitive) != 0
        && jmBuildMode.compare(QStringLiteral("JMTest"), Qt::CaseInsensitive) != 0
        && jmBuildMode.compare(QStringLiteral("Test2010"), Qt::CaseInsensitive) != 0
        && jmBuildMode.compare(QStringLiteral("DTSimpleGutter2010"), Qt::CaseInsensitive) != 0;
    if (jmNativeDomainRowWidget) jmNativeDomainRowWidget->setVisible(!loadExistingMode && jmEditableNativeGrid);
    if (rSoilControlsRowWidget) rSoilControlsRowWidget->setVisible(!loadExistingMode && rSoftContext);
    if (vnInitThetaRowWidget) vnInitThetaRowWidget->setVisible(!loadExistingMode && vnContext);
    if (vnFieldGeneratorRowWidget) vnFieldGeneratorRowWidget->setVisible(!loadExistingMode && vnContext);
    if (vnSoilToolRowWidget) vnSoilToolRowWidget->setVisible(vnContext);
    if (vnOutputToolRowWidget) vnOutputToolRowWidget->setVisible(vnContext);

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
    SetAutoSuggestedField(exePathEdit, false);
    if (workingDirEdit->text().trimmed().isEmpty()) {
        workingDirEdit->setText(FindRepoRoot());
        SetAutoSuggestedField(workingDirEdit, true);
    }
    if (templateDirEdit->text().trimmed().isEmpty()) {
        const QStringList rootCandidates = CandidateOpenHydroQualRoots(FindRepoRoot(), {dir, cliPath});
        const QString detectedTemplate = DetectTemplateDirectory(rootCandidates, workingDirEdit->text().trimmed());
        if (!detectedTemplate.isEmpty()) {
            templateDirEdit->setText(detectedTemplate);
            SetAutoSuggestedField(templateDirEdit, true);
            appendLog(stamp(tr("Auto-detected template directory: %1").arg(detectedTemplate)));
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
        SetAutoSuggestedField(scriptPathEdit, false);
        const QFileInfo info(fileName);
        if (workingDirEdit->text().isEmpty()) {
            workingDirEdit->setText(info.absolutePath());
            SetAutoSuggestedField(workingDirEdit, true);
        }
        saveSettings();
    }
}

void ModelCreatorWindow::chooseWorkingDirectory()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Select working directory"));
    if (!dir.isEmpty()) {
        workingDirEdit->setText(dir);
        SetAutoSuggestedField(workingDirEdit, false);
        const QStringList rootCandidates = CandidateOpenHydroQualRoots(FindRepoRoot(), {dir, exePathEdit->text().trimmed()});
        if (exePathEdit->text().trimmed().isEmpty()) {
            const QString detectedExecutable = DetectExecutablePath(rootCandidates);
            if (!detectedExecutable.isEmpty()) {
                exePathEdit->setText(detectedExecutable);
                SetAutoSuggestedField(exePathEdit, true);
                appendLog(stamp(tr("Auto-detected OHQ executable from selected working directory: %1")
                                .arg(detectedExecutable)));
            }
        }
        if (templateDirEdit->text().trimmed().isEmpty()) {
            const QString detectedTemplate = DetectTemplateDirectory(rootCandidates, dir);
            if (!detectedTemplate.isEmpty()) {
                templateDirEdit->setText(detectedTemplate);
                SetAutoSuggestedField(templateDirEdit, true);
                appendLog(stamp(tr("Auto-detected template directory from selected working directory: %1")
                                .arg(detectedTemplate)));
            }
        }
        syncVnToolDefaultPaths();
        saveSettings();
    }
}

void ModelCreatorWindow::chooseArtifactsDirectory()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Select artifacts directory"));
    if (!dir.isEmpty()) {
        artifactsDirEdit->setText(dir);
        SetAutoSuggestedField(artifactsDirEdit, false);
        saveSettings();
    }
}

void ModelCreatorWindow::chooseTemplateDirectory()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Select OHQ template directory"));
    if (!dir.isEmpty()) {
        templateDirEdit->setText(dir);
        SetAutoSuggestedField(templateDirEdit, false);
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
        SetAutoSuggestedField(generatedScriptEdit, false);
        saveSettings();
    }
}

void ModelCreatorWindow::applySuggestedDefaults()
{
    const QString repoRoot = FindRepoRoot();
    const QString suggestedWorkingDirectory = QDir(repoRoot).filePath("Models");
    QDir().mkpath(suggestedWorkingDirectory);
    const QString suggestedArtifactsDirectory = QDir(suggestedWorkingDirectory).filePath("artifacts");
    const QStringList rootCandidates = CandidateOpenHydroQualRoots(repoRoot, {
        workingDirEdit->text().trimmed(),
        exePathEdit->text().trimmed(),
        templateDirEdit->text().trimmed()
    });
    const QString suggestedTemplateDirectory = DetectTemplateDirectory(rootCandidates, suggestedWorkingDirectory);
    const QString suggestedGeneratedScriptPath = QDir(suggestedWorkingDirectory).filePath("starter_generated.ohq");
    const QString suggestedExecutablePath = DetectExecutablePath(rootCandidates);
    const QString suggestedInflowPath = DetectSuggestedInflowFile(modelTypeCombo->currentText(),
                                                                  templateDirEdit->text().trimmed());
    const QString suggestedRBioswaleSoilPath;
    const QString suggestedScriptPath = FirstExistingFile({
        QDir(suggestedWorkingDirectory).filePath("hq_drywell.ohq"),
        QDir(suggestedWorkingDirectory).filePath("vn_drywell.ohq"),
        QDir(suggestedWorkingDirectory).filePath("r_bioswale.ohq"),
        QDir(suggestedWorkingDirectory).filePath("examples/hq_drywell.ohq"),
        QDir(suggestedWorkingDirectory).filePath("examples/vn_drywell.ohq"),
        QDir(suggestedWorkingDirectory).filePath("examples/r_bioswale.ohq")
    });

    ApplySuggestedFieldValue(exePathEdit, suggestedExecutablePath);
    ApplySuggestedFieldValue(exeArgsEdit, QStringLiteral("{script}"));
    if (!suggestedExecutablePath.isEmpty()) {
        const QFileInfo currentExe(exePathEdit->text().trimmed());
        if (LooksLikeScriptFilePath(currentExe) || LooksLikeStaticLibraryPath(currentExe) || !currentExe.isExecutable()) {
            exePathEdit->setText(suggestedExecutablePath);
            SetAutoSuggestedField(exePathEdit, true);
            appendLog(stamp(tr("Replaced invalid executable path with suggested OHQ binary: %1")
                            .arg(suggestedExecutablePath)));
        }
    }
    ApplySuggestedFieldValue(scriptPathEdit, suggestedScriptPath);
    ApplySuggestedFieldValue(workingDirEdit, suggestedWorkingDirectory);
    ApplySuggestedFieldValue(artifactsDirEdit, suggestedArtifactsDirectory);
    ApplySuggestedFieldValue(templateDirEdit, suggestedTemplateDirectory);
    ApplySuggestedFieldValue(generatedScriptEdit, suggestedGeneratedScriptPath);
    ApplySuggestedFieldValue(rSoilPropsFileEdit, suggestedRBioswaleSoilPath);
    const bool inflowUpdated = ApplySuggestedFieldValue(inflowFileEdit, suggestedInflowPath);
    if (inflowUpdated
        || (!suggestedInflowPath.trimmed().isEmpty()
            && inflowFileEdit->text().trimmed().compare(suggestedInflowPath.trimmed(), Qt::CaseInsensitive) == 0)) {
        inflowAutoSuggested = true;
    }
    const QString suggestedOutputSeriesPath =
        QDir(workingDirEdit->text().trimmed().isEmpty() ? suggestedWorkingDirectory : workingDirEdit->text().trimmed())
            .filePath(QStringLiteral("OHQ_output.txt"));
    ApplySuggestedFieldValue(outputSeriesFileEdit, suggestedOutputSeriesPath);
    if (!inflowFileEdit->text().trimmed().isEmpty()) {
        suggestSimulationWindowFromInflow(inflowFileEdit->text().trimmed(), true);
    }
    ApplySuggestedFieldValue(simulationStartEdit, QStringLiteral("44435"));
    ApplySuggestedFieldValue(simulationEndEdit, QStringLiteral("44438"));

    saveSettings();
    appendLog(stamp(tr("Applied suggested defaults to empty or auto-suggested setup fields.")));
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
        SetAutoSuggestedField(guiConfigTemplateEdit, false);
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
        SetAutoSuggestedField(inflowFileEdit, false);
        inflowAutoSuggested = false;
        suggestSimulationWindowFromInflow(fileName, true);
        saveSettings();
        refreshPlots();
    }
}

void ModelCreatorWindow::suggestSimulationWindowFromInflow(const QString &path, bool forceApply)
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
    if (!forceApply && !usingDefaults && !simulationWindowAutoSuggested) {
        return;
    }

    simulationStartEdit->setText(QString::number(minX, 'g', 12));
    simulationEndEdit->setText(QString::number(maxX, 'g', 12));
    simulationWindowAutoSuggested = true;
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
        SetAutoSuggestedField(observationFileEdit, false);
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
        SetAutoSuggestedField(depthProfileFileEdit, false);
        saveSettings();
        refreshPlots();
    }
}

void ModelCreatorWindow::chooseVnBaseOhqFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Select base OHQ script"),
                                                          vnBaseOhqFileEdit->text(),
                                                          tr("OHQ/Text files (*.ohq *.txt);;All files (*.*)"));
    if (!fileName.isEmpty()) {
        vnBaseOhqFileEdit->setText(fileName);
        SetAutoSuggestedField(vnBaseOhqFileEdit, false);
        saveSettings();
    }
}

void ModelCreatorWindow::chooseVnSoilLayersFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Select soil layers snippet"),
                                                          vnSoilLayersFileEdit->text(),
                                                          tr("Supported files (*.ohq *.txt *.csv);;All files (*.*)"));
    if (!fileName.isEmpty()) {
        vnSoilLayersFileEdit->setText(fileName);
        SetAutoSuggestedField(vnSoilLayersFileEdit, false);
        saveSettings();
    }
}

void ModelCreatorWindow::chooseVnMoistureLayersFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Select moisture layers snippet"),
                                                          vnMoistureLayersFileEdit->text(),
                                                          tr("Supported files (*.ohq *.txt *.csv);;All files (*.*)"));
    if (!fileName.isEmpty()) {
        vnMoistureLayersFileEdit->setText(fileName);
        SetAutoSuggestedField(vnMoistureLayersFileEdit, false);
        saveSettings();
    }
}

void ModelCreatorWindow::chooseVnSoftSoilParameterFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Select soft soil parameter profile CSV"),
                                                          vnSoftSoilParameterFileEdit->text(),
                                                          tr("CSV files (*.csv);;Text files (*.txt);;All files (*.*)"));
    if (!fileName.isEmpty()) {
        vnSoftSoilParameterFileEdit->setText(fileName);
        SetAutoSuggestedField(vnSoftSoilParameterFileEdit, false);
        saveSettings();
    }
}

void ModelCreatorWindow::chooseHqSoilPropsFile()
{
    const QString startDir = hqSoilPropsFileEdit->text().trimmed().isEmpty()
        ? workingDirEdit->text().trimmed()
        : QFileInfo(hqSoilPropsFileEdit->text().trimmed()).absolutePath();
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Select HQ/DryWell soil properties file"),
                                                          startDir,
                                                          tr("Data files (*.txt *.csv *.dat);;All files (*.*)"));
    if (!fileName.isEmpty()) {
        hqSoilPropsFileEdit->setText(fileName);
        SetAutoSuggestedField(hqSoilPropsFileEdit, false);
        saveSettings();
    }
}


void ModelCreatorWindow::chooseRBioswaleSoilPropsFile()
{
    const QString startDir = rSoilPropsFileEdit->text().trimmed().isEmpty()
        ? workingDirEdit->text().trimmed()
        : QFileInfo(rSoilPropsFileEdit->text().trimmed()).absolutePath();
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Select Rosemead soil properties file"),
                                                          startDir,
                                                          tr("Data files (*.txt *.csv *.dat);;All files (*.*)"));
    if (!fileName.isEmpty()) {
        rSoilPropsFileEdit->setText(fileName);
        SetAutoSuggestedField(rSoilPropsFileEdit, false);
        saveSettings();
    }
}


void ModelCreatorWindow::showRBioswaleSoilPropsTable()
{
    QString path = rSoilPropsFileEdit->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::information(this, tr("R soil props table"), tr("Choose an R/Rosemead soil properties file first."));
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("R soil props table"), tr("Could not open:\n%1").arg(path));
        return;
    }

    QStringList lines = QString::fromUtf8(file.readAll()).split('\n', Qt::SkipEmptyParts);
    for (QString &line : lines) {
        line = line.trimmed();
    }
    lines.removeAll(QString());
    if (lines.size() < 2) {
        QMessageBox::warning(this, tr("R soil props table"), tr("Selected soil file has no data rows."));
        return;
    }

    const QStringList sourceHeaders = lines.first().split(QRegularExpression(QStringLiteral("[,;\\t]")), Qt::KeepEmptyParts);
    QVector<QStringList> sourceRows;
    for (int i = 1; i < lines.size(); ++i) {
        sourceRows.push_back(lines.at(i).split(QRegularExpression(QStringLiteral("[,;\\t]")), Qt::KeepEmptyParts));
    }

    bool depthOk = false;
    const double depth = rBioSwaleDepthEdit->text().trimmed().toDouble(&depthOk);
    const double bioswaleDepth = (depthOk && depth > 0.0) ? depth : 0.9144;

    int depthColumn = -1;
    for (int c = 0; c < sourceHeaders.size(); ++c) {
        const QString h = sourceHeaders.at(c).trimmed().toLower();
        if (h == QStringLiteral("depth") || h == QStringLiteral("depth_m")) {
            depthColumn = c;
            break;
        }
    }

    int minimumNz = sourceRows.size();
    if (depthColumn >= 0) {
        double cumulative = 0.0;
        for (int i = 0; i < sourceRows.size(); ++i) {
            if (depthColumn < sourceRows.at(i).size()) {
                bool ok = false;
                const double d = sourceRows.at(i).at(depthColumn).trimmed().toDouble(&ok);
                if (ok) cumulative += d;
            }
            if (cumulative > bioswaleDepth + 1e-9) {
                minimumNz = i + 1;
                break;
            }
        }
    }

    const int fileNz = sourceRows.size();

    QString nzText = rVerticalLayersEdit ? rVerticalLayersEdit->text().trimmed() : QString();
    const bool nzWasBlankOrAuto = nzText.isEmpty()
        || nzText.compare(QStringLiteral("auto"), Qt::CaseInsensitive) == 0;

    // Check should make Auto explicit: show the file-driven nz in the window,
    // but do not overwrite a user-entered nz such as 5.
    if (rVerticalLayersEdit) {
        rVerticalLayersEdit->setPlaceholderText(QString::number(fileNz));
        if (nzWasBlankOrAuto) {
            const QSignalBlocker blocker(rVerticalLayersEdit);
            rVerticalLayersEdit->setText(QString::number(fileNz));
            SetAutoSuggestedField(rVerticalLayersEdit, true);
            nzText = rVerticalLayersEdit->text().trimmed();
        }
    }

    bool nzOk = false;
    const int requestedNz = nzText.toInt(&nzOk);
    const int effectiveNz = (nzOk && requestedNz > 0) ? qMax(requestedNz, 2) : fileNz;

    auto *dialog = new QDialog(this);
    dialog->setWindowTitle(tr("R/Rosemead soil layers: effective nz=%1, file nz=%2").arg(effectiveNz).arg(fileNz));
    dialog->resize(900, 560);
    auto *layout = new QVBoxLayout(dialog);

    QString note;
    if (!nzOk || requestedNz <= 0) {
        note = tr("nz was not valid: using all rows from the soil file.");
    } else if (nzWasBlankOrAuto && requestedNz == fileNz) {
        note = tr("nz was Auto/blank. Check filled it with the file nz (%1).").arg(fileNz);
    } else if (minimumNz > requestedNz) {
        note = tr("Requested nz=%1 is shallower than the Rosemead depth split (%2 rows). Generation will still use nz=%3 and will force the last effective row to be the Bottom/UEngineered/GW-connected layer.")
                   .arg(requestedNz).arg(minimumNz).arg(effectiveNz);
    } else if (requestedNz < sourceRows.size()) {
        note = tr("nz trims the file after row %1.").arg(requestedNz);
    } else if (requestedNz > sourceRows.size()) {
        note = tr("nz is larger than file rows: generation repeats the last valid row.");
    } else {
        note = tr("nz matches the file row count.");
    }

    auto *summary = new QLabel(tr("File: %1\nFile nz: %2\nEffective nz: %3\nMinimum Rosemead split row for current bioswale depth: %4\n%5")
                                   .arg(path)
                                   .arg(fileNz)
                                   .arg(effectiveNz)
                                   .arg(minimumNz)
                                   .arg(note), dialog);
    summary->setWordWrap(true);
    layout->addWidget(summary);

    QStringList headers;
    headers << tr("effective_layer") << tr("source_row") << tr("source_note");
    for (const QString &h : sourceHeaders) headers << h.trimmed();

    auto *table = new QTableWidget(dialog);
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setRowCount(effectiveNz);

    for (int i = 0; i < effectiveNz; ++i) {
        const int sourceIndex = qMin(i, sourceRows.size() - 1);
        table->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        table->setItem(i, 1, new QTableWidgetItem(QString::number(sourceIndex + 1)));
        QString sourceNote;
        if (i >= sourceRows.size()) sourceNote = tr("repeated last row");
        else if (nzOk && requestedNz > 0 && effectiveNz > requestedNz && i + 1 > requestedNz) sourceNote = tr("kept for bottom/GW safety");
        else sourceNote = tr("file row");
        table->setItem(i, 2, new QTableWidgetItem(sourceNote));

        const QStringList row = sourceRows.at(sourceIndex);
        for (int c = 0; c < sourceHeaders.size(); ++c) {
            table->setItem(i, c + 3, new QTableWidgetItem(c < row.size() ? row.at(c).trimmed() : QString()));
        }
    }

    table->resizeColumnsToContents();
    layout->addWidget(table, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    layout->addWidget(buttons);

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}


void ModelCreatorWindow::showHqSoilPropsTable()
{
    const QString hqFilePath = hqSoilPropsFileEdit ? hqSoilPropsFileEdit->text().trimmed() : QString();
    const QString mode = !hqFilePath.isEmpty() ? QStringLiteral("HQ soil file")
                                               : (vnSoftSoilParamModeCombo ? vnSoftSoilParamModeCombo->currentData().toString().trimmed() : QString());
    const QString compactMode = QString(mode).toLower().remove(' ').remove('_').remove('-');
    const bool fileMode = !hqFilePath.isEmpty();
    const bool modelCreatorDefaults = mode.compare(QStringLiteral("ModelCreatorDefaults"), Qt::CaseInsensitive) == 0
        || compactMode == QStringLiteral("modelcreatordefaults");
    const bool referenceDefaults = mode.compare(QStringLiteral("ReferenceDefaults"), Qt::CaseInsensitive) == 0
        || compactMode == QStringLiteral("referencedefaults")
        || compactMode == QStringLiteral("vnrefdefaults")
        || compactMode == QStringLiteral("vnreferencedefaults");

    bool layersOk = false;
    int layers = hqSoftShallowLayersEdit ? hqSoftShallowLayersEdit->text().trimmed().toInt(&layersOk) : 0;
    if (!layersOk || layers <= 0) {
        layers = 34;
        if (hqSoftShallowLayersEdit) {
            const QSignalBlocker blocker(hqSoftShallowLayersEdit);
            hqSoftShallowLayersEdit->setText(QString::number(layers));
            SetAutoSuggestedField(hqSoftShallowLayersEdit, true);
        }
    }

    bool wellDepthOk = false;
    const double wellDepth = hqSoftWellDepthEdit ? hqSoftWellDepthEdit->text().trimmed().toDouble(&wellDepthOk) : 0.0;
    const double effectiveWellDepth = (wellDepthOk && wellDepth > 0.0) ? wellDepth : 20.0;
    const double dz = effectiveWellDepth / static_cast<double>(qMax(1, layers));

    struct Row {
        int sourceRow = 0;
        double sourceDepth = 0.0;
        double ksat = 1.0;
        double alpha = 1.0;
        double n = 1.41;
        double thetaSat = 0.4;
        double thetaRes = 0.05;
    };

    auto norm = [](QString h) {
        h = h.trimmed().toLower();
        h.remove(QLatin1Char(' '));
        h.remove(QLatin1Char('_'));
        h.remove(QLatin1Char('-'));
        return h;
    };
    auto split = [](const QString &line) {
        return line.split(QRegularExpression(QStringLiteral("[,;\\t]")), Qt::KeepEmptyParts);
    };
    auto getValue = [&](const QStringList &cells, const QHash<QString, int> &index, std::initializer_list<QString> names, double *value) {
        for (const QString &name : names) {
            const auto it = index.constFind(norm(name));
            if (it == index.constEnd()) continue;
            const int c = it.value();
            if (c < 0 || c >= cells.size()) continue;
            bool ok = false;
            const double v = cells.at(c).trimmed().toDouble(&ok);
            if (ok && std::isfinite(v)) {
                *value = v;
                return true;
            }
        }
        return false;
    };

    QVector<Row> fileRows;
    QString filePath = hqFilePath;
    QString note;
    if (fileMode) {
        if (filePath.isEmpty()) {
            QMessageBox::warning(this, tr("HQ soil table"), tr("HQ soil mode is File, but no soil-parameter file is selected."));
            return;
        }
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, tr("HQ soil table"), tr("Could not open:\n%1").arg(filePath));
            return;
        }
        QStringList lines = QString::fromUtf8(file.readAll()).split('\n', Qt::SkipEmptyParts);
        for (QString &line : lines) line = line.trimmed();
        lines.removeAll(QString());
        if (lines.size() < 2) {
            QMessageBox::warning(this, tr("HQ soil table"), tr("Selected soil-parameter file has no data rows."));
            return;
        }
        const QStringList headers = split(lines.first());
        QHash<QString, int> idx;
        for (int c = 0; c < headers.size(); ++c) idx.insert(norm(headers.at(c)), c);
        for (int i = 1; i < lines.size(); ++i) {
            const QStringList cells = split(lines.at(i));
            Row row;
            row.sourceRow = i;
            if (!getValue(cells, idx, {QStringLiteral("depth"), QStringLiteral("depth_m")}, &row.sourceDepth)) continue;
            getValue(cells, idx, {QStringLiteral("ksat"), QStringLiteral("k_sat_original")}, &row.ksat);
            getValue(cells, idx, {QStringLiteral("alpha")}, &row.alpha);
            getValue(cells, idx, {QStringLiteral("n")}, &row.n);
            getValue(cells, idx, {QStringLiteral("theta_s"), QStringLiteral("theta_sat")}, &row.thetaSat);
            getValue(cells, idx, {QStringLiteral("theta_r"), QStringLiteral("theta_res")}, &row.thetaRes);
            fileRows.push_back(row);
        }
        if (fileRows.isEmpty()) {
            QMessageBox::warning(this, tr("HQ soil table"), tr("No valid depth rows were found in the selected file."));
            return;
        }
        note = tr("HQ soil-file mode: each layer uses the matching file row; if layers exceed file rows, the last row is repeated.");
    } else if (modelCreatorDefaults) {
        note = tr("ModelCreatorDefaults mode: all HQ layers use ModelCreator defaults.");
    } else if (referenceDefaults) {
        note = tr("ReferenceDefaults mode: preview shows embedded reference defaults; actual existing blocks can keep their original block parameters.");
    } else {
        note = tr("Manual mode: all HQ layers use the current manual UI values.");
    }

    auto resolveFile = [&](double depth) {
        Row out;
        out.sourceDepth = depth;
        if (fileRows.isEmpty()) return out;
        if (depth <= fileRows.first().sourceDepth) {
            out = fileRows.first();
            return out;
        }
        if (depth >= fileRows.last().sourceDepth) {
            out = fileRows.last();
            return out;
        }
        for (int i = 1; i < fileRows.size(); ++i) {
            const Row &a = fileRows.at(i - 1);
            const Row &b = fileRows.at(i);
            if (depth < a.sourceDepth || depth > b.sourceDepth) continue;
            const double span = b.sourceDepth - a.sourceDepth;
            const double w = (span > 0.0) ? (depth - a.sourceDepth) / span : 1.0;
            out.sourceRow = b.sourceRow;
            out.sourceDepth = depth;
            out.ksat = a.ksat + w * (b.ksat - a.ksat);
            out.alpha = a.alpha + w * (b.alpha - a.alpha);
            out.n = a.n + w * (b.n - a.n);
            out.thetaSat = a.thetaSat + w * (b.thetaSat - a.thetaSat);
            out.thetaRes = a.thetaRes + w * (b.thetaRes - a.thetaRes);
            return out;
        }
        return fileRows.last();
    };

    QString displayMode = mode;
    if (compactMode == QStringLiteral("vnrefdefaults") || compactMode == QStringLiteral("vnreferencedefaults")) {
        displayMode = QStringLiteral("Reference defaults");
    } else if (compactMode == QStringLiteral("modelcreatordefaults")) {
        displayMode = QStringLiteral("ModelCreator defaults");
    }

    auto *dialog = new QDialog(this);
    dialog->setWindowTitle(tr("HQ soil parameters by layer"));
    dialog->resize(940, 560);
    auto *layout = new QVBoxLayout(dialog);

    bool nrOk = false;
    const int nr = hqSoftRadialCellsEdit ? hqSoftRadialCellsEdit->text().trimmed().toInt(&nrOk) : 0;
    auto *summary = new QLabel(tr("Mode: %1\nFile: %2\nHQ nr: %3\nHQ layers/nz: %4\nFile nz: %5\nWell depth: %6 m\nLayer dz: %7 m\n%8")
                                   .arg(displayMode.isEmpty() ? QStringLiteral("Manual") : displayMode)
                                   .arg(filePath.isEmpty() ? QStringLiteral("(none)") : filePath)
                                   .arg((nrOk && nr > 0) ? QString::number(nr) : QStringLiteral("10"))
                                   .arg(layers)
                                   .arg(fileMode ? QString::number(fileRows.size()) : QStringLiteral("(not used)"))
                                   .arg(effectiveWellDepth, 0, 'g', 10)
                                   .arg(dz, 0, 'g', 10)
                                   .arg(note), dialog);
    summary->setWordWrap(true);
    layout->addWidget(summary);

    QStringList headers;
    headers << tr("layer") << tr("mid_depth_m") << tr("source") << tr("Ksat") << tr("alpha") << tr("n") << tr("theta_sat") << tr("theta_res");
    auto *table = new QTableWidget(dialog);
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setRowCount(layers);
    table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);

    for (int i = 0; i < layers; ++i) {
        const int layer = i + 1;
        const double midDepth = (static_cast<double>(i) + 0.5) * dz;
        Row row;
        QString source;
        if (fileMode) {
            const int sourceIndex = fileRows.isEmpty() ? 0 : qMin(i, fileRows.size() - 1);
            if (!fileRows.isEmpty()) {
                row = fileRows.at(sourceIndex);
            }
            source = fileRows.isEmpty() ? tr("file row unavailable") : tr("file row %1%2").arg(sourceIndex + 1).arg(i >= fileRows.size() ? tr(" (repeated last row)") : QString());
        } else if (modelCreatorDefaults) {
            row.ksat = 1.05196; row.alpha = 3.47536; row.n = 1.74582; row.thetaSat = 0.39; row.thetaRes = 0.049;
            source = tr("ModelCreatorDefaults");
        } else if (referenceDefaults) {
            row.ksat = 1.0; row.alpha = 1.0; row.n = 1.41; row.thetaSat = 0.4; row.thetaRes = 0.05;
            source = tr("reference/default preview");
        } else {
            row.ksat = vnSoftSoilKsatOriginalEdit ? vnSoftSoilKsatOriginalEdit->text().toDouble() : 1.0;
            row.alpha = vnSoftSoilAlphaEdit ? vnSoftSoilAlphaEdit->text().toDouble() : 1.0;
            row.n = vnSoftSoilNEdit ? vnSoftSoilNEdit->text().toDouble() : 1.41;
            row.thetaSat = vnSoftSoilThetaSatEdit ? vnSoftSoilThetaSatEdit->text().toDouble() : 0.4;
            row.thetaRes = vnSoftSoilThetaResEdit ? vnSoftSoilThetaResEdit->text().toDouble() : 0.05;
            source = tr("manual UI values");
        }
        table->setItem(i, 0, new QTableWidgetItem(QString::number(layer)));
        table->setItem(i, 1, new QTableWidgetItem(QString::number(midDepth, 'g', 10)));
        table->setItem(i, 2, new QTableWidgetItem(source));
        table->setItem(i, 3, new QTableWidgetItem(QString::number(row.ksat, 'g', 10)));
        table->setItem(i, 4, new QTableWidgetItem(QString::number(row.alpha, 'g', 10)));
        table->setItem(i, 5, new QTableWidgetItem(QString::number(row.n, 'g', 10)));
        table->setItem(i, 6, new QTableWidgetItem(QString::number(row.thetaSat, 'g', 10)));
        table->setItem(i, 7, new QTableWidgetItem(QString::number(row.thetaRes, 'g', 10)));
    }

    table->resizeColumnsToContents();
    layout->addWidget(table, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    layout->addWidget(buttons);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void ModelCreatorWindow::showVnSoilPropsTable()
{
    showVnReferenceDefaultsTable();
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
        QMessageBox::warning(this, tr("VN soil table"), tr("Could not load VN soil-parameter profile for the current mode."));
        return;
    }

    const QStringList lines = csv.split('\n', Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        QMessageBox::warning(this, tr("VN soil table"), tr("VN soil-parameter table is empty."));
        return;
    }

    const QStringList headers = lines.first().split(',', Qt::KeepEmptyParts);
    auto *dialog = new QDialog(this);
    dialog->setWindowTitle(tr("VN soil parameters (%1)").arg(currentMode.isEmpty() ? QStringLiteral("Manual") : currentMode));
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
        QString vnValidationError;
    if (!validateVnAwarenessInputs(&vnValidationError, false)) {
        QMessageBox::warning(this, tr("VN validation"), vnValidationError);
        return;
    }

    StarterScriptOptions options;
        options.templateDirectory = templateDirEdit->text().trimmed();
        options.outputFile = generatedScriptEdit->text().trimmed();
        options.modelType = modelTypeCombo->currentText();
        options.enrichmentPreset = enrichmentPresetCombo->currentData().toString();
        options.useInflowFile = !inflowUseButton || inflowUseButton->isChecked();
        options.inflowFile = options.useInflowFile ? inflowFileEdit->text().trimmed() : QString();
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
            } else if (selectedPreset.isEmpty()) {
                options.vnBuildMode = QStringLiteral("SoftReference");
                options.vnPreset.clear();
            } else {
                options.vnBuildMode = QStringLiteral("SoftReference");
                options.vnPreset.clear();
            }
            if (options.vnBuildMode.compare(QStringLiteral("FullReference"), Qt::CaseInsensitive) == 0
                && IsVnSoftCustomizationRequested(options)) {
                options.vnBuildMode = QStringLiteral("SoftReference");
                const int softIndex = enrichmentPresetCombo->findData(QStringLiteral("VN_MODE:SoftReference"));
                if (softIndex >= 0) {
                    enrichmentPresetCombo->setCurrentIndex(softIndex);
                }
                appendLog(stamp(tr("VN mode auto-switched to SoftReference because VN soft controls/snippets were customized.")));
            }
        } else if (options.modelType.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0) {
            const QString selectedPreset = options.enrichmentPreset.trimmed();
            const QString selectedHqMode = BuildModeFromPresetSelection(selectedPreset, QStringLiteral("HQ_MODE"));
            if (!selectedHqMode.isEmpty()) {
                options.hqBuildMode = selectedHqMode;
                options.enrichmentPreset.clear();
            } else if (selectedPreset.isEmpty()) {
                options.hqBuildMode = QStringLiteral("SoftReference");
            } else {
                options.hqBuildMode = QStringLiteral("SoftReference");
            }
            AssignIntIfProvided(hqSoftRadialCellsEdit, &options.hqSoftRadialCells);
            AssignIntIfProvided(hqSoftShallowLayersEdit, &options.hqSoftShallowLayers);
            AssignDoubleIfProvided(hqSoftWellDepthEdit, &options.hqSoftWellDepth);
            AssignDoubleIfProvided(hqSoftWellRadiusEdit, &options.hqSoftWellRadius);
            AssignDoubleIfProvided(hqSoftPondRadiusEdit, &options.hqSoftPondRadius);
            AssignDoubleIfProvided(hqSoftSurfaceElevationEdit, &options.hqSoftSurfaceElevation);
            options.hqSoilPropsFile = hqSoilPropsFileEdit ? hqSoilPropsFileEdit->text().trimmed() : QString();
            if (options.hqBuildMode.compare(QStringLiteral("FullReference"), Qt::CaseInsensitive) == 0
                && IsHqSoftCustomizationRequested(options)) {
                options.hqBuildMode = QStringLiteral("SoftReference");
                const int softIndex = enrichmentPresetCombo->findData(QStringLiteral("HQ_MODE:SoftReference"));
                if (softIndex >= 0) {
                    enrichmentPresetCombo->setCurrentIndex(softIndex);
                }
                appendLog(stamp(tr("HQ mode auto-switched to SoftReference because HQ soft-soil controls were customized.")));
            }
        } else if (options.modelType.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0) {
            const QString selectedPreset = options.enrichmentPreset.trimmed();
            const QString selectedRMode = BuildModeFromPresetSelection(selectedPreset, QStringLiteral("R_MODE"));
            if (!selectedRMode.isEmpty()) {
                options.rBioswaleBuildMode = selectedRMode;
                options.enrichmentPreset.clear();
            } else if (selectedPreset.isEmpty()) {
                options.rBioswaleBuildMode = QStringLiteral("SoftReference");
            } else {
                options.rBioswaleBuildMode = QStringLiteral("SoftReference");
            }
            AssignDoubleIfProvided(rBioSwaleWidthEdit, &options.rBioSwaleWidth);
            AssignDoubleIfProvided(rSystemWidthEdit, &options.rSystemWidth);
            AssignDoubleIfProvided(rBioSwaleDepthEdit, &options.rBioSwaleDepth);
            AssignDoubleIfProvided(rLengthEdit, &options.rLength);
            AssignIntIfProvided(rLateralCellsEdit, &options.rLateralCells);
            AssignDoubleIfProvided(rStreetWidthEdit, &options.rStreetWidth);
            AssignIntIfProvided(rStreetCellsEdit, &options.rStreetCells);
            AssignIntIfProvided(rVerticalLayersEdit, &options.rVerticalLayers);
            AssignIntIfProvided(rEngineeredSoilNzEdit, &options.rEngineeredSoilNz);
            AssignIntIfProvided(rNativeSoilNzEdit, &options.rNativeSoilNz);
            AssignDoubleIfProvided(rAnisoRatioEdit, &options.rAnisoRatio);
            options.rSoilPropsFile = rSoilPropsFileEdit->text().trimmed();
            if (options.rBioswaleBuildMode.compare(QStringLiteral("FullReference"), Qt::CaseInsensitive) == 0
                && IsRBioswaleSoftCustomizationRequested(options)) {
                options.rBioswaleBuildMode = QStringLiteral("SoftReference");
                const int softIndex = enrichmentPresetCombo->findData(QStringLiteral("R_MODE:SoftReference"));
                if (softIndex >= 0) {
                    enrichmentPresetCombo->setCurrentIndex(softIndex);
                }
                appendLog(stamp(tr("R mode auto-switched to SoftReference because R geometry/soil controls were customized.")));
            }
        } else if (options.modelType.compare(QStringLiteral("JM_Bioretention"), Qt::CaseInsensitive) == 0) {
            const QString selectedPreset = options.enrichmentPreset.trimmed();
            const QString selectedJmMode = BuildModeFromPresetSelection(selectedPreset, QStringLiteral("JM_MODE"));
            options.jmBuildMode = selectedJmMode.isEmpty() ? QStringLiteral("SoftReference") : selectedJmMode;
            AssignIntIfProvided(jmNativeHorizontalCellsEdit, &options.jmNativeHorizontalCells);
            AssignIntIfProvided(jmNativeVerticalLayersEdit, &options.jmNativeVerticalLayers);
            options.enrichmentPreset.clear();
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
    const auto buildVnMetadataObject = [this](const QString &phase, const QString &scriptPath, const QString &workingDirectory) {
        QJsonObject meta;
        meta.insert(QStringLiteral("phase"), phase);
        meta.insert(QStringLiteral("model_type"), modelTypeCombo->currentText().trimmed());
        meta.insert(QStringLiteral("build_mode"), vnBuildModeCombo ? vnBuildModeCombo->currentData().toString().trimmed() : QString());
        meta.insert(QStringLiteral("init_theta_mode"), vnInitThetaModeCombo->currentData().toString());
        meta.insert(QStringLiteral("field_points"), vnFieldPointsEdit->text().trimmed().isEmpty() ? QStringLiteral("200") : vnFieldPointsEdit->text().trimmed());
        meta.insert(QStringLiteral("field_seed"), vnFieldSeedEdit->text().trimmed().isEmpty() ? QStringLiteral("42") : vnFieldSeedEdit->text().trimmed());
        meta.insert(QStringLiteral("field_dx"), vnFieldDxEdit->text().trimmed().isEmpty() ? QStringLiteral("0.5") : vnFieldDxEdit->text().trimmed());
        meta.insert(QStringLiteral("field_pdf_mode"), vnFieldPdfModeCombo->currentData().toString());
        meta.insert(QStringLiteral("ksat_all"), ksatScaleEdit->text().trimmed().isEmpty() ? QStringLiteral("(blank -> reference preserved)") : ksatScaleEdit->text().trimmed());
        meta.insert(QStringLiteral("ksat_g"), ksatScaleGEdit->text().trimmed().isEmpty() ? QStringLiteral("2.5") : ksatScaleGEdit->text().trimmed());
        meta.insert(QStringLiteral("ksat_uw"), ksatScaleUwEdit->text().trimmed().isEmpty() ? QStringLiteral("35") : ksatScaleUwEdit->text().trimmed());
        meta.insert(QStringLiteral("script_path"), scriptPath);
        meta.insert(QStringLiteral("working_directory"), workingDirectory);
        meta.insert(QStringLiteral("simulation_start"), simulationStartEdit->text().trimmed());
        meta.insert(QStringLiteral("simulation_end"), simulationEndEdit->text().trimmed());
        meta.insert(QStringLiteral("use_inflow_file"), inflowUseButton ? inflowUseButton->isChecked() : true);
        meta.insert(QStringLiteral("inflow_file"), inflowUseButton && !inflowUseButton->isChecked() ? QString() : inflowFileEdit->text().trimmed());
        meta.insert(QStringLiteral("field_generator_runtime_status"), QStringLiteral("metadata_only_in_current_app"));
        meta.insert(QStringLiteral("resultgrid_runtime_status"), vnResultGridRuntimeStatus());
        meta.insert(QStringLiteral("ert_snapshot_runtime_status"), vnErtSnapshotRuntimeStatus());
        return meta;
    };

    const bool loadExistingMode = workflowModeCombo->currentData().toString() == QStringLiteral("load");
    if (loadExistingMode) {
        QMessageBox::information(this,
                                 tr("Generate from scratch disabled"),
                                 tr("Workflow mode is set to 'Load/Edit existing .ohq'.\n\n"
                                    "Switch to 'Generate from scratch' to build a new starter script."));
        return false;
    }

    const bool vnGenerationContext = modelTypeCombo->currentText().trimmed().compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0;
    if (vnGenerationContext) {
        if (!vnFieldPointsEdit->text().trimmed().isEmpty() && !IsPositiveIntegerText(vnFieldPointsEdit->text())) {
            QMessageBox::warning(this, tr("Invalid VN field settings"), tr("VN field points must be a positive integer."));
            appendLog(stamp(tr("Generation cancelled: VN field points must be a positive integer.")));
            return false;
        }
        if (!vnFieldSeedEdit->text().trimmed().isEmpty() && !IsNonNegativeIntegerText(vnFieldSeedEdit->text())) {
            QMessageBox::warning(this, tr("Invalid VN field settings"), tr("VN field seed must be a non-negative integer."));
            appendLog(stamp(tr("Generation cancelled: VN field seed must be a non-negative integer.")));
            return false;
        }
        if (!vnFieldDxEdit->text().trimmed().isEmpty() && !IsPositiveDoubleText(vnFieldDxEdit->text())) {
            QMessageBox::warning(this, tr("Invalid VN field settings"), tr("VN field dx must be a positive number."));
            appendLog(stamp(tr("Generation cancelled: VN field dx must be a positive number.")));
            return false;
        }
        if (!ksatScaleEdit->text().trimmed().isEmpty() && !IsPositiveDoubleText(ksatScaleEdit->text())) {
            QMessageBox::warning(this, tr("Invalid Ksat scale"), tr("Ksat all must be positive when provided."));
            appendLog(stamp(tr("Generation cancelled: invalid Ksat all value.")));
            return false;
        }
        if (!ksatScaleGEdit->text().trimmed().isEmpty() && !IsPositiveDoubleText(ksatScaleGEdit->text())) {
            QMessageBox::warning(this, tr("Invalid Ksat scale"), tr("Ksat g must be positive when provided."));
            appendLog(stamp(tr("Generation cancelled: invalid Ksat g value.")));
            return false;
        }
        if (!ksatScaleUwEdit->text().trimmed().isEmpty() && !IsPositiveDoubleText(ksatScaleUwEdit->text())) {
            QMessageBox::warning(this, tr("Invalid Ksat scale"), tr("Ksat uw must be positive when provided."));
            appendLog(stamp(tr("Generation cancelled: invalid Ksat uw value.")));
            return false;
        }
        const QString currentBuildMode = vnBuildModeCombo ? vnBuildModeCombo->currentData().toString().trimmed() : QString();
        if (currentBuildMode.compare(QStringLiteral("LoadFromOhq"), Qt::CaseInsensitive) == 0
            && vnBaseOhqFileEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Missing VN base script"), tr("VN build mode is LoadFromOhq, but no VN base OHQ file is selected."));
            appendLog(stamp(tr("Generation cancelled: VN build mode LoadFromOhq requires a base OHQ file.")));
            return false;
        }
        const QString soilMode = vnSoftSoilParamModeCombo->currentData().toString().trimmed();
        if (soilMode.compare(QStringLiteral("File"), Qt::CaseInsensitive) == 0
            && vnSoftSoilParameterFileEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Missing VN soil parameter file"), tr("VN soft soil parameter mode is File, but no parameter file is selected."));
            appendLog(stamp(tr("Generation cancelled: VN soil parameter File mode requires a parameter file.")));
            return false;
        }
    }

    StarterScriptOptions options;
    options.templateDirectory = templateDirEdit->text().trimmed();
    options.outputFile = generatedScriptEdit->text().trimmed();
    options.modelType = modelTypeCombo->currentText();
    options.enrichmentPreset = enrichmentPresetCombo->currentData().toString();
    options.useInflowFile = !inflowUseButton || inflowUseButton->isChecked();
    options.inflowFile = options.useInflowFile ? inflowFileEdit->text().trimmed() : QString();
    options.simulationStart = simulationStartEdit->text().trimmed();
    options.simulationEnd = simulationEndEdit->text().trimmed();
    options.outputSeriesFile = outputSeriesFileEdit->text().trimmed();
    const QString workingDirectory = workingDirEdit->text().trimmed();
    if (!options.outputSeriesFile.isEmpty()) {
        const QFileInfo outputSeriesInfo(options.outputSeriesFile);
        if (outputSeriesInfo.isRelative() && !workingDirectory.isEmpty()) {
            options.outputSeriesFile = QDir(workingDirectory).filePath(options.outputSeriesFile);
            outputSeriesFileEdit->setText(options.outputSeriesFile);
            SetAutoSuggestedField(outputSeriesFileEdit, true);
            appendLog(stamp(tr("Resolved output series path to working directory: %1").arg(options.outputSeriesFile)));
        }
    }
    options.observationFile = observationFileEdit->text().trimmed();
    options.observationObject = observationObjectEdit->text().trimmed();
    options.observationExpression = observationExpressionEdit->text().trimmed();
    options.observationName = observationNameEdit->text().trimmed();
    options.additionalCommands = additionalCommandsEdit->toPlainText();
    if (options.modelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0) {
        QStringList vnMetadata;
        vnMetadata << QStringLiteral("# vn_runner_metadata:init_theta_mode=%1").arg(vnInitThetaModeCombo->currentData().toString());
        vnMetadata << QStringLiteral("# vn_runner_metadata:field_points=%1").arg(vnFieldPointsEdit->text().trimmed().isEmpty() ? QStringLiteral("200") : vnFieldPointsEdit->text().trimmed());
        vnMetadata << QStringLiteral("# vn_runner_metadata:field_seed=%1").arg(vnFieldSeedEdit->text().trimmed().isEmpty() ? QStringLiteral("42") : vnFieldSeedEdit->text().trimmed());
        vnMetadata << QStringLiteral("# vn_runner_metadata:field_dx=%1").arg(vnFieldDxEdit->text().trimmed().isEmpty() ? QStringLiteral("0.5") : vnFieldDxEdit->text().trimmed());
        vnMetadata << QStringLiteral("# vn_runner_metadata:field_pdf_mode=%1").arg(vnFieldPdfModeCombo->currentData().toString());
        const QString vnMetadataBlock = vnMetadata.join('\n');
        if (options.additionalCommands.trimmed().isEmpty()) {
            options.additionalCommands = vnMetadataBlock;
        } else {
            options.additionalCommands = vnMetadataBlock + QStringLiteral("\n") + options.additionalCommands;
        }
    }
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
        } else if (selectedPreset.isEmpty()) {
            options.vnBuildMode = QStringLiteral("SoftReference");
            options.vnPreset.clear();
        } else {
            options.vnBuildMode = QStringLiteral("SoftReference");
            options.vnPreset.clear();
        }
        if (options.vnBuildMode.compare(QStringLiteral("FullReference"), Qt::CaseInsensitive) == 0
            && IsVnSoftCustomizationRequested(options)) {
            options.vnBuildMode = QStringLiteral("SoftReference");
            const int softIndex = enrichmentPresetCombo->findData(QStringLiteral("VN_MODE:SoftReference"));
            if (softIndex >= 0) {
                enrichmentPresetCombo->setCurrentIndex(softIndex);
            }
            appendLog(stamp(tr("VN mode auto-switched to SoftReference because VN soft controls/snippets were customized.")));
        }
    } else if (options.modelType.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0) {
        const QString selectedPreset = options.enrichmentPreset.trimmed();
        const QString selectedHqMode = BuildModeFromPresetSelection(selectedPreset, QStringLiteral("HQ_MODE"));
        if (!selectedHqMode.isEmpty()) {
            options.hqBuildMode = selectedHqMode;
            options.enrichmentPreset.clear();
        } else if (selectedPreset.isEmpty()) {
            options.hqBuildMode = QStringLiteral("SoftReference");
        } else {
            options.hqBuildMode = QStringLiteral("SoftReference");
        }
        AssignIntIfProvided(hqSoftRadialCellsEdit, &options.hqSoftRadialCells);
        AssignIntIfProvided(hqSoftShallowLayersEdit, &options.hqSoftShallowLayers);
        AssignDoubleIfProvided(hqSoftWellDepthEdit, &options.hqSoftWellDepth);
        AssignDoubleIfProvided(hqSoftWellRadiusEdit, &options.hqSoftWellRadius);
        AssignDoubleIfProvided(hqSoftPondRadiusEdit, &options.hqSoftPondRadius);
        AssignDoubleIfProvided(hqSoftSurfaceElevationEdit, &options.hqSoftSurfaceElevation);
        options.hqSoilPropsFile = hqSoilPropsFileEdit ? hqSoilPropsFileEdit->text().trimmed() : QString();
        if (options.hqBuildMode.compare(QStringLiteral("FullReference"), Qt::CaseInsensitive) == 0
            && IsHqSoftCustomizationRequested(options)) {
            options.hqBuildMode = QStringLiteral("SoftReference");
            const int softIndex = enrichmentPresetCombo->findData(QStringLiteral("HQ_MODE:SoftReference"));
            if (softIndex >= 0) {
                enrichmentPresetCombo->setCurrentIndex(softIndex);
            }
            appendLog(stamp(tr("HQ mode auto-switched to SoftReference because HQ soft-soil controls were customized.")));
        }
    } else if (options.modelType.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0) {
        const QString selectedPreset = options.enrichmentPreset.trimmed();
        const QString selectedRMode = BuildModeFromPresetSelection(selectedPreset, QStringLiteral("R_MODE"));
        if (!selectedRMode.isEmpty()) {
            options.rBioswaleBuildMode = selectedRMode;
            options.enrichmentPreset.clear();
        } else if (selectedPreset.isEmpty()) {
            options.rBioswaleBuildMode = QStringLiteral("SoftReference");
        } else {
            options.rBioswaleBuildMode = QStringLiteral("SoftReference");
        }
        AssignDoubleIfProvided(rBioSwaleWidthEdit, &options.rBioSwaleWidth);
        AssignDoubleIfProvided(rSystemWidthEdit, &options.rSystemWidth);
        AssignDoubleIfProvided(rBioSwaleDepthEdit, &options.rBioSwaleDepth);
        AssignDoubleIfProvided(rLengthEdit, &options.rLength);
        AssignIntIfProvided(rLateralCellsEdit, &options.rLateralCells);
        AssignDoubleIfProvided(rStreetWidthEdit, &options.rStreetWidth);
        AssignIntIfProvided(rStreetCellsEdit, &options.rStreetCells);
        AssignIntIfProvided(rVerticalLayersEdit, &options.rVerticalLayers);
        AssignIntIfProvided(rEngineeredSoilNzEdit, &options.rEngineeredSoilNz);
        AssignIntIfProvided(rNativeSoilNzEdit, &options.rNativeSoilNz);
        AssignDoubleIfProvided(rAnisoRatioEdit, &options.rAnisoRatio);
        options.rSoilPropsFile = rSoilPropsFileEdit->text().trimmed();
        if (options.rBioswaleBuildMode.compare(QStringLiteral("FullReference"), Qt::CaseInsensitive) == 0
            && IsRBioswaleSoftCustomizationRequested(options)) {
            options.rBioswaleBuildMode = QStringLiteral("SoftReference");
            const int softIndex = enrichmentPresetCombo->findData(QStringLiteral("R_MODE:SoftReference"));
            if (softIndex >= 0) {
                enrichmentPresetCombo->setCurrentIndex(softIndex);
            }
            appendLog(stamp(tr("R mode auto-switched to SoftReference because R geometry/soil controls were customized.")));
        }

        QStringList rMetadata;
        rMetadata << QStringLiteral("# r_bioswale_ui:bioswale_width=%1").arg(options.rBioSwaleWidth);
        rMetadata << QStringLiteral("# r_bioswale_ui:extension_left=%1").arg(options.rSystemWidth);
        rMetadata << QStringLiteral("# r_bioswale_ui:bioswale_depth=%1").arg(options.rBioSwaleDepth);
        rMetadata << QStringLiteral("# r_bioswale_ui:length=%1").arg(options.rLength);
        rMetadata << QStringLiteral("# r_bioswale_ui:lateral_cells=%1").arg(options.rLateralCells);
        if (options.rVerticalLayers > 0) {
            rMetadata << QStringLiteral("# r_bioswale_ui:total_nz=%1").arg(options.rVerticalLayers);
        }
        if (options.rEngineeredSoilNz > 0) {
            rMetadata << QStringLiteral("# r_bioswale_ui:engineered_soil_nz=%1").arg(options.rEngineeredSoilNz);
        }
        if (options.rNativeSoilNz > 0) {
            rMetadata << QStringLiteral("# r_bioswale_ui:native_soil_nz=%1").arg(options.rNativeSoilNz);
        }
        rMetadata << QStringLiteral("# r_bioswale_ui:street_width=%1").arg(options.rStreetWidth);
        rMetadata << QStringLiteral("# r_bioswale_ui:street_cells=%1").arg(options.rStreetCells);
        rMetadata << QStringLiteral("# r_bioswale_ui:anisotropy_ratio=%1").arg(options.rAnisoRatio);
        if (!options.rSoilPropsFile.isEmpty()) {
            rMetadata << QStringLiteral("# r_bioswale_ui:soil_props_file=%1").arg(options.rSoilPropsFile);
        }
        const QString rMetadataBlock = rMetadata.join('\n');
        if (options.additionalCommands.trimmed().isEmpty()) {
            options.additionalCommands = rMetadataBlock;
        } else {
            options.additionalCommands = rMetadataBlock + QStringLiteral("\n") + options.additionalCommands;
        }
    } else if (options.modelType.compare(QStringLiteral("JM_Bioretention"), Qt::CaseInsensitive) == 0) {
        const QString selectedPreset = options.enrichmentPreset.trimmed();
        const QString selectedJmMode = BuildModeFromPresetSelection(selectedPreset, QStringLiteral("JM_MODE"));
        options.jmBuildMode = selectedJmMode.isEmpty() ? QStringLiteral("SoftReference") : selectedJmMode;
        AssignIntIfProvided(jmNativeHorizontalCellsEdit, &options.jmNativeHorizontalCells);
        AssignIntIfProvided(jmNativeVerticalLayersEdit, &options.jmNativeVerticalLayers);
        options.enrichmentPreset.clear();
    }
    const bool vnModel = options.modelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0;
    const bool usingExplicitVnBase = vnModel && !options.vnBaseOhqFile.isEmpty();

    if (!usingExplicitVnBase && options.templateDirectory.isEmpty()) {
        QStringList hintRoots;
        hintRoots << workingDirEdit->text().trimmed()
                  << exePathEdit->text().trimmed();
        const QString workingDirectory = workingDirEdit->text().trimmed();
        const QStringList rootCandidates = CandidateOpenHydroQualRoots(FindRepoRoot(), hintRoots);
        const QString detectedTemplate = DetectTemplateDirectory(rootCandidates, workingDirectory);
        if (!detectedTemplate.trimmed().isEmpty()) {
            options.templateDirectory = detectedTemplate;
            templateDirEdit->setText(detectedTemplate);
            SetAutoSuggestedField(templateDirEdit, true);
            appendLog(stamp(tr("Auto-detected template directory for generation: %1").arg(detectedTemplate)));
            saveSettings();
        }
    }

    if (!usingExplicitVnBase && options.templateDirectory.isEmpty()) {
        QMessageBox::warning(this, tr("Missing template directory"),
                             tr("Could not auto-detect an OHQ template directory for this machine/context."));
        return false;
    }

    if (options.outputFile.isEmpty()) {
        QMessageBox::warning(this, tr("Missing output file"), tr("Please choose where to save the generated .ohq script."));
        return false;
    }

    if (options.useInflowFile && options.inflowFile.isEmpty()) {
        if (vnModel) {
            options.inflowFile = DetectSuggestedInflowFile(QStringLiteral("VN_Drywell"),
                                                           options.templateDirectory);
            appendLog(stamp(tr("VN inflow was empty; using default inflow file: %1").arg(options.inflowFile)));
        } else {
            options.inflowFile = DetectSuggestedInflowFile(options.modelType,
                                                           options.templateDirectory);
            appendLog(stamp(tr("%1 inflow was empty; using default inflow file: %2")
                                .arg(options.modelType, options.inflowFile)));
        }
    }

    if (!usingExplicitVnBase && options.outputSeriesFile.isEmpty()) {
        QMessageBox::warning(this, tr("Missing output filename"), tr("Please provide the OHQ output series filename."));
        return false;
    }

    if (vnModel) {
        auto parsePositiveInt = [this](QLineEdit *edit, const QString &label, int fallback, int *out) -> bool {
            if (out == nullptr) {
                return false;
            }
            const QString text = edit ? edit->text().trimmed() : QString();
            if (text.isEmpty()) {
                *out = fallback;
                return true;
            }
            bool ok = false;
            const int value = text.toInt(&ok);
            if (!ok || value <= 0) {
                QMessageBox::warning(this,
                                     tr("Invalid VN setting"),
                                     tr("%1 must be a positive integer.").arg(label));
                return false;
            }
            *out = value;
            return true;
        };
        auto parsePositiveDouble = [this](QLineEdit *edit, const QString &label, double fallback, double *out) -> bool {
            if (out == nullptr) {
                return false;
            }
            const QString text = edit ? edit->text().trimmed() : QString();
            if (text.isEmpty()) {
                *out = fallback;
                return true;
            }
            bool ok = false;
            const double value = text.toDouble(&ok);
            if (!ok || !std::isfinite(value) || value <= 0.0) {
                QMessageBox::warning(this,
                                     tr("Invalid VN setting"),
                                     tr("%1 must be a positive number.").arg(label));
                return false;
            }
            *out = value;
            return true;
        };
        auto validateOptionalPositiveKsat = [this](QLineEdit *edit, const QString &label) -> bool {
            const QString text = edit ? edit->text().trimmed() : QString();
            if (text.isEmpty()) {
                return true;
            }
            bool ok = false;
            const double value = text.toDouble(&ok);
            if (!ok || !std::isfinite(value) || value <= 0.0) {
                QMessageBox::warning(this,
                                     tr("Invalid VN Ksat scale"),
                                     tr("%1 must be blank or a positive number.").arg(label));
                return false;
            }
            return true;
        };

        int vnFieldPoints = 200;
        double vnFieldDx = 0.5;
        if (!parsePositiveInt(vnFieldPointsEdit, tr("VN field points"), 200, &vnFieldPoints)) {
            return false;
        }
        if (!parsePositiveDouble(vnFieldDxEdit, tr("VN field dx"), 0.5, &vnFieldDx)) {
            return false;
        }
        if (!validateOptionalPositiveKsat(ksatScaleEdit, tr("Ksat all"))
            || !validateOptionalPositiveKsat(ksatScaleGEdit, tr("Ksat g"))
            || !validateOptionalPositiveKsat(ksatScaleUwEdit, tr("Ksat uw"))) {
            return false;
        }
    }

    if (options.modelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0) {
        const QString effectiveAll = options.ksatScaleAll.trimmed().isEmpty() ? QStringLiteral("(blank -> reference preserved)") : options.ksatScaleAll.trimmed();
        const QString effectiveG = options.ksatScaleG.trimmed().isEmpty() ? QStringLiteral("2.5") : options.ksatScaleG.trimmed();
        const QString effectiveUw = options.ksatScaleUw.trimmed().isEmpty() ? QStringLiteral("35") : options.ksatScaleUw.trimmed();
        appendLog(stamp(tr("VN generation config: buildMode=%1, initTheta=%2, field(points=%3, seed=%4, dx=%5, pdf=%6), Ksat(all=%7, g=%8, uw=%9)")
                            .arg(options.vnBuildMode.isEmpty() ? QStringLiteral("SoftReference") : options.vnBuildMode,
                                 vnInitThetaModeCombo->currentData().toString(),
                                 vnFieldPointsEdit->text().trimmed().isEmpty() ? QStringLiteral("200") : vnFieldPointsEdit->text().trimmed(),
                                 vnFieldSeedEdit->text().trimmed().isEmpty() ? QStringLiteral("42") : vnFieldSeedEdit->text().trimmed(),
                                 vnFieldDxEdit->text().trimmed().isEmpty() ? QStringLiteral("0.5") : vnFieldDxEdit->text().trimmed(),
                                 vnFieldPdfModeCombo->currentData().toString(),
                                 effectiveAll,
                                 effectiveG,
                                 effectiveUw)));
        appendLog(stamp(tr("VN field-generator settings are stored in script metadata only in the current app workflow.")));
        appendLog(stamp(tr("VN init-theta mode is written into script metadata in the current app workflow; no executable runtime flag is passed.")));
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

    if (vnGenerationContext) {
        const QString metadataDir = QFileInfo(options.outputFile).absolutePath().isEmpty()
            ? workingDirEdit->text().trimmed()
            : QFileInfo(options.outputFile).absolutePath();
        if (!metadataDir.trimmed().isEmpty()) {
            const QString metadataPath = QDir(metadataDir).filePath(QStringLiteral("vn_runner_metadata.json"));
            const QJsonObject metadataObject = buildVnMetadataObject(QStringLiteral("generate"),
                                                                    options.outputFile,
                                                                    metadataDir);
            if (WriteJsonFile(metadataPath, metadataObject)) {
                appendLog(stamp(tr("Wrote VN sidecar metadata: %1").arg(metadataPath)));
            } else {
                appendLog(stamp(tr("Warning: failed to write VN sidecar metadata: %1").arg(metadataPath)));
            }
        }
        appendLog(stamp(tr("VN field-generator settings are tracked as metadata/comments only in the current app workflow; no in-app FieldGenerator execution is performed.")));
    }

    appendLog(stamp(tr("Generated %1 starter script: %2").arg(options.modelType, options.outputFile)));
    if (options.modelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0) {
        const QString metadataPath = QDir(workingDirEdit->text().trimmed()).filePath(QStringLiteral("vn_runner_metadata.json"));
        QString metadataError;
        if (writeVnMetadataJson(metadataPath, &metadataError)) {
            appendLog(stamp(tr("Wrote VN metadata JSON: %1").arg(metadataPath)));
        } else if (!metadataError.trimmed().isEmpty()) {
            appendLog(stamp(tr("VN metadata JSON was not written: %1").arg(metadataError)));
        }
    }
    return true;
}

void ModelCreatorWindow::runScript()
{
    const auto buildVnRuntimeMetadataObject = [this](const QString &phase,
                                                     const QString &scriptPath,
                                                     const QString &workingDirectory,
                                                     const QString &executablePath,
                                                     const QStringList &args) {
        QJsonObject meta;
        meta.insert(QStringLiteral("phase"), phase);
        meta.insert(QStringLiteral("model_type"), modelTypeCombo->currentText().trimmed());
        meta.insert(QStringLiteral("init_theta_mode"), vnInitThetaModeCombo->currentData().toString());
        meta.insert(QStringLiteral("field_points"), vnFieldPointsEdit->text().trimmed().isEmpty() ? QStringLiteral("200") : vnFieldPointsEdit->text().trimmed());
        meta.insert(QStringLiteral("field_seed"), vnFieldSeedEdit->text().trimmed().isEmpty() ? QStringLiteral("42") : vnFieldSeedEdit->text().trimmed());
        meta.insert(QStringLiteral("field_dx"), vnFieldDxEdit->text().trimmed().isEmpty() ? QStringLiteral("0.5") : vnFieldDxEdit->text().trimmed());
        meta.insert(QStringLiteral("field_pdf_mode"), vnFieldPdfModeCombo->currentData().toString());
        meta.insert(QStringLiteral("ksat_all"), ksatScaleEdit->text().trimmed().isEmpty() ? QStringLiteral("(blank -> reference preserved)") : ksatScaleEdit->text().trimmed());
        meta.insert(QStringLiteral("ksat_g"), ksatScaleGEdit->text().trimmed().isEmpty() ? QStringLiteral("2.5") : ksatScaleGEdit->text().trimmed());
        meta.insert(QStringLiteral("ksat_uw"), ksatScaleUwEdit->text().trimmed().isEmpty() ? QStringLiteral("35") : ksatScaleUwEdit->text().trimmed());
        meta.insert(QStringLiteral("script_path"), scriptPath);
        meta.insert(QStringLiteral("working_directory"), workingDirectory);
        meta.insert(QStringLiteral("executable_path"), executablePath);
        meta.insert(QStringLiteral("simulation_start"), simulationStartEdit->text().trimmed());
        meta.insert(QStringLiteral("simulation_end"), simulationEndEdit->text().trimmed());
        meta.insert(QStringLiteral("field_generator_runtime_status"), QStringLiteral("metadata_only_in_current_app"));
        meta.insert(QStringLiteral("resultgrid_runtime_status"), vnResultGridRuntimeStatus());
        meta.insert(QStringLiteral("ert_snapshot_runtime_status"), vnErtSnapshotRuntimeStatus());
        QJsonArray argsArray;
        for (const QString &arg : args) {
            argsArray.append(arg);
        }
        meta.insert(QStringLiteral("executable_args"), argsArray);
        return meta;
    };

    if (runner->isRunning()) {
        QMessageBox::information(this, tr("Already running"), tr("A run is already in progress."));
        return;
    }

    QString vnValidationError;
    if (!validateVnAwarenessInputs(&vnValidationError, true)) {
        QMessageBox::warning(this, tr("VN validation"), vnValidationError);
        return;
    }

    const bool vnRunContext =
        modelTypeCombo->currentText().trimmed().compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0;

    if (vnRunContext) {
        if (!vnFieldPointsEdit->text().trimmed().isEmpty() && !IsPositiveIntegerText(vnFieldPointsEdit->text())) {
            QMessageBox::warning(this, tr("Invalid VN field settings"), tr("VN field points must be a positive integer."));
            appendLog(stamp(tr("Run cancelled: VN field points must be a positive integer.")));
            return;
        }
        if (!vnFieldSeedEdit->text().trimmed().isEmpty() && !IsNonNegativeIntegerText(vnFieldSeedEdit->text())) {
            QMessageBox::warning(this, tr("Invalid VN field settings"), tr("VN field seed must be a non-negative integer."));
            appendLog(stamp(tr("Run cancelled: VN field seed must be a non-negative integer.")));
            return;
        }
        if (!vnFieldDxEdit->text().trimmed().isEmpty() && !IsPositiveDoubleText(vnFieldDxEdit->text())) {
            QMessageBox::warning(this, tr("Invalid VN field settings"), tr("VN field dx must be a positive number."));
            appendLog(stamp(tr("Run cancelled: VN field dx must be a positive number.")));
            return;
        }
        if (!ksatScaleEdit->text().trimmed().isEmpty() && !IsPositiveDoubleText(ksatScaleEdit->text())) {
            QMessageBox::warning(this, tr("Invalid Ksat scale"), tr("Ksat all must be positive when provided."));
            appendLog(stamp(tr("Run cancelled: invalid Ksat all value.")));
            return;
        }
        if (!ksatScaleGEdit->text().trimmed().isEmpty() && !IsPositiveDoubleText(ksatScaleGEdit->text())) {
            QMessageBox::warning(this, tr("Invalid Ksat scale"), tr("Ksat g must be positive when provided."));
            appendLog(stamp(tr("Run cancelled: invalid Ksat g value.")));
            return;
        }
        if (!ksatScaleUwEdit->text().trimmed().isEmpty() && !IsPositiveDoubleText(ksatScaleUwEdit->text())) {
            QMessageBox::warning(this, tr("Invalid Ksat scale"), tr("Ksat uw must be positive when provided."));
            appendLog(stamp(tr("Run cancelled: invalid Ksat uw value.")));
            return;
        }
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
                                     tr("No nearby OHQ CLI/internal solver executable was found for:\n%1\n\n"
                                        "GUI fallback is disabled.\n"
                                        "Please select an OHQ CLI/internal solver executable (recommended) "
                                        "or enable 'Allow OpenHydroQual GUI execution fallback'.")
                                        .arg(exeInfo.absoluteFilePath()));
                appendLog(stamp(tr("Run cancelled: GUI executable selected and no CLI/internal solver discovered. GUI fallback is disabled.")));
                return;
            }
            appendLog(stamp(tr("No nearby OHQ CLI/internal solver discovered for '%1'; proceeding with GUI fallback because it is enabled.")
                            .arg(exeInfo.absoluteFilePath())));
        }
    }

    const QString configuredArgsTemplate = exeArgsEdit->text().trimmed();
    const QString normalizedConfiguredArgsTemplate =
        configuredArgsTemplate.compare(QStringLiteral("script"), Qt::CaseInsensitive) == 0
            ? QStringLiteral("{script}")
            : configuredArgsTemplate;
    const QFileInfo executableToRunInfo(executablePathToRun);
    const bool executableLooksLikeCli = LooksLikeCliOhqBinaryName(executableToRunInfo.fileName());
    const bool executableLooksLikeGui = LooksLikeGuiOpenHydroQualExecutable(executableToRunInfo);
    const bool templateReferencesScript =
        normalizedConfiguredArgsTemplate.contains(QStringLiteral("{script}"), Qt::CaseInsensitive)
        || QRegularExpression(QStringLiteral("(^|\\s)script(\\s|$)"),
                              QRegularExpression::CaseInsensitiveOption)
               .match(normalizedConfiguredArgsTemplate)
               .hasMatch();
    const bool noTemplateArgsProvided = normalizedConfiguredArgsTemplate.isEmpty();
    const bool legacyScriptOnlyTemplate = normalizedConfiguredArgsTemplate.compare(QStringLiteral("{script}"), Qt::CaseInsensitive) == 0;
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


    if (vnRunContext) {
        auto validateOptionalPositiveKsat = [this](QLineEdit *edit, const QString &label) -> bool {
            const QString text = edit ? edit->text().trimmed() : QString();
            if (text.isEmpty()) {
                return true;
            }
            bool ok = false;
            const double value = text.toDouble(&ok);
            if (!ok || !std::isfinite(value) || value <= 0.0) {
                QMessageBox::warning(this,
                                     tr("Invalid VN Ksat scale"),
                                     tr("%1 must be blank or a positive number before run.").arg(label));
                return false;
            }
            return true;
        };
        if (!validateOptionalPositiveKsat(ksatScaleEdit, tr("Ksat all"))
            || !validateOptionalPositiveKsat(ksatScaleGEdit, tr("Ksat g"))
            || !validateOptionalPositiveKsat(ksatScaleUwEdit, tr("Ksat uw"))) {
            appendLog(stamp(tr("Run cancelled: invalid VN Ksat scale input.")));
            return;
        }
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

    if (vnRunContext) {
        const QString metadataPath = QDir(wdInfo.absoluteFilePath()).filePath(QStringLiteral("vn_runner_metadata.json"));
        QString metadataError;
        if (writeVnMetadataJson(metadataPath, &metadataError)) {
            appendLog(stamp(tr("Updated VN metadata JSON before run: %1").arg(metadataPath)));
        } else if (!metadataError.trimmed().isEmpty()) {
            appendLog(stamp(tr("VN metadata JSON was not updated before run: %1").arg(metadataError)));
        }
    }

    QStringList jsonPathsToPreserve;
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
            jsonPathsToPreserve << generatedConfigPath;
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
                jsonPathsToPreserve << autoConfigPath;
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
        executableArgs = BuildExecutableArguments(normalizedConfiguredArgsTemplate,
                                                  scriptInfo.absoluteFilePath());
        if (executableLooksLikeGui && legacyScriptOnlyTemplate) {
            executableArgs << QStringLiteral("--run");
            appendLog(stamp(tr("Normalized legacy executable args 'script' to '{script} --run' for OpenHydroQual GUI.")));
        }
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

    if (!passScriptWithRunFlagDefault) {
        QStringList removedJsonConfigs;
        QStringList failedJsonConfigs;
        RemoveStaleRunnerGuiConfigs(wdInfo.absoluteFilePath(), jsonPathsToPreserve, &removedJsonConfigs, &failedJsonConfigs);
        for (const QString &path : removedJsonConfigs) {
            appendLog(stamp(tr("Removed stale GUI JSON config before run: %1").arg(path)));
        }
        for (const QString &path : failedJsonConfigs) {
            appendLog(stamp(tr("Warning: could not remove stale GUI JSON config before run: %1").arg(path)));
        }
    }

    {
        const QString generatedFieldSidecar = defaultVnGeneratedFieldFilePath() + QStringLiteral(".json");
        if (QFileInfo::exists(generatedFieldSidecar) && QFile::remove(generatedFieldSidecar)) {
            appendLog(stamp(tr("Removed stale generated-field sidecar before run: %1").arg(generatedFieldSidecar)));
        }
    }

    QStringList runtimeJsonCandidates;
    runtimeJsonCandidates << QDir(wdInfo.absoluteFilePath()).filePath(QStringLiteral("vn_runner_metadata.json"));
    runtimeJsonCandidates << jsonPathsToPreserve;
    LogRuntimeJsonCandidates(wdInfo.absoluteFilePath(), runtimeJsonCandidates, [this](const QString &line) {
        appendLog(stamp(line));
    });

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
    if (vnRunContext) {
        const QString effectiveAll = ksatScaleEdit->text().trimmed().isEmpty() ? QStringLiteral("(blank -> reference preserved)") : ksatScaleEdit->text().trimmed();
        const QString effectiveG = ksatScaleGEdit->text().trimmed().isEmpty() ? QStringLiteral("2.5") : ksatScaleGEdit->text().trimmed();
        const QString effectiveUw = ksatScaleUwEdit->text().trimmed().isEmpty() ? QStringLiteral("35") : ksatScaleUwEdit->text().trimmed();
        appendLog(stamp(tr("VN runtime metadata: initTheta=%1, field(points=%2, seed=%3, dx=%4, pdf=%5), Ksat(all=%6, g=%7, uw=%8)")
                            .arg(vnInitThetaModeCombo->currentData().toString(),
                                 vnFieldPointsEdit->text().trimmed().isEmpty() ? QStringLiteral("200") : vnFieldPointsEdit->text().trimmed(),
                                 vnFieldSeedEdit->text().trimmed().isEmpty() ? QStringLiteral("42") : vnFieldSeedEdit->text().trimmed(),
                                 vnFieldDxEdit->text().trimmed().isEmpty() ? QStringLiteral("0.5") : vnFieldDxEdit->text().trimmed(),
                                 vnFieldPdfModeCombo->currentData().toString(),
                                 effectiveAll,
                                 effectiveG,
                                 effectiveUw)));
        const QString initThetaMode = vnInitThetaModeCombo->currentData().toString().trimmed();
        if (!initThetaMode.isEmpty() && initThetaMode.compare(QStringLiteral("Default"), Qt::CaseInsensitive) != 0) {
            appendLog(stamp(tr("VN init-theta mode is metadata-only in the current app workflow: %1").arg(initThetaMode)));
        } else {
            appendLog(stamp(tr("VN init-theta mode remains Default metadata in the current app workflow.")));
        }
        appendLog(stamp(tr("VN field-generator settings remain metadata-only in the current app workflow; they are not passed as executable flags.")));
    }
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
    const QString defaultWorkingDirectory = QDir(repoRoot).filePath("Models");
    QDir().mkpath(defaultWorkingDirectory);
    const QString defaultArtifactsDirectory = QDir(defaultWorkingDirectory).filePath("artifacts");
    const QStringList rootCandidates = CandidateOpenHydroQualRoots(repoRoot);
    const QString defaultTemplateDirectory = DetectTemplateDirectory(rootCandidates, defaultWorkingDirectory);
    const QString defaultGeneratedScriptPath = QDir(defaultWorkingDirectory).filePath("starter_generated.ohq");
    const QString defaultExecutablePath = DetectExecutablePath(rootCandidates);
    const QString defaultScriptPath = FirstExistingFile({
        QDir(defaultWorkingDirectory).filePath("hq_drywell.ohq"),
        QDir(defaultWorkingDirectory).filePath("vn_drywell.ohq"),
        QDir(defaultWorkingDirectory).filePath("r_bioswale.ohq"),
        QDir(defaultWorkingDirectory).filePath("JM.ohq"),
        QDir(defaultWorkingDirectory).filePath("examples/hq_drywell.ohq"),
        QDir(defaultWorkingDirectory).filePath("examples/vn_drywell.ohq"),
        QDir(defaultWorkingDirectory).filePath("examples/r_bioswale.ohq"),
        QDir(defaultWorkingDirectory).filePath("examples/JM.ohq")
    });

    modelTypeCombo->setCurrentText(settings.value("modelType", "HQ_Drywell").toString());
    const int workflowIndex = workflowModeCombo->findData(settings.value("workflowMode", "generate").toString());
    workflowModeCombo->setCurrentIndex(workflowIndex >= 0 ? workflowIndex : 0);
    const QString enrichmentPreset = settings.value("enrichmentPreset").toString().trimmed();
    const QString currentModelType = modelTypeCombo->currentText().trimmed();
    QString defaultModePreset;
    if (currentModelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0) {
        defaultModePreset = QStringLiteral("VN_MODE:SoftReference");
    } else if (currentModelType.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0) {
        defaultModePreset = QStringLiteral("HQ_MODE:SoftReference");
    } else if (currentModelType.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0) {
        defaultModePreset = QStringLiteral("R_MODE:SoftReference");
    } else if (currentModelType.compare(QStringLiteral("JM_Bioretention"), Qt::CaseInsensitive) == 0) {
        defaultModePreset = QStringLiteral("JM_MODE:SoftReference");
    }
    int presetIndex = enrichmentPresetCombo->findData(enrichmentPreset);
    if (presetIndex < 0 && !defaultModePreset.isEmpty()) {
        presetIndex = enrichmentPresetCombo->findData(defaultModePreset);
    }
    enrichmentPresetCombo->setCurrentIndex(presetIndex >= 0 ? presetIndex : 0);
    exePathEdit->setText(settings.value("ohqExecutable", defaultExecutablePath).toString());
    exeArgsEdit->setText(settings.value("ohqExecutableArgs", QStringLiteral("{script}")).toString());
    guiConfigTemplateEdit->setText(settings.value("guiConfigTemplate").toString());
    scriptPathEdit->setText(settings.value("ohqScript", defaultScriptPath).toString());
    workingDirEdit->setText(settings.value("workingDirectory", defaultWorkingDirectory).toString());
    artifactsDirEdit->setText(settings.value("artifactsDirectory", defaultArtifactsDirectory).toString());
    templateDirEdit->setText(settings.value("templateDirectory",
                                            defaultTemplateDirectory.isEmpty() ? QDir(defaultWorkingDirectory).filePath("templates")
                                                                               : defaultTemplateDirectory).toString());
    generatedScriptEdit->setText(settings.value("generatedScriptPath", defaultGeneratedScriptPath).toString());
    inflowFileEdit->setText(settings.value("inflowFile").toString());
    if (inflowUseButton) {
        const bool useInflow = settings.value("useInflowFile", true).toBool();
        inflowUseButton->setChecked(useInflow);
        inflowUseButton->setText(useInflow ? tr("Use") : tr("No file"));
        inflowFileEdit->setEnabled(useInflow);
    }
    simulationStartEdit->setText(settings.value("simulationStart", "44435").toString());
    simulationEndEdit->setText(settings.value("simulationEnd", "44438").toString());
    ksatScaleEdit->setText(settings.value("ksatScale").toString());
    ksatScaleGEdit->setText(settings.value("ksatScaleG").toString());
    ksatScaleUwEdit->setText(settings.value("ksatScaleUw").toString());
    outputSeriesFileEdit->setText(settings.value("outputSeriesFile",
                                                  QDir(defaultWorkingDirectory).filePath("OHQ_output.txt")).toString());
    observationFileEdit->setText(settings.value("observationFile").toString());
    depthProfileFileEdit->setText(settings.value("depthProfileFile").toString());
    vnBaseOhqFileEdit->clear();
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
    hqSoftRadialCellsEdit->setText(settingTextOrDefault("hqSoftRadialCells", "10"));
    {
        const QString savedHqLayers = settings.value("hqSoftShallowLayers").toString().trimmed();
        // Older builds saved "1" as a placeholder. Blank means: use the HQ reference layer count.
        hqSoftShallowLayersEdit->setText((savedHqLayers.isEmpty() || savedHqLayers == QStringLiteral("1")) ? QStringLiteral("34") : savedHqLayers);
    }
    hqSoftWellDepthEdit->setText(settingTextOrDefault("hqSoftWellDepth", "20"));
    hqSoftWellRadiusEdit->setText(settingTextOrDefault("hqSoftWellRadius", "0.381"));
    hqSoftPondRadiusEdit->setText(settingTextOrDefault("hqSoftPondRadius", "6"));
    hqSoftSurfaceElevationEdit->setText(settingTextOrDefault("hqSoftSurfaceElevation", "140"));
    hqSoilPropsFileEdit->setText(settings.value("hqSoilPropsFile").toString());
    rBioSwaleWidthEdit->setText(settingTextOrDefault("rBioSwaleWidth", "0.6096"));
    rSystemWidthEdit->setText(settingTextOrDefault("rSystemWidth", "3"));
    rBioSwaleDepthEdit->setText(settingTextOrDefault("rBioSwaleDepth", "0.9144"));
    rSoilPropsFileEdit->setText(settings.value("rSoilPropsFile").toString());
    rLateralCellsEdit->setText(settingTextOrDefault("rLateralCells", "6"));
    rLengthEdit->setText(settingTextOrDefault("rLength", "8"));
    rStreetWidthEdit->setText(settingTextOrDefault("rStreetWidth", "5"));
    rStreetCellsEdit->setText(settingTextOrDefault("rStreetCells", "10"));
    rVerticalLayersEdit->setText(settingTextOrDefault("rVerticalLayers", ""));
    rEngineeredSoilNzEdit->setText(settingTextOrDefault("rEngineeredSoilNz", ""));
    rNativeSoilNzEdit->setText(settingTextOrDefault("rNativeSoilNz", ""));
    rAnisoRatioEdit->setText(settingTextOrDefault("rAnisoRatio", "5"));
    jmNativeHorizontalCellsEdit->setText(settingTextOrDefault("jmNativeHorizontalCells", "4"));
    jmNativeVerticalLayersEdit->setText(settingTextOrDefault("jmNativeVerticalLayers", "3"));
    const QString vnInitThetaMode = settingTextOrDefault("vnInitThetaMode", "Default");
    const int vnInitThetaModeIndex = vnInitThetaModeCombo->findData(vnInitThetaMode);
    vnInitThetaModeCombo->setCurrentIndex(vnInitThetaModeIndex >= 0 ? vnInitThetaModeIndex : 0);
    vnFieldPointsEdit->setText(settingTextOrDefault("vnFieldPoints", "200"));
    vnFieldSeedEdit->setText(settingTextOrDefault("vnFieldSeed", "42"));
    vnFieldDxEdit->setText(settingTextOrDefault("vnFieldDx", "0.5"));
    vnSoilProfileExportEdit->setText(settings.value("vnSoilProfileExportPath", QDir(defaultWorkingDirectory).filePath("vn_soil_profile.csv")).toString());
    vnDepthSliceExportEdit->setText(settings.value("vnDepthSliceExportPath", QDir(defaultWorkingDirectory).filePath("vn_depth_slice.csv")).toString());
    vnErtSnapshotExportEdit->setText(settings.value("vnErtSnapshotExportPath", QDir(defaultWorkingDirectory).filePath("vn_ert_snapshot.csv")).toString());
    vtkInventoryExportEdit->setText(settings.value("vtkInventoryExportPath", QDir(defaultWorkingDirectory).filePath("vtk_inventory.csv")).toString());
    const QString vnFieldPdfMode = settingTextOrDefault("vnFieldPdfMode", "parametric");
    const int vnFieldPdfModeIndex = vnFieldPdfModeCombo->findData(vnFieldPdfMode);
    vnFieldPdfModeCombo->setCurrentIndex(vnFieldPdfModeIndex >= 0 ? vnFieldPdfModeIndex : 0);
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

    const QString modelType = modelTypeCombo->currentText().trimmed();
    const QString templateDirectory = templateDirEdit->text().trimmed();
    const QString suggestedInflowPath = DetectSuggestedInflowFile(modelType, templateDirectory);
    const auto markAutoSuggestedFromValue = [](QLineEdit *edit, const QString &suggested) {
        const QString current = edit->text().trimmed();
        const bool isAuto = !current.isEmpty()
            && !suggested.trimmed().isEmpty()
            && current.compare(suggested.trimmed(), Qt::CaseInsensitive) == 0;
        SetAutoSuggestedField(edit, isAuto);
    };
    markAutoSuggestedFromValue(exePathEdit, defaultExecutablePath);
    markAutoSuggestedFromValue(exeArgsEdit, QStringLiteral("{script}"));
    markAutoSuggestedFromValue(scriptPathEdit, defaultScriptPath);
    markAutoSuggestedFromValue(workingDirEdit, defaultWorkingDirectory);
    markAutoSuggestedFromValue(artifactsDirEdit, defaultArtifactsDirectory);
    markAutoSuggestedFromValue(templateDirEdit, defaultTemplateDirectory.isEmpty()
                                                   ? QDir(defaultWorkingDirectory).filePath("templates")
                                                   : defaultTemplateDirectory);
    markAutoSuggestedFromValue(generatedScriptEdit, defaultGeneratedScriptPath);
    markAutoSuggestedFromValue(inflowFileEdit, suggestedInflowPath);
    markAutoSuggestedFromValue(outputSeriesFileEdit,
                               QDir(defaultWorkingDirectory).filePath(QStringLiteral("OHQ_output.txt")));
    inflowAutoSuggested = IsAutoSuggestedField(inflowFileEdit);

    const QString currentStart = simulationStartEdit->text().trimmed();
    const QString currentEnd = simulationEndEdit->text().trimmed();
    simulationWindowAutoSuggested = (currentStart == QStringLiteral("44435") && currentEnd == QStringLiteral("44438"));
    lastSelectedModelType = modelTypeCombo->currentText().trimmed();
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
    settings.setValue("useInflowFile", inflowUseButton ? inflowUseButton->isChecked() : true);
    settings.setValue("simulationStart", simulationStartEdit->text());
    settings.setValue("simulationEnd", simulationEndEdit->text());
    settings.setValue("ksatScale", ksatScaleEdit->text());
    settings.setValue("ksatScaleG", ksatScaleGEdit->text());
    settings.setValue("ksatScaleUw", ksatScaleUwEdit->text());
    settings.setValue("outputSeriesFile", outputSeriesFileEdit->text());
    settings.setValue("observationFile", observationFileEdit->text());
    settings.setValue("depthProfileFile", depthProfileFileEdit->text());
    settings.setValue("vnBaseOhqFile", QString());
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
    settings.setValue("hqSoftRadialCells", hqSoftRadialCellsEdit->text());
    settings.setValue("hqSoftShallowLayers", hqSoftShallowLayersEdit->text());
    settings.setValue("hqSoftWellDepth", hqSoftWellDepthEdit->text());
    settings.setValue("hqSoftWellRadius", hqSoftWellRadiusEdit->text());
    settings.setValue("hqSoftPondRadius", hqSoftPondRadiusEdit->text());
    settings.setValue("hqSoftSurfaceElevation", hqSoftSurfaceElevationEdit->text());
    settings.setValue("hqSoilPropsFile", hqSoilPropsFileEdit->text());
    settings.setValue("rBioSwaleWidth", rBioSwaleWidthEdit->text());
    settings.setValue("rSystemWidth", rSystemWidthEdit->text());
    settings.setValue("rBioSwaleDepth", rBioSwaleDepthEdit->text());
    settings.setValue("rSoilPropsFile", rSoilPropsFileEdit->text());
    settings.setValue("rLateralCells", rLateralCellsEdit->text());
    settings.setValue("rLength", rLengthEdit->text());
    settings.setValue("rStreetWidth", rStreetWidthEdit->text());
    settings.setValue("rStreetCells", rStreetCellsEdit->text());
    settings.setValue("rVerticalLayers", rVerticalLayersEdit->text());
    settings.setValue("rEngineeredSoilNz", rEngineeredSoilNzEdit->text());
    settings.setValue("rNativeSoilNz", rNativeSoilNzEdit->text());
    settings.setValue("rAnisoRatio", rAnisoRatioEdit->text());
    settings.setValue("jmNativeHorizontalCells", jmNativeHorizontalCellsEdit->text());
    settings.setValue("jmNativeVerticalLayers", jmNativeVerticalLayersEdit->text());
    settings.setValue("vnInitThetaMode", vnInitThetaModeCombo->currentData().toString());
    settings.setValue("vnFieldPoints", vnFieldPointsEdit->text());
    settings.setValue("vnFieldSeed", vnFieldSeedEdit->text());
    settings.setValue("vnFieldDx", vnFieldDxEdit->text());
    settings.setValue("vnSoilProfileExportPath", vnSoilProfileExportEdit->text());
    settings.setValue("vnDepthSliceExportPath", vnDepthSliceExportEdit->text());
    settings.setValue("vnErtSnapshotExportPath", vnErtSnapshotExportEdit->text());
    settings.setValue("vtkInventoryExportPath", vtkInventoryExportEdit->text());
    settings.setValue("vnFieldPdfMode", vnFieldPdfModeCombo->currentData().toString());
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

void ModelCreatorWindow::updateVnRuntimeStatusFromArtifacts(const QStringList &artifacts)
{
    if (modelTypeCombo->currentText().trimmed().compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) != 0) {
        return;
    }
    const QString workingDir = workingDirEdit->text().trimmed();
    const auto toAbsolutePath = [&workingDir](const QString &candidate) {
        const QFileInfo info(candidate);
        if (info.isAbsolute()) {
            return info.absoluteFilePath();
        }
        return QDir(workingDir).filePath(candidate);
    };
    const auto normalizePath = [](const QString &path) {
        return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
    };

    QSet<QString> normalizedArtifacts;
    bool hasVtkArtifact = false;
    bool hasPvdArtifact = false;
    for (const QString &path : artifacts) {
        normalizedArtifacts.insert(normalizePath(path));
        const QString ext = QFileInfo(path).suffix().toLower();
        if (ext == QStringLiteral("vtk") || ext == QStringLiteral("vtp") || ext == QStringLiteral("vtu")
            || ext == QStringLiteral("vti") || ext == QStringLiteral("vtm") || ext == QStringLiteral("vtmb")) {
            hasVtkArtifact = true;
        } else if (ext == QStringLiteral("pvd")) {
            hasPvdArtifact = true;
        }
    }

    if (hasVtkArtifact || hasPvdArtifact) {
        vnResultGridStatus = hasPvdArtifact
            ? QStringLiteral("detected_vtk_and_pvd_artifacts")
            : QStringLiteral("detected_vtk_artifacts");
    }

    const QString configuredOutputSeries = outputSeriesFileEdit->text().trimmed();
    if (!configuredOutputSeries.isEmpty()) {
        const QString outputSeriesPath = normalizePath(toAbsolutePath(configuredOutputSeries));
        if (normalizedArtifacts.contains(outputSeriesPath) && !hasVtkArtifact && !hasPvdArtifact) {
            vnResultGridStatus = QStringLiteral("detected_output_series_in_run_artifacts");
        }
    }

    QString configuredErtPath = vnErtSnapshotExportEdit->text().trimmed();
    if (configuredErtPath.isEmpty() && !workingDir.isEmpty()) {
        configuredErtPath = QDir(workingDir).filePath(QStringLiteral("vn_ert_snapshot.csv"));
    }
    if (!configuredErtPath.isEmpty()) {
        const QString ertPath = normalizePath(toAbsolutePath(configuredErtPath));
        if (normalizedArtifacts.contains(ertPath)) {
            vnErtSnapshotStatus = QStringLiteral("detected_ert_snapshot_in_run_artifacts");
        }
    }
}

QString ModelCreatorWindow::vnResultGridRuntimeStatus() const
{
    return vnResultGridStatus.trimmed().isEmpty()
        ? QStringLiteral("not_run_in_current_app")
        : vnResultGridStatus.trimmed();
}

QString ModelCreatorWindow::vnErtSnapshotRuntimeStatus() const
{
    return vnErtSnapshotStatus.trimmed().isEmpty()
        ? QStringLiteral("not_run_in_current_app")
        : vnErtSnapshotStatus.trimmed();
}


QString ModelCreatorWindow::currentEffectiveVnInitTheta() const
{
    return vnInitThetaModeCombo->currentData().toString().trimmed().isEmpty()
        ? QStringLiteral("Default")
        : vnInitThetaModeCombo->currentData().toString().trimmed();
}

QString ModelCreatorWindow::currentEffectiveVnFieldMode() const
{
    return QStringLiteral("CurrentAppProfile");
}

QString ModelCreatorWindow::currentEffectiveVnFieldPoints() const
{
    return vnFieldPointsEdit->text().trimmed().isEmpty() ? QStringLiteral("200") : vnFieldPointsEdit->text().trimmed();
}

QString ModelCreatorWindow::currentEffectiveVnFieldSeed() const
{
    return vnFieldSeedEdit->text().trimmed().isEmpty() ? QStringLiteral("42") : vnFieldSeedEdit->text().trimmed();
}

QString ModelCreatorWindow::currentEffectiveVnFieldDx() const
{
    return vnFieldDxEdit->text().trimmed().isEmpty() ? QStringLiteral("0.5") : vnFieldDxEdit->text().trimmed();
}

QString ModelCreatorWindow::currentEffectiveVnFieldPdf() const
{
    return vnFieldPdfModeCombo->currentData().toString().trimmed().isEmpty()
        ? QStringLiteral("parametric")
        : vnFieldPdfModeCombo->currentData().toString().trimmed();
}

QString ModelCreatorWindow::currentEffectiveKsatAll() const
{
    return ksatScaleEdit->text().trimmed().isEmpty() ? QStringLiteral("(blank -> reference preserved)") : ksatScaleEdit->text().trimmed();
}

QString ModelCreatorWindow::currentEffectiveKsatG() const
{
    return ksatScaleGEdit->text().trimmed().isEmpty() ? QStringLiteral("2.5") : ksatScaleGEdit->text().trimmed();
}

QString ModelCreatorWindow::currentEffectiveKsatUw() const
{
    return ksatScaleUwEdit->text().trimmed().isEmpty() ? QStringLiteral("35") : ksatScaleUwEdit->text().trimmed();
}

bool ModelCreatorWindow::validateVnAwarenessInputs(QString *errorMessage, bool forRun) const
{
    const bool vnContext = modelTypeCombo->currentText().trimmed().compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0;
    if (!vnContext) {
        return true;
    }

    bool ok = false;
    const int points = currentEffectiveVnFieldPoints().toInt(&ok);
    if (!ok || points <= 0) {
        if (errorMessage) *errorMessage = tr("VN field points must be a positive integer.");
        return false;
    }
    const int seed = currentEffectiveVnFieldSeed().toInt(&ok);
    if (!ok || seed < 0) {
        if (errorMessage) *errorMessage = tr("VN field seed must be a non-negative integer.");
        return false;
    }
    const double dx = currentEffectiveVnFieldDx().toDouble(&ok);
    if (!ok || !std::isfinite(dx) || dx <= 0.0) {
        if (errorMessage) *errorMessage = tr("VN field dx must be a positive number.");
        return false;
    }
    const auto validatePositiveOptional = [&](const QLineEdit *edit, const QString &label) {
        if (!edit || edit->text().trimmed().isEmpty()) return true;
        bool localOk = false;
        const double v = edit->text().trimmed().toDouble(&localOk);
        if (!localOk || !std::isfinite(v) || v <= 0.0) {
            if (errorMessage) *errorMessage = tr("%1 must be a positive number when provided.").arg(label);
            return false;
        }
        return true;
    };
    if (!validatePositiveOptional(ksatScaleEdit, tr("Ksat all"))
        || !validatePositiveOptional(ksatScaleGEdit, tr("Ksat g"))
        || !validatePositiveOptional(ksatScaleUwEdit, tr("Ksat uw"))) {
        return false;
    }
    const QString mode = vnBuildModeCombo->currentData().toString().trimmed();
    if (mode.compare(QStringLiteral("LoadFromOhq"), Qt::CaseInsensitive) == 0
        && vnBaseOhqFileEdit->text().trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = tr("VN LoadFromOhq mode requires a base .ohq file.");
        return false;
    }
    const QString soilMode = vnSoftSoilParamModeCombo->currentData().toString().trimmed();
    if (soilMode.compare(QStringLiteral("File"), Qt::CaseInsensitive) == 0
        && vnSoftSoilParameterFileEdit->text().trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = tr("VN soil param mode 'File' requires a soil-parameter profile file.");
        return false;
    }
    if (forRun) {
        const QString scriptPath = scriptPathEdit->text().trimmed();
        if (scriptPath.isEmpty()) {
            if (errorMessage) *errorMessage = tr("Select or generate an OHQ script before running VN-aware tools.");
            return false;
        }
    }
    return true;
}

bool ModelCreatorWindow::writeVnMetadataJson(const QString &targetPath, QString *errorMessage) const
{
    const bool vnContext = modelTypeCombo->currentText().trimmed().compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0;
    if (!vnContext) {
        if (errorMessage) *errorMessage = tr("Current model is not VN_Drywell.");
        return false;
    }

    QString validationError;
    if (!validateVnAwarenessInputs(&validationError, false)) {
        if (errorMessage) *errorMessage = validationError;
        return false;
    }

    QJsonObject root;
    root.insert(QStringLiteral("model_type"), modelTypeCombo->currentText().trimmed());
    root.insert(QStringLiteral("workflow_mode"), workflowModeCombo->currentData().toString());
    root.insert(QStringLiteral("vn_build_mode"), vnBuildModeCombo->currentData().toString());
    root.insert(QStringLiteral("init_theta_mode"), currentEffectiveVnInitTheta());
    root.insert(QStringLiteral("field_points"), currentEffectiveVnFieldPoints());
    root.insert(QStringLiteral("field_seed"), currentEffectiveVnFieldSeed());
    root.insert(QStringLiteral("field_dx"), currentEffectiveVnFieldDx());
    root.insert(QStringLiteral("field_pdf_mode"), currentEffectiveVnFieldPdf());
    root.insert(QStringLiteral("ksat_all"), currentEffectiveKsatAll());
    root.insert(QStringLiteral("ksat_g"), currentEffectiveKsatG());
    root.insert(QStringLiteral("ksat_uw"), currentEffectiveKsatUw());
    root.insert(QStringLiteral("simulation_start"), simulationStartEdit->text().trimmed());
    root.insert(QStringLiteral("simulation_end"), simulationEndEdit->text().trimmed());
    root.insert(QStringLiteral("use_inflow_file"), inflowUseButton ? inflowUseButton->isChecked() : true);
    root.insert(QStringLiteral("inflow_file"), inflowUseButton && !inflowUseButton->isChecked() ? QString() : inflowFileEdit->text().trimmed());
    root.insert(QStringLiteral("script_path"), scriptPathEdit->text().trimmed());
    root.insert(QStringLiteral("working_directory"), workingDirEdit->text().trimmed());
    root.insert(QStringLiteral("field_generator_runtime"), QStringLiteral("metadata_only_in_current_app"));
    root.insert(QStringLiteral("resultgrid_runtime"), vnResultGridRuntimeStatus());
    root.insert(QStringLiteral("ert_snapshot_runtime"), vnErtSnapshotRuntimeStatus());
    root.insert(QStringLiteral("written_utc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    QSaveFile out(targetPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) *errorMessage = tr("Could not open VN metadata JSON for writing: %1").arg(targetPath);
        return false;
    }
    out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!out.commit()) {
        if (errorMessage) *errorMessage = tr("Could not finalize VN metadata JSON: %1").arg(targetPath);
        return false;
    }
    return true;
}

QString ModelCreatorWindow::defaultVnGeneratedFieldFilePath() const
{
    const QString workDir = workingDirEdit->text().trimmed();
    if (!workDir.isEmpty()) {
        return QDir(workDir).filePath(QStringLiteral("vn_generated_field_profile.csv"));
    }
    return QStringLiteral("vn_generated_field_profile.csv");
}

void ModelCreatorWindow::syncVnToolDefaultPaths()
{
    const QString workDir = workingDirEdit->text().trimmed();
    if (workDir.isEmpty()) {
        return;
    }

    const QString soilDefault = QDir(workDir).filePath(QStringLiteral("vn_soil_profile.csv"));
    const QString outputDefault = QDir(workDir).filePath(QStringLiteral("vn_depth_slice.csv"));
    const QString ertDefault = QDir(workDir).filePath(QStringLiteral("vn_ert_snapshot.csv"));
    const QString vtkDefault = QDir(workDir).filePath(QStringLiteral("vtk_inventory.csv"));

    const auto shouldReplaceWithDefault = [](const QString &currentValue, const QString &defaultFileName) {
        const QString trimmed = currentValue.trimmed();
        if (trimmed.isEmpty()) {
            return true;
        }
        const QFileInfo info(trimmed);
        return info.fileName().compare(defaultFileName, Qt::CaseInsensitive) == 0;
    };

    if (shouldReplaceWithDefault(vnSoilProfileExportEdit->text(), QStringLiteral("vn_soil_profile.csv"))) {
        vnSoilProfileExportEdit->setText(soilDefault);
    }
    if (shouldReplaceWithDefault(vnDepthSliceExportEdit->text(), QStringLiteral("vn_depth_slice.csv"))) {
        vnDepthSliceExportEdit->setText(outputDefault);
    }
    if (shouldReplaceWithDefault(vnErtSnapshotExportEdit->text(), QStringLiteral("vn_ert_snapshot.csv"))) {
        vnErtSnapshotExportEdit->setText(ertDefault);
    }
    if (shouldReplaceWithDefault(vtkInventoryExportEdit->text(), QStringLiteral("vtk_inventory.csv"))) {
        vtkInventoryExportEdit->setText(vtkDefault);
    }
}

bool ModelCreatorWindow::writeVnGeneratedFieldFile(const QString &targetPath, QString *errorMessage) const
{
    const QString target = targetPath.trimmed();
    if (target.isEmpty()) {
        if (errorMessage) *errorMessage = tr("Target field file path is empty.");
        return false;
    }

    bool ok = false;
    const int pointCount = currentEffectiveVnFieldPoints().trimmed().toInt(&ok);
    const int safePointCount = ok && pointCount > 0 ? pointCount : 200;
    const double dx = currentEffectiveVnFieldDx().trimmed().toDouble(&ok);
    const double safeDx = ok && dx > 0.0 ? dx : 0.5;

    const int nzG = vnSoftGridYEdit->text().trimmed().toInt(&ok);
    const int safeNzG = ok && nzG > 0 ? nzG : 15;
    const int nzUw = vnSoftUwGridYEdit->text().trimmed().toInt(&ok);
    const int safeNzUw = ok && nzUw > 0 ? nzUw : 12;
    const double top = vnSoftTopElevationEdit->text().trimmed().toDouble(&ok);
    const double safeTop = ok ? top : -5.0;
    const double dz = vnSoftLayerThicknessEdit->text().trimmed().toDouble(&ok);
    const double safeDz = ok && dz > 0.0 ? dz : 1.0;

    const QString ksat = vnSoftSoilKsatOriginalEdit->text().trimmed().isEmpty() ? QStringLiteral("1.05196") : vnSoftSoilKsatOriginalEdit->text().trimmed();
    const QString alpha = vnSoftSoilAlphaEdit->text().trimmed().isEmpty() ? QStringLiteral("3.47536") : vnSoftSoilAlphaEdit->text().trimmed();
    const QString n = vnSoftSoilNEdit->text().trimmed().isEmpty() ? QStringLiteral("1.74582") : vnSoftSoilNEdit->text().trimmed();
    const QString thetaSat = vnSoftSoilThetaSatEdit->text().trimmed().isEmpty() ? QStringLiteral("0.39") : vnSoftSoilThetaSatEdit->text().trimmed();
    const QString thetaRes = vnSoftSoilThetaResEdit->text().trimmed().isEmpty() ? QStringLiteral("0.049") : vnSoftSoilThetaResEdit->text().trimmed();

    QSaveFile out(target);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) *errorMessage = tr("Could not open generated field file for writing: %1").arg(target);
        return false;
    }

    QTextStream ts(&out);
    ts << "point_index,z,depth_m,zone,Ksat,alpha,n,theta_sat,theta_res,dx_m,field_mode,pdf_mode\n";
    for (int i = 0; i < safePointCount; ++i) {
        const double z = i * safeDx;
        const int layerIndex = static_cast<int>(std::floor(z / safeDz));
        const bool inG = (layerIndex < safeNzG);
        const bool inUw = (!inG && layerIndex < safeNzG + safeNzUw);
        const QString zone = inG ? QStringLiteral("Soil-g") : (inUw ? QStringLiteral("Soil-uw") : QStringLiteral("BelowProfile"));
        const double actY = safeTop - (z + 0.5 * safeDz);
        const double depthM = -actY;
        ts << i << ','
           << z << ','
           << depthM << ','
           << zone << ','
           << ksat << ','
           << alpha << ','
           << n << ','
           << thetaSat << ','
           << thetaRes << ','
           << safeDx << ','
           << currentEffectiveVnFieldMode() << ','
           << currentEffectiveVnFieldPdf() << "\n";
    }
    if (!out.commit()) {
        if (errorMessage) *errorMessage = tr("Could not finalize generated field file: %1").arg(target);
        return false;
    }
    const QString sidecarPath = target + QStringLiteral(".json");
    QJsonObject meta;
    meta.insert(QStringLiteral("kind"), QStringLiteral("vn_generated_field_profile"));
    meta.insert(QStringLiteral("field_mode"), currentEffectiveVnFieldMode());
    meta.insert(QStringLiteral("pdf_mode"), currentEffectiveVnFieldPdf());
    meta.insert(QStringLiteral("points"), currentEffectiveVnFieldPoints());
    meta.insert(QStringLiteral("seed"), currentEffectiveVnFieldSeed());
    meta.insert(QStringLiteral("dx"), currentEffectiveVnFieldDx());
    meta.insert(QStringLiteral("source_build_mode"), vnBuildModeCombo->currentData().toString());
    meta.insert(QStringLiteral("source_soil_param_mode"), vnSoftSoilParamModeCombo->currentData().toString());
    meta.insert(QStringLiteral("written_utc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    WriteJsonFile(sidecarPath, meta);

    return true;
}

void ModelCreatorWindow::saveVnGeneratedFieldFile()
{
    QString validationError;
    if (!validateVnAwarenessInputs(&validationError, false)) {
        QMessageBox::warning(this, tr("Save field file"), validationError);
        return;
    }

    const QString target = defaultVnGeneratedFieldFilePath();
    QString error;
    if (!writeVnGeneratedFieldFile(target, &error)) {
        QMessageBox::warning(this, tr("Save field file"), error);
        return;
    }

    vnSoilProfileExportEdit->setText(target);
    appendLog(stamp(tr("Saved generated VN field file: %1").arg(target)));
    appendLog(stamp(tr("This file is a current-app field-style export from UI settings, not full in-app FieldGenerator execution.")));
    saveSettings();
}

void ModelCreatorWindow::useVnGeneratedFieldFile()
{
    const QString target = defaultVnGeneratedFieldFilePath();
    if (!QFileInfo::exists(target)) {
        QString error;
        if (!writeVnGeneratedFieldFile(target, &error)) {
            QMessageBox::warning(this, tr("Use field file"), error);
            return;
        }
        appendLog(stamp(tr("Saved generated VN field file: %1").arg(target)));
    }

    vnSoftSoilParamModeCombo->setCurrentIndex(qMax(0, vnSoftSoilParamModeCombo->findData(QStringLiteral("File"))));
    vnSoftSoilParameterFileEdit->setText(target);
    vnSoilProfileExportEdit->setText(target);
    appendLog(stamp(tr("Using generated VN field file as soil-parameter profile: %1").arg(target)));
    saveSettings();
    updateFieldVisibilityForContext();
}

void ModelCreatorWindow::exportVnMetadataJson()
{
    const QString suggested = vnDepthSliceExportEdit->text().trimmed().isEmpty()
        ? QDir(workingDirEdit->text().trimmed()).filePath(QStringLiteral("vn_runner_metadata.json"))
        : QDir(QFileInfo(vnDepthSliceExportEdit->text().trimmed()).absolutePath()).filePath(QStringLiteral("vn_runner_metadata.json"));
    const QString target = QFileDialog::getSaveFileName(this, tr("Save VN metadata JSON"), suggested, tr("JSON files (*.json);;All files (*.*)"));
    if (target.isEmpty()) {
        return;
    }
    QString error;
    if (!writeVnMetadataJson(target, &error)) {
        QMessageBox::warning(this, tr("Export VN metadata JSON"), error);
        return;
    }
    appendLog(stamp(tr("Exported VN metadata JSON: %1").arg(target)));
}

void ModelCreatorWindow::exportVnSoilProfileCsv()
{
    QString validationError;
    if (!validateVnAwarenessInputs(&validationError, false)) {
        QMessageBox::warning(this, tr("Export VN soil profile"), validationError);
        return;
    }

    QString target = vnSoilProfileExportEdit->text().trimmed();
    if (target.isEmpty()) {
        target = QFileDialog::getSaveFileName(this,
                                              tr("Save VN soil profile CSV"),
                                              QDir(workingDirEdit->text().trimmed()).filePath(QStringLiteral("vn_soil_profile.csv")),
                                              tr("CSV files (*.csv);;All files (*.*)"));
        if (target.isEmpty()) return;
        vnSoilProfileExportEdit->setText(target);
    }

    if (vnSoftSoilParamModeCombo->currentData().toString().trimmed().compare(QStringLiteral("File"), Qt::CaseInsensitive) == 0) {
        QFile::remove(target);
        if (!QFile::copy(vnSoftSoilParameterFileEdit->text().trimmed(), target)) {
            QMessageBox::warning(this, tr("Export VN soil profile"), tr("Could not copy VN soil parameter profile to target path."));
            return;
        }
        appendLog(stamp(tr("Copied VN soil profile file: %1").arg(target)));
        saveSettings();
        return;
    }

    bool ok = false;
    const int nzG = vnSoftGridYEdit->text().trimmed().toInt(&ok);
    const int safeNzG = ok && nzG > 0 ? nzG : 15;
    const int nzUw = vnSoftUwGridYEdit->text().trimmed().toInt(&ok);
    const int safeNzUw = ok && nzUw > 0 ? nzUw : 12;
    const double top = vnSoftTopElevationEdit->text().trimmed().toDouble(&ok);
    const double safeTop = ok ? top : -5.0;
    const double dz = vnSoftLayerThicknessEdit->text().trimmed().toDouble(&ok);
    const double safeDz = ok && dz > 0.0 ? dz : 1.0;
    const QString ksat = vnSoftSoilKsatOriginalEdit->text().trimmed().isEmpty() ? QStringLiteral("1.05196") : vnSoftSoilKsatOriginalEdit->text().trimmed();
    const QString alpha = vnSoftSoilAlphaEdit->text().trimmed().isEmpty() ? QStringLiteral("3.47536") : vnSoftSoilAlphaEdit->text().trimmed();
    const QString n = vnSoftSoilNEdit->text().trimmed().isEmpty() ? QStringLiteral("1.74582") : vnSoftSoilNEdit->text().trimmed();
    const QString thetaSat = vnSoftSoilThetaSatEdit->text().trimmed().isEmpty() ? QStringLiteral("0.39") : vnSoftSoilThetaSatEdit->text().trimmed();
    const QString thetaRes = vnSoftSoilThetaResEdit->text().trimmed().isEmpty() ? QStringLiteral("0.049") : vnSoftSoilThetaResEdit->text().trimmed();

    QSaveFile out(target);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export VN soil profile"), tr("Could not open target CSV for writing."));
        return;
    }
    QTextStream ts(&out);
    ts << "zone,act_Y,depth_m,Ksat,alpha,n,theta_sat,theta_res\n";
    for (int j = 0; j < safeNzG; ++j) {
        const double actY = safeTop - (j + 0.5) * safeDz;
        ts << "Soil-g," << actY << ',' << -actY << ',' << ksat << ',' << alpha << ',' << n << ',' << thetaSat << ',' << thetaRes << "\n";
    }
    for (int j = 0; j < safeNzUw; ++j) {
        const double actY = safeTop - (safeNzG + j + 0.5) * safeDz;
        ts << "Soil-uw," << actY << ',' << -actY << ',' << ksat << ',' << alpha << ',' << n << ',' << thetaSat << ',' << thetaRes << "\n";
    }
    if (!out.commit()) {
        QMessageBox::warning(this, tr("Export VN soil profile"), tr("Could not finalize soil profile CSV."));
        return;
    }
    appendLog(stamp(tr("Exported VN soil profile CSV: %1").arg(target)));
    saveSettings();
}

void ModelCreatorWindow::exportVnDepthSliceCsv()
{
    QString validationError;
    if (!validateVnAwarenessInputs(&validationError, false)) {
        QMessageBox::warning(this, tr("Export VN depth slice"), validationError);
        return;
    }


    QString error;
    if (!loadOutputColumns(&error)) {
        QMessageBox::warning(this, tr("Export VN depth slice"), error);
        return;
    }

    const int xIdx = outputXAxisCombo->currentIndex();
    const int yIdx = outputYAxisCombo->currentIndex();
    const int depthIdx = depthColumnCombo->currentIndex();
    if (xIdx < 0 || yIdx < 0 || depthIdx < 0) {
        QMessageBox::warning(this, tr("Export VN depth slice"), tr("Select valid X/Y/Depth columns first."));
        return;
    }

    bool ok = false;
    double targetX = sliceXEdit->text().trimmed().toDouble(&ok);
    if (!ok) {
        const QVector<double> &xCol = outputNumericColumns[xIdx];
        if (xCol.isEmpty()) {
            QMessageBox::warning(this, tr("Export VN depth slice"), tr("X column is empty."));
            return;
        }
        double minX = xCol.first();
        double maxX = xCol.first();
        for (double x : xCol) { minX = qMin(minX, x); maxX = qMax(maxX, x); }
        targetX = 0.5 * (minX + maxX);
        sliceXEdit->setText(QString::number(targetX, 'g', 6));
    }

    const QVector<QPointF> series = computeDepthSliceSeries(targetX, xIdx, yIdx, depthIdx);
    if (series.isEmpty()) {
        QMessageBox::information(this, tr("Export VN depth slice"), tr("No depth-slice data available."));
        return;
    }

    QString target = vnDepthSliceExportEdit->text().trimmed();
    if (target.isEmpty()) {
        target = QFileDialog::getSaveFileName(this,
                                              tr("Save VN depth slice CSV"),
                                              QDir(workingDirEdit->text().trimmed()).filePath(QStringLiteral("vn_depth_slice.csv")),
                                              tr("CSV files (*.csv);;All files (*.*)"));
        if (target.isEmpty()) return;
        vnDepthSliceExportEdit->setText(target);
    }

    QSaveFile out(target);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export VN depth slice"), tr("Could not open target CSV for writing."));
        return;
    }
    QTextStream ts(&out);
    ts << "depth_m,value,target_x,x_column,depth_column,y_column\n";
    for (const QPointF &pt : series) {
        ts << pt.x() << ',' << pt.y() << ',' << targetX << ','
           << outputNumericHeaders.value(xIdx) << ','
           << outputNumericHeaders.value(depthIdx) << ','
           << outputNumericHeaders.value(yIdx) << "\n";
    }
    if (!out.commit()) {
        QMessageBox::warning(this, tr("Export VN depth slice"), tr("Could not finalize depth-slice CSV."));
        return;
    }
    appendLog(stamp(tr("Exported VN depth-slice CSV: %1").arg(target)));
    saveSettings();
}


void ModelCreatorWindow::exportVnErtSnapshotCsv()
{
    QString validationError;
    if (!validateVnAwarenessInputs(&validationError, false)) {
        QMessageBox::warning(this, tr("Export ERT-ready CSV"), validationError);
        return;
    }

    QString error;
    if (!loadOutputColumns(&error)) {
        QMessageBox::warning(this, tr("Export ERT-ready CSV"), error);
        return;
    }

    const int xIdx = outputXAxisCombo->currentIndex();
    const int yIdx = outputYAxisCombo->currentIndex();
    const int depthIdx = depthColumnCombo->currentIndex();
    if (xIdx < 0 || yIdx < 0 || depthIdx < 0) {
        QMessageBox::warning(this, tr("Export ERT-ready CSV"), tr("Select valid X/Y/Depth columns first."));
        return;
    }

    bool ok = false;
    double targetX = sliceXEdit->text().trimmed().toDouble(&ok);
    if (!ok) {
        const QVector<double> &xCol = outputNumericColumns[xIdx];
        if (xCol.isEmpty()) {
            QMessageBox::warning(this, tr("Export ERT-ready CSV"), tr("X column is empty."));
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

    const QVector<QPointF> series = computeDepthSliceSeries(targetX, xIdx, yIdx, depthIdx);
    if (series.isEmpty()) {
        QMessageBox::information(this, tr("Export ERT-ready CSV"), tr("No depth-slice data available."));
        return;
    }

    QString target = vnErtSnapshotExportEdit->text().trimmed();
    if (target.isEmpty()) {
        const QString suggested = QDir(workingDirEdit->text().trimmed()).filePath(QStringLiteral("vn_ert_snapshot.csv"));
        target = QFileDialog::getSaveFileName(this,
                                              tr("Save ERT-ready CSV"),
                                              suggested,
                                              tr("CSV files (*.csv);;All files (*.*)"));
        if (target.isEmpty()) {
            return;
        }
        vnErtSnapshotExportEdit->setText(target);
    }

    const QString boreholeName = InferErtBoreholeName(targetX);
    QSaveFile out(target);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export ERT-ready CSV"), tr("Could not open target CSV for writing."));
        return;
    }

    QTextStream ts(&out);
    ts << "borehole_name,r_m,depth_m,theta_model,target_x,x_column,depth_column,value_column,init_theta_mode,field_points,field_seed,field_dx,field_pdf_mode\n";
    for (const QPointF &pt : series) {
        ts << boreholeName << ','
           << targetX << ','
           << pt.x() << ','
           << pt.y() << ','
           << targetX << ','
           << outputNumericHeaders.value(xIdx) << ','
           << outputNumericHeaders.value(depthIdx) << ','
           << outputNumericHeaders.value(yIdx) << ','
           << currentEffectiveVnInitTheta() << ','
           << currentEffectiveVnFieldPoints() << ','
           << currentEffectiveVnFieldSeed() << ','
           << currentEffectiveVnFieldDx() << ','
           << currentEffectiveVnFieldPdf() << "\n";
    }
    if (!out.commit()) {
        QMessageBox::warning(this, tr("Export ERT-ready CSV"), tr("Could not finalize ERT-ready CSV."));
        return;
    }

    vnErtSnapshotStatus = QStringLiteral("exported_in_app");
    appendLog(stamp(tr("Exported ERT-ready borehole CSV: %1").arg(target)));
    appendLog(stamp(tr("This app-side export uses the currently selected output/depth columns and slice X/R as a borehole-style profile.")));
    saveSettings();
}

void ModelCreatorWindow::exportVtkInventoryCsv()
{
    const QString workingDirectory = workingDirEdit->text().trimmed();
    if (workingDirectory.isEmpty()) {
        QMessageBox::warning(this, tr("Export VTK inventory"), tr("Set a working directory first."));
        return;
    }
    const QFileInfo wdInfo(workingDirectory);
    if (!wdInfo.exists() || !wdInfo.isDir()) {
        QMessageBox::warning(this, tr("Export VTK inventory"), tr("Working directory does not exist or is not a directory."));
        return;
    }

    QStringList vtkFiles;
    QDirIterator it(workingDirectory, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QFileInfo fi = it.fileInfo();
        const QString ext = fi.suffix().toLower();
        if (ext == QStringLiteral("vtk") || ext == QStringLiteral("vtp") || ext == QStringLiteral("vtu")
            || ext == QStringLiteral("vti") || ext == QStringLiteral("pvd")
            || ext == QStringLiteral("vtm") || ext == QStringLiteral("vtmb")) {
            vtkFiles << fi.absoluteFilePath();
        }
    }
    vtkFiles.sort();

    if (vtkFiles.isEmpty()) {
        QMessageBox::information(this, tr("Export VTK inventory"), tr("No VTK files (.vtk/.vtp/.vtu/.vti/.pvd/.vtm/.vtmb) were found under the current working directory."));
        return;
    }

    QString target = vtkInventoryExportEdit->text().trimmed();
    if (target.isEmpty()) {
        const QString suggested = QDir(workingDirectory).filePath(QStringLiteral("vtk_inventory.csv"));
        target = QFileDialog::getSaveFileName(this,
                                              tr("Save VTK inventory CSV"),
                                              suggested,
                                              tr("CSV files (*.csv);;All files (*.*)"));
        if (target.isEmpty()) {
            return;
        }
        vtkInventoryExportEdit->setText(target);
    }

    const auto esc = [](const QString &value) {
        QString copy = value;
        copy.replace('"', "\"\"");
        return copy;
    };

    QSaveFile out(target);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export VTK inventory"), tr("Could not open target CSV for writing."));
        return;
    }

    QTextStream ts(&out);
    ts << "file_path,relative_path,file_name,extension,size_bytes,last_modified_utc\n";
    const QDir root(workingDirectory);
    for (const QString &path : vtkFiles) {
        const QFileInfo fi(path);
        ts << '"' << esc(fi.absoluteFilePath()) << '"' << ','
           << '"' << esc(root.relativeFilePath(fi.absoluteFilePath())) << '"' << ','
           << '"' << esc(fi.fileName()) << '"' << ','
           << fi.suffix().toLower() << ','
           << fi.size() << ','
           << fi.lastModified().toUTC().toString(Qt::ISODate) << "\n";
    }
    if (!out.commit()) {
        QMessageBox::warning(this, tr("Export VTK inventory"), tr("Could not finalize VTK inventory CSV."));
        return;
    }

    appendLog(stamp(tr("Exported VTK inventory CSV: %1 (%2 file(s))").arg(target).arg(vtkFiles.size())));
    saveSettings();
}


QStringList ModelCreatorWindow::createVnPvdSidecars(const QString &vtkDir,
                                                    const QStringList &prefixes,
                                                    int timestepCount,
                                                    const QVector<double> &times)
{
    QStringList created;
    if (vtkDir.trimmed().isEmpty() || timestepCount <= 0) {
        return created;
    }

    QDir dir(vtkDir);
    if (!dir.exists()) {
        return created;
    }

    for (const QString &prefix : prefixes) {
        const QStringList files = dir.entryList(QStringList() << QStringLiteral("%1_*.vtp").arg(prefix),
                                                QDir::Files | QDir::NoSymLinks,
                                                QDir::Name);
        if (files.isEmpty()) {
            continue;
        }

        const QString pvdPath = dir.filePath(QStringLiteral("%1.pvd").arg(prefix));
        QSaveFile out(pvdPath);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
            appendLog(stamp(tr("VN VTK PVD export warning: could not open %1 for writing.").arg(pvdPath)));
            continue;
        }

        QTextStream ts(&out);
        ts << "<?xml version=\"1.0\"?>\n";
        ts << "<VTKFile type=\"Collection\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
        ts << "  <Collection>\n";
        const int n = qMin(files.size(), timestepCount);
        for (int i = 0; i < n; ++i) {
            const double t = (i < times.size()) ? times.at(i) : static_cast<double>(i);
            ts << "    <DataSet timestep=\"" << QString::number(t, 'g', 15)
               << "\" group=\"\" part=\"0\" file=\"" << VnXmlEscape(files.at(i)) << "\"/>\n";
        }
        ts << "  </Collection>\n";
        ts << "</VTKFile>\n";

        if (out.commit()) {
            created << pvdPath;
        } else {
            appendLog(stamp(tr("VN VTK PVD export warning: could not finalize %1.").arg(pvdPath)));
        }
    }

    return created;
}

QStringList ModelCreatorWindow::createVnVtkOutputsFromRunArtifacts()
{
    QStringList created;
    if (modelTypeCombo->currentText().trimmed().compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) != 0) {
        return created;
    }

    const QString scriptPath = scriptPathEdit->text().trimmed();
    const QString workingDirectory = workingDirEdit->text().trimmed();
    if (scriptPath.isEmpty() || workingDirectory.isEmpty()) {
        return created;
    }

    const QVector<VnVtkBlockPoint> geometry = VnReadSoilBlockGeometryForVtk(scriptPath);
    if (geometry.isEmpty()) {
        vnResultGridStatus = QStringLiteral("vtk_skipped_no_soil_geometry");
        appendLog(stamp(tr("VN VTK export skipped: no Soil-g/Soil-uw block geometry was found in the generated script.")));
        return created;
    }

    QString outputPath = outputSeriesFileEdit->text().trimmed();
    if (outputPath.isEmpty()) {
        outputPath = QDir(workingDirectory).filePath(QStringLiteral("OHQ_output.txt"));
    } else if (QFileInfo(outputPath).isRelative()) {
        outputPath = QDir(workingDirectory).filePath(outputPath);
    }
    if (!QFileInfo::exists(outputPath)) {
        const QString fallback1 = QDir(workingDirectory).filePath(QStringLiteral("output.txt"));
        const QString fallback2 = QDir(workingDirectory).filePath(QStringLiteral("Output_LR.txt"));
        if (QFileInfo::exists(fallback1)) {
            outputPath = fallback1;
        } else if (QFileInfo::exists(fallback2)) {
            outputPath = fallback2;
        } else {
            vnResultGridStatus = QStringLiteral("vtk_skipped_no_output_series_file");
            appendLog(stamp(tr("VN VTK export skipped: output time-series file was not found.")));
            return created;
        }
    }

    QFile file(outputPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        vnResultGridStatus = QStringLiteral("vtk_skipped_output_series_unreadable");
        appendLog(stamp(tr("VN VTK export skipped: could not read output file %1").arg(outputPath)));
        return created;
    }

    QTextStream in(&file);
    QString headerLine;
    while (!in.atEnd() && headerLine.trimmed().isEmpty()) {
        headerLine = in.readLine();
    }
    const QStringList header = VnSplitDelimitedLine(headerLine);
    if (header.size() < 2) {
        vnResultGridStatus = QStringLiteral("vtk_skipped_unrecognized_output_series_header");
        appendLog(stamp(tr("VN VTK export skipped: output header was not recognized.")));
        return created;
    }

    const int timeColumn = VnFindTimeColumn(header);
    struct QuantitySpec {
        QString seriesQuantity;
        QString scalarName;
        QString filePrefix;
        QMap<int, int> blockToColumn;
    };

    QVector<QuantitySpec> quantities;
    QuantitySpec theta; theta.seriesQuantity = QStringLiteral("theta"); theta.scalarName = QStringLiteral("Moisture_content"); theta.filePrefix = QStringLiteral("moisture"); quantities.push_back(theta);
    QuantitySpec age; age.seriesQuantity = QStringLiteral("meanagetracer:concentration"); age.scalarName = QStringLiteral("Mean_Age"); age.filePrefix = QStringLiteral("mean_age"); quantities.push_back(age);

    for (QuantitySpec &spec : quantities) {
        for (int b = 0; b < geometry.size(); ++b) {
            const QString expected1 = geometry.at(b).name + QStringLiteral("_") + spec.seriesQuantity;
            const QString expected2 = geometry.at(b).name + QStringLiteral(":") + spec.seriesQuantity;
            for (int c = 0; c < header.size(); ++c) {
                const QString h = header.at(c).trimmed();
                if (h.compare(expected1, Qt::CaseInsensitive) == 0
                    || h.compare(expected2, Qt::CaseInsensitive) == 0
                    || (h.contains(geometry.at(b).name, Qt::CaseInsensitive)
                        && h.contains(spec.seriesQuantity, Qt::CaseInsensitive))) {
                    spec.blockToColumn.insert(b, c);
                    break;
                }
            }
        }
    }

    const QString vtkDir = QDir(workingDirectory).filePath(QStringLiteral("Moisture"));
    QDir().mkpath(vtkDir);
    int rowIndex = 0;
    int written = 0;
    QVector<double> times;
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }
        const QStringList fields = VnSplitDelimitedLine(line);
        if (fields.size() < header.size()) {
            continue;
        }
        double t = rowIndex;
        if (timeColumn >= 0 && timeColumn < fields.size()) {
            bool okT = false;
            const double parsedT = fields.at(timeColumn).trimmed().toDouble(&okT);
            if (okT && std::isfinite(parsedT)) {
                t = parsedT;
            }
        }
        times.push_back(t);

        for (const QuantitySpec &spec : quantities) {
            if (spec.blockToColumn.isEmpty()) {
                continue;
            }
            QVector<VnVtkBlockPoint> pts;
            QVector<double> vals;
            pts.reserve(spec.blockToColumn.size());
            vals.reserve(spec.blockToColumn.size());
            for (auto it = spec.blockToColumn.constBegin(); it != spec.blockToColumn.constEnd(); ++it) {
                const int blockIndex = it.key();
                const int columnIndex = it.value();
                if (blockIndex < 0 || blockIndex >= geometry.size() || columnIndex < 0 || columnIndex >= fields.size()) {
                    continue;
                }
                bool okV = false;
                const double value = fields.at(columnIndex).trimmed().toDouble(&okV);
                if (!okV || !std::isfinite(value)) {
                    continue;
                }
                pts.push_back(geometry.at(blockIndex));
                vals.push_back(value);
            }
            if (pts.isEmpty()) {
                continue;
            }
            const QString fileName = QStringLiteral("%1_%2.vtp").arg(spec.filePrefix).arg(rowIndex + 1, 4, 10, QLatin1Char('0'));
            const QString outPath = QDir(vtkDir).filePath(fileName);
            QString error;
            if (VnWriteDelaunayVtp(outPath, spec.scalarName, pts, vals, &error)) {
                created << outPath;
                ++written;
            } else if (!error.trimmed().isEmpty()) {
                appendLog(stamp(tr("VN VTK export warning: %1").arg(error)));
            }
        }
        ++rowIndex;
    }

    if (written > 0) {
        QStringList prefixes;
        for (const QuantitySpec &spec : quantities) {
            if (!spec.blockToColumn.isEmpty()) {
                prefixes << spec.filePrefix;
            }
        }
        const QStringList pvdFiles = createVnPvdSidecars(vtkDir, prefixes, rowIndex, times);
        for (const QString &path : pvdFiles) {
            if (!created.contains(path)) {
                created << path;
            }
        }

        vnResultGridStatus = pvdFiles.isEmpty()
            ? QStringLiteral("created_delaunay_vtp_from_soil_output_series")
            : QStringLiteral("created_delaunay_vtp_and_pvd_from_soil_output_series");
        appendLog(stamp(tr("VN VTK export created %1 VTP snapshot file(s) in %2").arg(written).arg(vtkDir)));
        if (!pvdFiles.isEmpty()) {
            appendLog(stamp(tr("VN VTK export created %1 PVD time-series file(s).").arg(pvdFiles.size())));
        }
    } else {
        vnResultGridStatus = QStringLiteral("vtk_skipped_no_matching_theta_or_age_series");
        appendLog(stamp(tr("VN VTK export skipped: no matching theta or mean-age output columns were found for Soil-g/Soil-uw blocks.")));
    }

    return created;
}

void ModelCreatorWindow::exportVnVtkSnapshots()
{
    const QStringList created = createVnVtkOutputsFromRunArtifacts();
    if (created.isEmpty()) {
        QMessageBox::information(this, tr("Export VN VTK"), tr("No VN VTK files were created. Check that a VN run output file and generated script are available."));
        return;
    }

    appendLog(stamp(tr("Export VN VTK completed: %1 file(s) created or refreshed.").arg(created.size())));
    const QStringList artifacts = collectRunArtifacts();
    if (!artifacts.isEmpty()) {
        updateVnRuntimeStatusFromArtifacts(artifacts);
        copyArtifacts(artifacts);
        writeArtifactManifest(artifacts);
    }
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

    static const QStringList allowedExt = {"txt", "csv", "log", "json", "ohq", "vtk", "vtp", "vtu", "vti", "pvd", "vtm", "vtmb"};

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

    const bool vnRunContext =
        modelTypeCombo->currentText().trimmed().compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0;
    const QString effectiveInitTheta = vnRunContext ? currentEffectiveVnInitTheta() : QString();
    const QString effectiveFieldPoints = vnRunContext ? currentEffectiveVnFieldPoints() : QString();
    const QString effectiveFieldSeed = vnRunContext ? currentEffectiveVnFieldSeed() : QString();
    const QString effectiveFieldDx = vnRunContext ? currentEffectiveVnFieldDx() : QString();
    const QString effectiveFieldPdf = vnRunContext ? currentEffectiveVnFieldPdf() : QString();
    const QString effectiveKsatAll = vnRunContext ? currentEffectiveKsatAll() : QString();
    const QString effectiveKsatG = vnRunContext ? currentEffectiveKsatG() : QString();
    const QString effectiveKsatUw = vnRunContext ? currentEffectiveKsatUw() : QString();

    const auto esc = [](const QString &value) {
        QString copy = value;
        copy.replace('"', "\"\"");
        return copy;
    };

    QSaveFile manifest(QDir(targetDirPath).filePath("artifact_manifest.csv"));
    if (!manifest.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream ts(&manifest);
    ts << "file_path,file_name,last_modified_utc,model_type,vn_init_theta_mode,vn_field_points,vn_field_seed,vn_field_dx,vn_field_pdf_mode,ksat_all,ksat_g,ksat_uw\n";
    for (const QString &path : artifacts) {
        const QFileInfo fi(path);
        ts << '"' << esc(fi.absoluteFilePath()) << '"' << ','
           << '"' << esc(fi.fileName()) << '"' << ','
           << fi.lastModified().toUTC().toString(Qt::ISODate) << ','
           << '"' << esc(modelTypeCombo->currentText().trimmed()) << '"' << ','
           << '"' << esc(effectiveInitTheta) << '"' << ','
           << '"' << esc(effectiveFieldPoints) << '"' << ','
           << '"' << esc(effectiveFieldSeed) << '"' << ','
           << '"' << esc(effectiveFieldDx) << '"' << ','
           << '"' << esc(effectiveFieldPdf) << '"' << ','
           << '"' << esc(effectiveKsatAll) << '"' << ','
           << '"' << esc(effectiveKsatG) << '"' << ','
           << '"' << esc(effectiveKsatUw) << '"' << "\n";
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
