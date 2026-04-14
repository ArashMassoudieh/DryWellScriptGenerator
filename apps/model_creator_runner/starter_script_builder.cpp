// NOTE: This file is part of the DryWellSuite/OpenHydroQual codebase.
#include "starter_script_builder.h"
#include "hq_drywell_builder.h"
#include "r_bioswale_builder.h"
#include "structure_registry.h"
#include "vn_drywell_builder.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTextStream>
#include <QVector>
#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>

namespace {

QString TemplateFile(const QString &templateDir, const QString &filename)
{
    return QDir(templateDir).filePath(filename).replace('\\', '/');
}

QStringList RequiredTemplates()
{
    return {
        QStringLiteral("main_components.json"),
        QStringLiteral("Pond_Plugin.json"),
        QStringLiteral("unsaturated_soil.json"),
        QStringLiteral("Well.json"),
        QStringLiteral("Sewer_system.json"),
        QStringLiteral("soil_evapotranspiration_models.json"),
        QStringLiteral("evapotranspiration_models.json"),
        QStringLiteral("pipe_pump_tank.json")
    };
}

QStringList RequiredVnFullReferenceTemplates()
{
    return {
        QStringLiteral("main_components.json"),
        QStringLiteral("unsaturated_soil_revised_model.json"),
        QStringLiteral("Well.json"),
        QStringLiteral("Sewer_system.json"),
        QStringLiteral("pipe_pump_tank.json"),
        QStringLiteral("Pond_Plugin.json")
    };
}

bool IsNumber(const QString &value)
{
    bool ok = false;
    value.toDouble(&ok);
    return ok;
}

QString ResolveKsatScaleString(const QString &primary,
                               const QString &fallback,
                               const QString &defaultValue)
{
    const auto sanePositive = [](const QString &raw) -> QString {
        bool ok = false;
        const double parsed = raw.toDouble(&ok);
        if (ok && std::isfinite(parsed) && parsed > 0.0) {
            return raw;
        }
        return {};
    };

    const QString p = primary.trimmed();
    const QString pSane = sanePositive(p);
    if (!pSane.isEmpty()) {
        return pSane;
    }
    const QString f = fallback.trimmed();
    const QString fSane = sanePositive(f);
    if (!fSane.isEmpty()) {
        return fSane;
    }
    return defaultValue;
}

void ApplyVnKsatScaleOverrides(QString *scriptText, const StarterScriptOptions &options)
{
    if (scriptText == nullptr) {
        return;
    }
    const QString gScale = ResolveKsatScaleString(options.ksatScaleG, options.ksatScaleAll, QStringLiteral("1.0"));
    const QString uwScale = ResolveKsatScaleString(options.ksatScaleUw, options.ksatScaleAll, QStringLiteral("1.0"));
    scriptText->replace(QStringLiteral("K_sat_scale_factor=2.5"),
                        QStringLiteral("K_sat_scale_factor=%1").arg(gScale));
    scriptText->replace(QStringLiteral("K_sat_scale_factor=35"),
                        QStringLiteral("K_sat_scale_factor=%1").arg(uwScale));
}

bool LoadEntireFile(const QString &path, QString *text, QString *errorMessage)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Unable to read file: %1").arg(path);
        }
        return false;
    }
    QTextStream in(&file);
    if (text) {
        *text = in.readAll();
    }
    return true;
}

QString ExtractCommandValue(const QString &line, const QString &key);
QString NormalizeVnSoftSoilParamMode(const QString &mode);

void ApplyCommonScriptFixups(QString *scriptText, const QString &inflowFile)
{
    if (scriptText == nullptr) {
        return;
    }
    if (!inflowFile.trimmed().isEmpty()) {
        scriptText->replace(QStringLiteral("Synthetic_rain_flow.csv"), inflowFile);
        scriptText->replace(QRegularExpression(QStringLiteral("(?i)(\\binflow\\s*=)\\s*([^,\\n\\r]+)")),
                            QStringLiteral("\\1") + inflowFile);
        scriptText->replace(QRegularExpression(QStringLiteral("(?i)(\\bquantity\\s*=\\s*inflow\\s*,\\s*value\\s*=)\\s*([^\\n\\r]+)")),
                            QStringLiteral("\\1") + inflowFile);
    }
    scriptText->replace(QRegularExpression(QStringLiteral("(?i)\\bactual_x\\b")), QStringLiteral("act_X"));
    scriptText->replace(QRegularExpression(QStringLiteral("(?i)\\bactual_y\\b")), QStringLiteral("act_Y"));

    const QStringList lines = scriptText->split('\n', Qt::KeepEmptyParts);
    QStringList filtered;
    filtered.reserve(lines.size());
    for (const QString &line : lines) {
        const QString lower = line.toLower();
        if (lower.contains(QStringLiteral("create observation;")) && lower.contains(QStringLiteral("observed_data="))) {
            const QString observed = ExtractCommandValue(line, QStringLiteral("observed_data"));
            if (!observed.trimmed().isEmpty()) {
                QFileInfo fi(observed.trimmed());
                if (!fi.exists()) {
                    continue;
                }
            }
        }
        filtered.push_back(line);
    }
    *scriptText = filtered.join('\n');
}

bool AppendSnippetFile(const QString &path,
                       const QString &label,
                       QString *scriptText,
                       QString *errorMessage)
{
    if (path.trimmed().isEmpty()) {
        return true;
    }

    QString snippet;
    if (!LoadEntireFile(path, &snippet, errorMessage)) {
        return false;
    }

    if (snippet.trimmed().isEmpty()) {
        return true;
    }

    if (scriptText == nullptr) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Internal error: script output buffer is null.");
        }
        return false;
    }

    if (!scriptText->isEmpty() && !scriptText->endsWith('\n')) {
        *scriptText += '\n';
    }

    *scriptText += QStringLiteral("\n# %1 snippet loaded from %2\n")
                       .arg(label, path);
    *scriptText += snippet.trimmed();
    *scriptText += '\n';
    return true;
}

bool IsKnownPreset(const QString &preset)
{
    return StructureRegistry::IsKnownPreset(preset);
}

bool IsHQ_DrywellLikeModel(const QString &modelType)
{
    return modelType.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0
        || modelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0;
}

bool IsKnownModelType(const QString &modelType)
{
    return StructureRegistry::IsKnownModelType(modelType);
}

bool IsPresetCompatibleWithModel(const QString &preset, const QString &modelType)
{
    return StructureRegistry::IsPresetCompatibleWithModel(preset, modelType);
}

bool IsVnModel(const QString &modelType)
{
    return modelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0;
}

QString DefaultVnInflowFile()
{
    return QStringLiteral("Synthetic_rain_flow.csv");
}

QString EmbeddedFullReferenceScriptForModel(const QString &modelType)
{
    if (modelType.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0) {
        return HqDrywellBuilder::FullReferenceScript();
    }
    if (modelType.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0) {
        return RBioswaleBuilder::FullReferenceScript();
    }
    return VnDrywellBuilder::VnFullReferenceScript();
}

QString EmbeddedInflowTargetForModel(const QString &modelType)
{
    if (modelType.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0) {
        return HqDrywellBuilder::InflowTargetObject();
    }
    if (modelType.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0) {
        return RBioswaleBuilder::InflowTargetObject();
    }
    return VnDrywellBuilder::InflowTargetObject();
}

QString ExtractEmbeddedReferenceInflowForModel(const QString &modelType)
{
    const QString embedded = EmbeddedFullReferenceScriptForModel(modelType);
    const QString target = EmbeddedInflowTargetForModel(modelType);
    if (embedded.trimmed().isEmpty() || target.trimmed().isEmpty()) {
        return QString();
    }
    const QStringList lines = embedded.split('\n', Qt::SkipEmptyParts);
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.contains(QStringLiteral("quantity=inflow"), Qt::CaseInsensitive)
            && line.contains(QStringLiteral("object=%1").arg(target), Qt::CaseInsensitive)) {
            const QString value = ExtractCommandValue(line, QStringLiteral("value"));
            if (!value.trimmed().isEmpty()) {
                return value.trimmed();
            }
        }
        if (line.startsWith(QStringLiteral("create block;"), Qt::CaseInsensitive)
            && line.contains(QStringLiteral("name=%1").arg(target), Qt::CaseInsensitive)
            && line.contains(QStringLiteral("inflow="), Qt::CaseInsensitive)) {
            const QString value = ExtractCommandValue(line, QStringLiteral("inflow"));
            if (!value.trimmed().isEmpty()) {
                return value.trimmed();
            }
        }
    }
    return QString();
}

QStringList CandidateProjectRootsFromTemplateDirectory(const QString &templateDirectory)
{
    QStringList roots {
        QStringLiteral("/mnt/3rd900/Projects"),
        QStringLiteral("/home/arash/Projects"),
        QStringLiteral("/home/hoomanmoradpour/Projects"),
        QStringLiteral("/media/arash/E/Projects")
    };
    const QFileInfo templateInfo(templateDirectory);
    if (templateInfo.exists()) {
        QDir dir = templateInfo.isDir() ? QDir(templateInfo.absoluteFilePath())
                                        : templateInfo.absoluteDir();
        // Typical template dir: <Projects>/OpenHydroQual/resources
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

QString DetectStructureDefaultInflowFile(const QString &modelType,
                                         const QString &templateDirectory)
{
    const QString normalizedModel = modelType.trimmed();
    QStringList candidates;
    const QString embeddedDefault = ExtractEmbeddedReferenceInflowForModel(normalizedModel);
    if (!embeddedDefault.trimmed().isEmpty()) {
        candidates << embeddedDefault.trimmed();
    }
    const QStringList projectRoots = CandidateProjectRootsFromTemplateDirectory(templateDirectory);
    if (normalizedModel.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0) {
        for (const QString &root : projectRoots) {
            candidates << QDir(root).filePath(QStringLiteral("LA Project/Data/Inflow_Corrected_New_Khiem.csv"));
        }
        candidates << QStringLiteral("/mnt/3rd900/Projects/LA Project/Data/Inflow_Corrected_New_Khiem.csv");
    } else if (normalizedModel.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0) {
        for (const QString &root : projectRoots) {
            candidates << QDir(root).filePath(QStringLiteral("LA Project/Data/Inflow_Rosemead_August.txt"));
        }
        candidates << QStringLiteral("/mnt/3rd900/Projects/LA Project/Data/Inflow_Rosemead_August.txt");
    } else {
        for (const QString &root : projectRoots) {
            candidates << QDir(root).filePath(QStringLiteral("VN Drywell_Models/LA_Precipitaion (5 yr new).csv"));
        }
        candidates << QStringLiteral("/mnt/3rd900/Projects/VN Drywell_Models/LA_Precipitaion (5 yr new).csv");
    }
    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }
    return candidates.isEmpty() ? QString() : candidates.front();
}

bool IsKnownReferenceInflowForOtherModel(const QString &inflowPath, const QString &targetModel)
{
    const QString p = inflowPath.trimmed();
    if (p.isEmpty()) {
        return false;
    }
    const QString vnRef = ExtractEmbeddedReferenceInflowForModel(QStringLiteral("VN_Drywell"));
    const QString hqRef = ExtractEmbeddedReferenceInflowForModel(QStringLiteral("HQ_Drywell"));
    const QString rRef = ExtractEmbeddedReferenceInflowForModel(QStringLiteral("R_Bioswale"));
    const QString pName = QFileInfo(p).fileName();
    const QString vnName = vnRef.isEmpty() ? QStringLiteral("LA_Precipitaion (5 yr new).csv") : QFileInfo(vnRef).fileName();
    const QString hqName = hqRef.isEmpty() ? QStringLiteral("Inflow_Corrected_New_Khiem.csv") : QFileInfo(hqRef).fileName();
    const QString rName = rRef.isEmpty() ? QStringLiteral("Inflow_Rosemead_August.txt") : QFileInfo(rRef).fileName();
    const bool isVnRef = (!vnRef.isEmpty() && p.compare(vnRef, Qt::CaseInsensitive) == 0)
        || pName.compare(vnName, Qt::CaseInsensitive) == 0;
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

QString NormalizeVnBuildMode(const QString &mode)
{
    const QString m = mode.trimmed();
    if (m.compare(QStringLiteral("FullReference"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("FullReference");
    }
    if (m.compare(QStringLiteral("SoftReference"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("SoftReference");
    }
    if (m.compare(QStringLiteral("LoadFromOhq"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("LoadFromOhq");
    }
    if (m.compare(QStringLiteral("Preset"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("Preset");
    }
    return QStringLiteral("SoftReference");
}

QString NormalizeStructureBuildMode(const QString &mode)
{
    const QString m = mode.trimmed();
    if (m.compare(QStringLiteral("SoftReference"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("SoftReference");
    }
    if (m.compare(QStringLiteral("FullReference"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("FullReference");
    }
    if (m.compare(QStringLiteral("LoadFromOhq"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("LoadFromOhq");
    }
    if (m.compare(QStringLiteral("Preset"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("Preset");
    }
    return QStringLiteral("Preset");
}

QString ResolveVnPreset(const StarterScriptOptions &options)
{
    const QString vnPreset = options.vnPreset.trimmed();
    if (!vnPreset.isEmpty()) {
        return vnPreset;
    }

    const QString legacyPreset = options.enrichmentPreset.trimmed();
    if (!legacyPreset.isEmpty()) {
        return legacyPreset;
    }

    return QStringLiteral("VN_Drywell");
}

QString EffectiveBuildModeForMetadata(const StarterScriptOptions &options)
{
    if (IsVnModel(options.modelType)) {
        return NormalizeVnBuildMode(options.vnBuildMode);
    }
    if (options.modelType.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0) {
        return NormalizeStructureBuildMode(options.hqBuildMode);
    }
    if (options.modelType.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0) {
        return NormalizeStructureBuildMode(options.rBioswaleBuildMode);
    }
    return QStringLiteral("Preset");
}

QString EffectivePresetForMetadata(const StarterScriptOptions &options,
                                   const QString &effectiveBuildMode)
{
    if (effectiveBuildMode != QStringLiteral("Preset")) {
        return QString();
    }
    if (IsVnModel(options.modelType)) {
        return ResolveVnPreset(options);
    }
    return options.enrichmentPreset.trimmed();
}

QString EffectiveSoilParameterStrategyForMetadata(const StarterScriptOptions &options,
                                                  const QString &effectiveBuildMode)
{
    if (IsVnModel(options.modelType)) {
        if (effectiveBuildMode == QStringLiteral("SoftReference")) {
            const QString vnSoilMode = NormalizeVnSoftSoilParamMode(options.vnSoftSoilParamMode);
            if (vnSoilMode == QStringLiteral("InputFile")) {
                return QStringLiteral("VN_InputProfile");
            }
            if (vnSoilMode == QStringLiteral("VnReferenceDefaults")) {
                return QStringLiteral("VN_ReferenceDefaults");
            }
            return QStringLiteral("VN_ModelCreatorDefaults");
        }
        return QStringLiteral("VN_EmbeddedReference");
    }
    if (options.modelType.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("HQ_EmbeddedReference");
    }
    if (options.modelType.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("R_EmbeddedReference");
    }
    return QStringLiteral("Unknown");
}

void PrependStarterMetadata(const StarterScriptOptions &options, QString *scriptText)
{
    if (scriptText == nullptr) {
        return;
    }
    QString header;
    QTextStream hs(&header);
    const QString buildMode = EffectiveBuildModeForMetadata(options);
    const QString preset = EffectivePresetForMetadata(options, buildMode);
    const QString soilStrategy = EffectiveSoilParameterStrategyForMetadata(options, buildMode);
    hs << "# starter_metadata:model_type=" << options.modelType.trimmed() << "\n";
    hs << "# starter_metadata:build_mode=" << buildMode << "\n";
    hs << "# starter_metadata:soil_param_strategy=" << soilStrategy << "\n";
    if (!preset.isEmpty()) {
        hs << "# starter_metadata:preset=" << preset << "\n";
    }
    hs << "\n";
    scriptText->prepend(header);
}


void AppendTemplateLoads(QString *scriptText, const QString &templateDirectory, const QStringList &templateFiles)
{
    if (scriptText == nullptr) {
        return;
    }

    if (!scriptText->isEmpty() && !scriptText->endsWith('\n')) {
        *scriptText += '\n';
    }

    for (int i = 0; i < templateFiles.size(); ++i) {
        const QString command = i == 0 ? QStringLiteral("loadtemplate") : QStringLiteral("addtemplate");
        *scriptText += QStringLiteral("%1; filename=%2\n")
                           .arg(command, TemplateFile(templateDirectory, templateFiles.at(i)));
    }
}

void AppendEmbeddedVnFullReferenceScript(const StarterScriptOptions &options, QString *scriptText)
{
    if (scriptText == nullptr) {
        return;
    }

    QString embedded = VnDrywellBuilder::VnFullReferenceScript();
    ApplyVnKsatScaleOverrides(&embedded, options);

    const QStringList filteredLines = embedded
                                          .split('\n', Qt::KeepEmptyParts);
    for (const QString &line : filteredLines) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("loadtemplate;"), Qt::CaseInsensitive)
            || trimmed.startsWith(QStringLiteral("addtemplate;"), Qt::CaseInsensitive)
            || trimmed.contains(QStringLiteral("quantity=simulation_start_time"), Qt::CaseInsensitive)
            || trimmed.contains(QStringLiteral("quantity=simulation_end_time"), Qt::CaseInsensitive)
            || trimmed.contains(QStringLiteral("quantity=outputfile"), Qt::CaseInsensitive)
            || trimmed.contains(QStringLiteral("quantity=numthreads"), Qt::CaseInsensitive)
            || trimmed.contains(QStringLiteral("quantity=number_of_threads"), Qt::CaseInsensitive)) {
            continue;
        }
        *scriptText += line + '\n';
    }

    if (!scriptText->endsWith('\n')) {
        *scriptText += '\n';
    }
}

bool IsSoftReferenceGridLine(const QString &line)
{
    return line.contains(QStringLiteral("name=Soil-uw ("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("name=Soil-uw("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("name=Soil-g ("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("name=Soil-g("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("from=Soil-uw ("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("from=Soil-uw("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("from=Soil-g ("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("from=Soil-g("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("to=Soil-uw ("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("to=Soil-uw("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("to=Soil-g ("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("to=Soil-g("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("HL_Well_g - Soil-uw"), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("VL_Well_g - Soil-uw"), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("HL_Well_g - Soil-g"), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("Soil to Groundwater ("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("type=fixed_head,name=Ground Water"), Qt::CaseInsensitive);
}

bool IsHqSoftReferenceSoilLine(const QString &line)
{
    return line.contains(QStringLiteral("name=Soil ("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("name=Soil("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("name=SoilDeep ("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("name=SoilDeep("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("from=Soil ("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("from=Soil("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("to=Soil ("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("to=Soil("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("from=SoilDeep ("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("from=SoilDeep("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("to=SoilDeep ("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("to=SoilDeep("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("object=Soil ("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("object=Soil("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("object=SoilDeep ("), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("object=SoilDeep("), Qt::CaseInsensitive);
}

bool IsRBioswaleSoftReferenceSoilLine(const QString &line)
{
    const QStringList tokens {
        QStringLiteral("EngineeredSoil ("),
        QStringLiteral("EngineeredSoil("),
        QStringLiteral("UEngineered ("),
        QStringLiteral("UEngineered("),
        QStringLiteral("LeftTop ("),
        QStringLiteral("LeftTop("),
        QStringLiteral("RightTop ("),
        QStringLiteral("RightTop("),
        QStringLiteral("LeftBottom ("),
        QStringLiteral("LeftBottom("),
        QStringLiteral("RightBottom ("),
        QStringLiteral("RightBottom(")
    };
    for (const QString &token : tokens) {
        if (line.contains(token, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

void AppendEmbeddedStructureSoftReferenceScaffold(const QString &embeddedScript,
                                                  const QString &inflowObject,
                                                  const std::function<bool(const QString &)> &isSoilLine,
                                                  QString *scriptText)
{
    if (scriptText == nullptr) {
        return;
    }

    const QStringList lines = embeddedScript.split('\n', Qt::KeepEmptyParts);
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("loadtemplate;"), Qt::CaseInsensitive)
            || trimmed.startsWith(QStringLiteral("addtemplate;"), Qt::CaseInsensitive)
            || trimmed.contains(QStringLiteral("quantity=simulation_start_time"), Qt::CaseInsensitive)
            || trimmed.contains(QStringLiteral("quantity=simulation_end_time"), Qt::CaseInsensitive)
            || trimmed.contains(QStringLiteral("quantity=outputfile"), Qt::CaseInsensitive)
            || trimmed.contains(QStringLiteral("quantity=numthreads"), Qt::CaseInsensitive)
            || trimmed.contains(QStringLiteral("quantity=number_of_threads"), Qt::CaseInsensitive)
            || trimmed.contains(QStringLiteral("setvalue; object=%1, quantity=inflow").arg(inflowObject), Qt::CaseInsensitive)
            || isSoilLine(trimmed)) {
            continue;
        }
        *scriptText += line + '\n';
    }

    if (!scriptText->endsWith('\n')) {
        *scriptText += '\n';
    }
}

void AppendEmbeddedStructureSoftReferenceSoils(const QString &embeddedScript,
                                               const std::function<bool(const QString &)> &isSoilLine,
                                               const std::function<QString(const QString &)> &lineTransformer,
                                               QTextStream *ts)
{
    if (ts == nullptr) {
        return;
    }
    const QStringList lines = embeddedScript.split('\n', Qt::KeepEmptyParts);
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || !isSoilLine(trimmed)) {
            continue;
        }
        *ts << (lineTransformer ? lineTransformer(line) : line) << '\n';
    }
}

struct VnSoftSoilProps
{
    double ksat = 1.05196;
    double alpha = 3.47536;
    double n = 1.74582;
    double thetaSat = 0.39;
    double thetaRes = 0.049;
};

struct VnSoftSoilProfileRow
{
    double depth = 0.0;
    double actY = 0.0;
    VnSoftSoilProps props;
};

QString ExtractCommandValue(const QString &line, const QString &key)
{
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
}

QString ReplaceCommandValue(QString line, const QString &key, const QString &value)
{
    const QString token = key + QStringLiteral("=");
    const int start = line.indexOf(token, 0, Qt::CaseInsensitive);
    if (start < 0) {
        return line;
    }
    const int valueStart = start + token.size();
    int end = line.indexOf(',', valueStart);
    if (end < 0) {
        end = line.size();
    }
    line.replace(valueStart, end - valueStart, value);
    return line;
}

VnSoftSoilProps ResolveSoftReferenceSoilOverrides(const StarterScriptOptions &options,
                                                  const VnSoftSoilProps &referenceDefaults,
                                                  bool allowModelCreatorDefaults = true)
{
    const QString mode = NormalizeVnSoftSoilParamMode(options.vnSoftSoilParamMode);
    if (mode == QStringLiteral("Manual")) {
        return VnSoftSoilProps {
            options.vnSoftSoilKsatOriginal,
            options.vnSoftSoilAlpha,
            options.vnSoftSoilN,
            options.vnSoftSoilThetaSat,
            options.vnSoftSoilThetaRes
        };
    }
    if (allowModelCreatorDefaults && mode == QStringLiteral("ModelCreatorDefaults")) {
        return VnSoftSoilProps { 1.05196, 3.47536, 1.74582, 0.39, 0.049 };
    }
    return referenceDefaults;
}

QString ApplySoilOverridesToLine(const QString &line,
                                 const VnSoftSoilProps &props)
{
    QString updated = line;
    updated = ReplaceCommandValue(updated, QStringLiteral("theta_sat"), QString::number(props.thetaSat, 'g', 12));
    updated = ReplaceCommandValue(updated, QStringLiteral("theta_res"), QString::number(props.thetaRes, 'g', 12));
    updated = ReplaceCommandValue(updated, QStringLiteral("n"), QString::number(props.n, 'g', 12));
    updated = ReplaceCommandValue(updated, QStringLiteral("K_sat_original"), QString::number(props.ksat, 'g', 12));
    updated = ReplaceCommandValue(updated, QStringLiteral("alpha"), QString::number(props.alpha, 'g', 12));
    return updated;
}

bool LoadVnReferenceProfileRows(QVector<VnSoftSoilProfileRow> *gRows,
                                QVector<VnSoftSoilProfileRow> *uwRows)
{
    if (gRows == nullptr || uwRows == nullptr) {
        return false;
    }
    gRows->clear();
    uwRows->clear();

    const QString embedded = VnDrywellBuilder::VnFullReferenceScript();
    const QStringList lines = embedded.split('\n', Qt::SkipEmptyParts);
    QHash<int, VnSoftSoilProfileRow> uniqueGByDepthKey;
    QHash<int, VnSoftSoilProfileRow> uniqueUwByDepthKey;

    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (!line.startsWith(QStringLiteral("create block;type=Soil"), Qt::CaseInsensitive)) {
            continue;
        }
        const bool isG = line.contains(QStringLiteral("name=Soil-g ("), Qt::CaseInsensitive)
            || line.contains(QStringLiteral("name=Soil-g("), Qt::CaseInsensitive);
        const bool isUw = line.contains(QStringLiteral("name=Soil-uw ("), Qt::CaseInsensitive)
            || line.contains(QStringLiteral("name=Soil-uw("), Qt::CaseInsensitive);
        if (!isG && !isUw) {
            continue;
        }

        bool okActY = false;
        bool okKsat = false;
        bool okAlpha = false;
        bool okN = false;
        bool okThetaSat = false;
        bool okThetaRes = false;

        const double actY = ExtractCommandValue(line, QStringLiteral("act_Y")).toDouble(&okActY);
        const double ksat = ExtractCommandValue(line, QStringLiteral("K_sat_original")).toDouble(&okKsat);
        const double alpha = ExtractCommandValue(line, QStringLiteral("alpha")).toDouble(&okAlpha);
        const double n = ExtractCommandValue(line, QStringLiteral("n")).toDouble(&okN);
        const double thetaSat = ExtractCommandValue(line, QStringLiteral("theta_sat")).toDouble(&okThetaSat);
        const double thetaRes = ExtractCommandValue(line, QStringLiteral("theta_res")).toDouble(&okThetaRes);
        if (!(okActY && okKsat && okAlpha && okN && okThetaSat && okThetaRes)) {
            continue;
        }

        const double depth = std::fabs(actY);
        const int depthKey = qRound(depth * 1000.0);
        QHash<int, VnSoftSoilProfileRow> &target = isG ? uniqueGByDepthKey : uniqueUwByDepthKey;
        if (target.contains(depthKey)) {
            continue;
        }
        VnSoftSoilProfileRow row;
        row.depth = depth;
        row.actY = actY;
        row.props.ksat = ksat;
        row.props.alpha = alpha;
        row.props.n = n;
        row.props.thetaSat = thetaSat;
        row.props.thetaRes = thetaRes;
        target.insert(depthKey, row);
    }

    gRows->reserve(uniqueGByDepthKey.size());
    for (auto it = uniqueGByDepthKey.cbegin(); it != uniqueGByDepthKey.cend(); ++it) {
        gRows->push_back(it.value());
    }
    uwRows->reserve(uniqueUwByDepthKey.size());
    for (auto it = uniqueUwByDepthKey.cbegin(); it != uniqueUwByDepthKey.cend(); ++it) {
        uwRows->push_back(it.value());
    }
    std::sort(gRows->begin(), gRows->end(), [](const VnSoftSoilProfileRow &lhs, const VnSoftSoilProfileRow &rhs) {
        return lhs.depth < rhs.depth;
    });
    std::sort(uwRows->begin(), uwRows->end(), [](const VnSoftSoilProfileRow &lhs, const VnSoftSoilProfileRow &rhs) {
        return lhs.depth < rhs.depth;
    });
    return !gRows->isEmpty() || !uwRows->isEmpty();
}

QString NormalizeVnSoftSoilParamMode(const QString &mode)
{
    const QString trimmed = mode.trimmed();
    const QString compact = trimmed.toLower().remove(' ').remove('_').remove('-');

    if (trimmed.compare(QStringLiteral("File"), Qt::CaseInsensitive) == 0
        || compact == QStringLiteral("file")
        || compact == QStringLiteral("filedepthprofile")) {
        return QStringLiteral("File");
    }
    if (trimmed.compare(QStringLiteral("VnReferenceDefaults"), Qt::CaseInsensitive) == 0
        || compact == QStringLiteral("vnrefdefaults")
        || compact == QStringLiteral("vnreferencedefaults")) {
        return QStringLiteral("VnReferenceDefaults");
    }
    if (trimmed.compare(QStringLiteral("ModelCreatorDefaults"), Qt::CaseInsensitive) == 0
        || compact == QStringLiteral("modelcreatordefaults")) {
        return QStringLiteral("ModelCreatorDefaults");
    }
    return QStringLiteral("Manual");
}

int FindColumnIndex(const QStringList &headers, const QStringList &aliases)
{
    for (int i = 0; i < headers.size(); ++i) {
        const QString h = headers.at(i).trimmed().toLower();
        for (const QString &alias : aliases) {
            if (h == alias) {
                return i;
            }
        }
    }
    return -1;
}

bool TryLoadVnSoftSoilProfile(const QString &csvPath, QVector<VnSoftSoilProfileRow> *rows)
{
    if (rows == nullptr) {
        return false;
    }
    rows->clear();

    QFile file(csvPath.trimmed());
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&file);
    bool headerRead = false;
    int depthIdx = -1;
    int ksatIdx = -1;
    int alphaIdx = -1;
    int nIdx = -1;
    int thetaSatIdx = -1;
    int thetaResIdx = -1;

    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }

        const QStringList cols = line.split(',', Qt::KeepEmptyParts);
        if (!headerRead) {
            headerRead = true;
            depthIdx = FindColumnIndex(cols, {QStringLiteral("depth"), QStringLiteral("depth_m")});
            ksatIdx = FindColumnIndex(cols, {QStringLiteral("ksat"), QStringLiteral("k_sat"), QStringLiteral("k_sat_original")});
            alphaIdx = FindColumnIndex(cols, {QStringLiteral("alpha")});
            nIdx = FindColumnIndex(cols, {QStringLiteral("n")});
            thetaSatIdx = FindColumnIndex(cols, {QStringLiteral("theta_s"), QStringLiteral("theta_sat")});
            thetaResIdx = FindColumnIndex(cols, {QStringLiteral("theta_r"), QStringLiteral("theta_res")});
            if (depthIdx < 0 || ksatIdx < 0 || alphaIdx < 0 || nIdx < 0 || thetaSatIdx < 0 || thetaResIdx < 0) {
                return false;
            }
            continue;
        }

        const int maxIndex = qMax(depthIdx, qMax(ksatIdx, qMax(alphaIdx, qMax(nIdx, qMax(thetaSatIdx, thetaResIdx)))));
        if (cols.size() <= maxIndex) {
            continue;
        }

        bool okDepth = false;
        bool okKsat = false;
        bool okAlpha = false;
        bool okN = false;
        bool okThetaSat = false;
        bool okThetaRes = false;
        const double depth = cols.at(depthIdx).trimmed().toDouble(&okDepth);
        const double ksat = cols.at(ksatIdx).trimmed().toDouble(&okKsat);
        const double alpha = cols.at(alphaIdx).trimmed().toDouble(&okAlpha);
        const double n = cols.at(nIdx).trimmed().toDouble(&okN);
        const double thetaSat = cols.at(thetaSatIdx).trimmed().toDouble(&okThetaSat);
        const double thetaRes = cols.at(thetaResIdx).trimmed().toDouble(&okThetaRes);
        if (!(okDepth && okKsat && okAlpha && okN && okThetaSat && okThetaRes)) {
            continue;
        }

        VnSoftSoilProfileRow row;
        row.depth = depth;
        row.props.ksat = ksat;
        row.props.alpha = alpha;
        row.props.n = n;
        row.props.thetaSat = thetaSat;
        row.props.thetaRes = thetaRes;
        rows->push_back(row);
    }

    std::sort(rows->begin(), rows->end(), [](const VnSoftSoilProfileRow &lhs, const VnSoftSoilProfileRow &rhs) {
        return lhs.depth < rhs.depth;
    });

    return rows->size() >= 2;
}

double InterpolateByDepth(const QVector<VnSoftSoilProfileRow> &rows,
                          double depth,
                          const std::function<double(const VnSoftSoilProfileRow &)> &selector)
{
    if (rows.isEmpty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (depth <= rows.first().depth) {
        return selector(rows.first());
    }
    if (depth >= rows.last().depth) {
        return selector(rows.last());
    }
    for (int i = 1; i < rows.size(); ++i) {
        if (depth <= rows.at(i).depth) {
            const auto &lo = rows.at(i - 1);
            const auto &hi = rows.at(i);
            const double dx = hi.depth - lo.depth;
            if (std::fabs(dx) < 1e-12) {
                return selector(lo);
            }
            const double w = (depth - lo.depth) / dx;
            return selector(lo) + w * (selector(hi) - selector(lo));
        }
    }
    return selector(rows.last());
}

bool IsDefaultVnSoftReferenceOptions(const StarterScriptOptions &options)
{
    constexpr double kEpsilon = 1e-9;
    const auto same = [](double lhs, double rhs) {
        return std::fabs(lhs - rhs) <= kEpsilon;
    };

    return options.vnSoftGridXCount == 16
        && options.vnSoftGridYCount == 15
        && options.vnSoftUwGridXCount == 16
        && options.vnSoftUwGridYCount == 12
        && same(options.vnSoftCellSize, 586.9)
        && same(options.vnSoftUwCellSize, 586.9)
        && same(options.vnSoftGapSize, 0.0)
        && same(options.vnSoftRwG, 1.2192)
        && same(options.vnSoftRwUw, 1.2192)
        && same(options.vnSoftRadiusOfInfluence, 20.0)
        && same(options.vnSoftDepthOfWellC, 4.8768)
        && same(options.vnSoftDepthOfWellG, 7.3152)
        && same(options.vnSoftDepthToGroundWater, 43.2816)
        && same(options.vnSoftTopElevation, -5.0)
        && same(options.vnSoftLayerThickness, 1.0)
        && same(options.vnSoftSoilKsatOriginal, 1.05196)
        && same(options.vnSoftSoilAlpha, 3.47536)
        && same(options.vnSoftSoilN, 1.74582)
        && same(options.vnSoftSoilThetaSat, 0.39)
        && same(options.vnSoftSoilThetaRes, 0.049);
}

bool ShouldUseCanonicalVnSoftReference(const StarterScriptOptions &options)
{
    const QString mode = NormalizeVnSoftSoilParamMode(options.vnSoftSoilParamMode);
    return mode == QStringLiteral("VnReferenceDefaults")
        && IsDefaultVnSoftReferenceOptions(options)
        && options.vnSoilLayersFile.trimmed().isEmpty()
        && options.vnMoistureLayersFile.trimmed().isEmpty();
}

void AppendEmbeddedVnSoftReferenceGridDefault(const StarterScriptOptions &options, QTextStream *ts)
{
    if (ts == nullptr) {
        return;
    }

    QString embedded = VnDrywellBuilder::VnFullReferenceScript();
    ApplyVnKsatScaleOverrides(&embedded, options);

    const QStringList lines = embedded.split('\n', Qt::KeepEmptyParts);
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || !IsSoftReferenceGridLine(trimmed)) {
            continue;
        }
        *ts << line << '\n';
    }
}

void AppendEmbeddedVnSoftReferenceScaffold(const StarterScriptOptions &options, QString *scriptText)
{
    if (scriptText == nullptr) {
        return;
    }

    QString embedded = VnDrywellBuilder::VnFullReferenceScript();
    ApplyVnKsatScaleOverrides(&embedded, options);

    const QStringList filteredLines = embedded
                                          .split('\n', Qt::KeepEmptyParts);
    for (const QString &line : filteredLines) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("loadtemplate;"), Qt::CaseInsensitive)
            || trimmed.startsWith(QStringLiteral("addtemplate;"), Qt::CaseInsensitive)
            || trimmed.contains(QStringLiteral("quantity=simulation_start_time"), Qt::CaseInsensitive)
            || trimmed.contains(QStringLiteral("quantity=simulation_end_time"), Qt::CaseInsensitive)
            || trimmed.contains(QStringLiteral("quantity=outputfile"), Qt::CaseInsensitive)
            || trimmed.contains(QStringLiteral("quantity=numthreads"), Qt::CaseInsensitive)
            || trimmed.contains(QStringLiteral("quantity=number_of_threads"), Qt::CaseInsensitive)
            || trimmed.contains(QStringLiteral("setvalue; object=%1, quantity=inflow").arg(VnDrywellBuilder::InflowTargetObject()), Qt::CaseInsensitive)
            || IsSoftReferenceGridLine(trimmed)) {
            continue;
        }
        *scriptText += line + '\n';
    }

    if (!scriptText->endsWith('\n')) {
        *scriptText += '\n';
    }
}

void AppendEnrichmentPreset(QTextStream &ts,
                            const QString &preset,
                            const QString &inflowFile = QString())
{
    if (preset == QStringLiteral("HQ_Drywell_MonitoringWell")) {
        ts << "\n# enrichment_preset: HQ_Drywell_MonitoringWell\n";
        ts << "create block;type=Well,name=Monitoring_Well,_width=180,_height=180,x=350,y=-120,bottom_elevation=-2[m],depth=4[m],diameter=0.3[m]\n";
        ts << "create link;from=Infiltration_Pond,to=Monitoring_Well,type=soil_to_well_link,name=Pond_to_MonitoringWell\n";
    } else if (preset == QStringLiteral("HQ_Drywell_GroundwaterBoundary")) {
        ts << "\n# enrichment_preset: HQ_Drywell_GroundwaterBoundary\n";
        ts << "create block;type=fixed_head,name=GW,_width=180,_height=180,x=0,y=-420,head=-3[m],Storage=100000[m~^3]\n";
        ts << "create link;from=Infiltration_Pond,to=GW,type=soil_to_fixedhead_link,name=Pond_to_GW\n";
    } else if (preset == QStringLiteral("HQ_Drywell_PretreatmentChambers")) {
        ts << "\n# enrichment_preset: HQ_Drywell_PretreatmentChambers\n";
        ts << "create block;type=Pond,name=Side_Settling_Chamber,_width=180,_height=180,x=-260,y=40,bottom_elevation=0[m],Storage=0[m~^3],alpha=50,beta=2.2\n";
        ts << "create block;type=Pond,name=Sedimentation_Chamber,_width=180,_height=180,x=-120,y=20,bottom_elevation=0[m],Storage=0[m~^3],alpha=60,beta=2.3\n";
        ts << "create link;from=Side_Settling_Chamber,to=Sedimentation_Chamber,type=surfacewater_to_surfacewater_link,name=Pretreat_Link_1\n";
        ts << "create link;from=Sedimentation_Chamber,to=Infiltration_Pond,type=surfacewater_to_surfacewater_link,name=Pretreat_Link_2\n";
    } else if (preset == QStringLiteral("HQ_Drywell_SuiteStyle")) {
        ts << "\n# enrichment_preset: HQ_Drywell_SuiteStyle\n";
        ts << "create block;type=Well,name=Monitoring_Well,_width=180,_height=180,x=350,y=-120,bottom_elevation=-2[m],depth=4[m],diameter=0.3[m]\n";
        ts << "create block;type=fixed_head,name=GW,_width=180,_height=180,x=0,y=-420,head=-3[m],Storage=100000[m~^3]\n";
        ts << "create link;from=Infiltration_Pond,to=Monitoring_Well,type=soil_to_well_link,name=Suite_Pond_to_MonitoringWell\n";
        ts << "create link;from=Infiltration_Pond,to=GW,type=soil_to_fixedhead_link,name=Suite_Pond_to_GW\n";
    } else if (preset == QStringLiteral("HQ_Drywell_LegacyStyle")) {
        ts << "\n# enrichment_preset: HQ_Drywell_LegacyStyle\n";
        ts << "create block;type=Pond,name=Side_Settling_Chamber,_width=180,_height=180,x=-260,y=40,bottom_elevation=0[m],Storage=0[m~^3],alpha=50,beta=2.2\n";
        ts << "create block;type=Pond,name=Sedimentation_Chamber,_width=180,_height=180,x=-120,y=20,bottom_elevation=0[m],Storage=0[m~^3],alpha=60,beta=2.3\n";
        ts << "create block;type=fixed_head,name=GW,_width=180,_height=180,x=0,y=-420,head=-3[m],Storage=100000[m~^3]\n";
        ts << "create link;from=Side_Settling_Chamber,to=Sedimentation_Chamber,type=surfacewater_to_surfacewater_link,name=Legacy_Link_1\n";
        ts << "create link;from=Sedimentation_Chamber,to=Infiltration_Pond,type=surfacewater_to_surfacewater_link,name=Legacy_Link_2\n";
        ts << "create link;from=Infiltration_Pond,to=GW,type=soil_to_fixedhead_link,name=Legacy_Pond_to_GW\n";
    } else if (preset == QStringLiteral("VN_Drywell")) {
        ts << "\n# enrichment_preset: VN_Drywell\n";
        ts << "create block;type=Well_aggregate,name=Well_c,_height=9753.6,"
              "_width=1219.2,bottom_elevation=-4.8768[m],diameter=2.4384[m],"
              "depth=0[m],porosity=1,x=780.8,y=975.36,inflow=" << inflowFile << "\n";
        ts << "create block;type=Well_aggregate,name=Well_g,_height=23408.64,"
              "_width=1219.2,bottom_elevation=-12.192[m],diameter=2.4384[m],"
              "depth=0.01[m],porosity=0.5,x=780.8,y=12192\n";
        ts << "create block;type=junction_elastic,name=Junction_elastic,"
              "_height=1000,_width=1000,x=3000,y=10753.6,elevation=-4.8768[m]\n";
        ts << "create link;from=Well_c,to=Well_g,type=Sewer_pipe,"
              "name=Well_to_well_overflow,ManningCoeff=0.01,diameter=0.2032[m],"
              "length=10[m],start_elevation=-1.8288[m],end_elevation=-8.5344[m]\n";
        ts << "create link;from=Well_c,to=Junction_elastic,type=darcy_connector,"
              "name=Well_to_junction\n";
        ts << "create link;from=Junction_elastic,to=Well_g,type=darcy_connector,"
              "name=Junction_to_well\n";
    } else if (preset == QStringLiteral("VN_Drywell_Pro")) {
        ts << "\n# enrichment_preset: VN_Drywell_Pro\n";
        ts << "create block;type=Pond,name=Infiltration_Pond,_width=200,_height=200,"
              "x=-5971,y=-249,bottom_elevation=0[m],Storage=0[m~^3],alpha=86.061,"
              "beta=2.766\n";
        ts << "create block;type=Well_aggregate,name=Well_c,_height=9753.6,"
              "_width=1219.2,bottom_elevation=-4.8768[m],diameter=2.4384[m],"
              "depth=0[m],porosity=1,x=780.8,y=975.36,inflow=" << inflowFile << "\n";
        ts << "create block;type=Well_aggregate,name=Well_g,_height=23408.64,"
              "_width=1219.2,bottom_elevation=-12.192[m],diameter=2.4384[m],"
              "depth=0.01[m],porosity=0.5,x=780.8,y=12192\n";
        ts << "create block;type=junction_elastic,name=Junction_elastic,"
              "_height=1000,_width=1000,x=3000,y=10753.6,elevation=-4.8768[m]\n";
        ts << "create link;from=Well_c,to=Well_g,type=Sewer_pipe,"
              "name=Well_to_well_overflow,ManningCoeff=0.01,diameter=0.2032[m],"
              "length=10[m],start_elevation=-1.8288[m],end_elevation=-8.5344[m]\n";
        ts << "create link;from=Well_c,to=Junction_elastic,type=darcy_connector,"
              "name=Well_to_junction\n";
        ts << "create link;from=Junction_elastic,to=Well_g,type=darcy_connector,"
              "name=Junction_to_well\n";
    } else if (preset == QStringLiteral("R_Bioswale_Underdrain")) {
        ts << "\n# enrichment_preset: R_Bioswale_Underdrain\n";
        ts << "create block;type=Pipe,name=Underdrain,_width=180,_height=180,x=320,y=-320,diameter=0.15[m],length=40[m],slope=0.01\n";
        ts << "create link;from=Catchment (1),to=Underdrain,type=surfacewater_to_pipe_link,name=Catchment_to_Underdrain\n";
    } else if (preset == QStringLiteral("R_Bioswale_Underdrain_GW")) {
        ts << "\n# enrichment_preset: R_Bioswale_Underdrain_GW\n";
        ts << "create block;type=Pipe,name=Underdrain,_width=180,_height=180,x=320,y=-320,diameter=0.15[m],length=40[m],slope=0.01\n";
        ts << "create block;type=fixed_head,name=GW,_width=180,_height=180,x=420,y=-360,head=-2[m],Storage=100000[m~^3]\n";
        ts << "create link;from=Catchment (1),to=Underdrain,type=surfacewater_to_pipe_link,name=Catchment_to_Underdrain\n";
        ts << "create link;from=Underdrain,to=GW,type=pipe_to_fixedhead_link,name=Underdrain_to_GW\n";
    } else if (preset == QStringLiteral("R_Bioswale_SuiteStyle")) {
        ts << "\n# enrichment_preset: R_Bioswale_SuiteStyle\n";
        ts << "create block;type=Pipe,name=Underdrain,_width=180,_height=180,x=320,y=-320,diameter=0.15[m],length=40[m],slope=0.01\n";
        ts << "create block;type=fixed_head,name=GW,_width=180,_height=180,x=420,y=-360,head=-2[m],Storage=100000[m~^3]\n";
        ts << "create link;from=Catchment (1),to=Underdrain,type=surfacewater_to_pipe_link,name=Suite_Catchment_to_Underdrain\n";
        ts << "create link;from=Underdrain,to=GW,type=pipe_to_fixedhead_link,name=Suite_Underdrain_to_GW\n";
    } else if (preset == QStringLiteral("R_Bioswale_LegacyStyle")) {
        ts << "\n# enrichment_preset: R_Bioswale_LegacyStyle\n";
        ts << "create block;type=Pipe,name=Underdrain,_width=180,_height=180,x=300,y=-300,diameter=0.15[m],length=35[m],slope=0.01\n";
        ts << "create block;type=fixed_head,name=GW,_width=180,_height=180,x=420,y=-360,head=-2[m],Storage=100000[m~^3]\n";
        ts << "create link;from=Catchment (1),to=Underdrain,type=surfacewater_to_pipe_link,name=Legacy_Catchment_to_Underdrain\n";
        ts << "create link;from=Underdrain,to=GW,type=pipe_to_fixedhead_link,name=Legacy_Underdrain_to_GW\n";
    }
}

bool ValidateObservationOptions(const StarterScriptOptions &options, QString *errorMessage)
{
    if (options.observationFile.trimmed().isEmpty()) {
        return true;
    }

    if (options.observationObject.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Observation object is required when observation file is provided.");
        }
        return false;
    }

    if (options.observationExpression.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Observation expression is required when observation file is provided.");
        }
        return false;
    }

    if (options.observationName.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Observation name is required when observation file is provided.");
        }
        return false;
    }

    return true;
}

void AppendObservationIfAny(QTextStream &ts, const StarterScriptOptions &options)
{
    if (options.observationFile.trimmed().isEmpty()) {
        return;
    }

    ts << "create observation;type=Observation,object=" << options.observationObject
       << ",name=" << options.observationName
       << ",expression=" << options.observationExpression
       << ",observed_data=" << options.observationFile
       << ",error_structure=normal,error_standard_deviation=1\n";
}

void AppendAdditionalCommandsIfAny(QTextStream &ts, const StarterScriptOptions &options)
{
    const QString extra = options.additionalCommands.trimmed();
    if (extra.isEmpty()) {
        return;
    }

    ts << "\n# user_additional_commands\n" << extra;
    if (!extra.endsWith('\n')) {
        ts << "\n";
    }
}

void AppendVnSoftReferenceGrid(QTextStream &ts, const StarterScriptOptions &options)
{
    const QString normalizedSoilMode = NormalizeVnSoftSoilParamMode(options.vnSoftSoilParamMode);
    if (normalizedSoilMode == QStringLiteral("VnReferenceDefaults")
        && IsDefaultVnSoftReferenceOptions(options)) {
        // Exact VN Ref mode: emit embedded Full-reference soil/grid content directly
        // so parameters match canonical VN reference values exactly.
        AppendEmbeddedVnSoftReferenceGridDefault(options, &ts);
        return;
    }

    const int gNx = qMax(1, options.vnSoftGridXCount);
    const int gNy = qMax(1, options.vnSoftGridYCount);
    const int uwNx = qMax(1, options.vnSoftUwGridXCount);
    const int uwNy = qMax(1, options.vnSoftUwGridYCount);

    constexpr double kEpsilon = 1e-9;
    const auto differs = [](double lhs, double rhs) {
        return std::fabs(lhs - rhs) > kEpsilon;
    };

    const bool radiiCustomized = differs(options.vnSoftRwG, 1.2192)
        || differs(options.vnSoftRwUw, 1.2192)
        || differs(options.vnSoftRadiusOfInfluence, 20.0);
    const bool geometryFromRadii = radiiCustomized
        && options.vnSoftRadiusOfInfluence > options.vnSoftRwG
        && options.vnSoftRadiusOfInfluence > options.vnSoftRwUw;
    const double gDr = geometryFromRadii
        ? (options.vnSoftRadiusOfInfluence - options.vnSoftRwG) / gNx
        : ((options.vnSoftCellSize > 0.0 ? options.vnSoftCellSize : 586.9) / 500.0);
    const double uwDr = geometryFromRadii
        ? (options.vnSoftRadiusOfInfluence - options.vnSoftRwUw) / uwNx
        : ((options.vnSoftUwCellSize > 0.0 ? options.vnSoftUwCellSize : (gDr * 500.0)) / 500.0);
    const bool validDepthGeometry = options.vnSoftDepthOfWellG > 0.0
        && options.vnSoftDepthToGroundWater > (options.vnSoftDepthOfWellC + options.vnSoftDepthOfWellG);
    const double gLayerThickness = validDepthGeometry
        ? options.vnSoftDepthOfWellG / gNy
        : (options.vnSoftLayerThickness > 0.0 ? options.vnSoftLayerThickness : 1.0);
    const int uwNyTotal = qMax(1, uwNy);
    const double uwLayerThickness = validDepthGeometry
        ? (options.vnSoftDepthToGroundWater - (options.vnSoftDepthOfWellC + options.vnSoftDepthOfWellG)) / uwNyTotal
        : (options.vnSoftLayerThickness > 0.0 ? options.vnSoftLayerThickness : 1.0);
    const double topElevation = options.vnSoftTopElevation;
    const double gap = qMax(0.0, options.vnSoftGapSize);
    const QString gScale = ResolveKsatScaleString(options.ksatScaleG, options.ksatScaleAll, QStringLiteral("1.0"));
    const QString uwScale = ResolveKsatScaleString(options.ksatScaleUw, options.ksatScaleAll, QStringLiteral("1.0"));
    const double depthWellT = options.vnSoftDepthOfWellC + options.vnSoftDepthOfWellG;
    const int assumedNzC = 5;
    const double gwHead = topElevation - options.vnSoftDepthToGroundWater;
    const double uwGapXOffset = gap * 2000.0;
    constexpr double kPi = 3.14159265358979323846;
    // Keep naming aligned with ModelCreator interpolation sources:
    //   SoilData keys: Ksat, alpha, n, theta_s, theta_r
    //   OHQ block fields: K_sat_original, alpha, n, theta_sat, theta_res
    const QString soilMode = normalizedSoilMode;
    const bool useFileProfile = soilMode == QStringLiteral("File");
    const bool useVnReferenceDefaults = soilMode == QStringLiteral("VnReferenceDefaults");
    const bool useModelCreatorDefaults = soilMode == QStringLiteral("ModelCreatorDefaults");
    QVector<VnSoftSoilProfileRow> soilProfileRows;
    QVector<VnSoftSoilProfileRow> vnReferenceGProfileRows;
    QVector<VnSoftSoilProfileRow> vnReferenceUwProfileRows;
    const bool fileProfileLoaded = useFileProfile
        && TryLoadVnSoftSoilProfile(options.vnSoftSoilParameterFile, &soilProfileRows);
    const bool vnReferenceProfileLoaded = useVnReferenceDefaults
        && LoadVnReferenceProfileRows(&vnReferenceGProfileRows, &vnReferenceUwProfileRows);

    // Keep ModelCreator defaults local to script-builder so this module does not
    // depend on UI-side headers or include-path availability.
    constexpr VnSoftSoilProps kModelCreatorDefaults {
        1.05196, 3.47536, 1.74582, 0.39, 0.049
    };
    const VnSoftSoilProps manualProps {
        options.vnSoftSoilKsatOriginal,
        options.vnSoftSoilAlpha,
        options.vnSoftSoilN,
        options.vnSoftSoilThetaSat,
        options.vnSoftSoilThetaRes
    };
    const auto soilPropsAtDepth = [&](double depthFromTop, bool underWellZone) {
        if (fileProfileLoaded) {
            VnSoftSoilProps p;
            p.ksat = InterpolateByDepth(soilProfileRows, depthFromTop, [](const VnSoftSoilProfileRow &r) { return r.props.ksat; });
            p.alpha = InterpolateByDepth(soilProfileRows, depthFromTop, [](const VnSoftSoilProfileRow &r) { return r.props.alpha; });
            p.n = InterpolateByDepth(soilProfileRows, depthFromTop, [](const VnSoftSoilProfileRow &r) { return r.props.n; });
            p.thetaSat = InterpolateByDepth(soilProfileRows, depthFromTop, [](const VnSoftSoilProfileRow &r) { return r.props.thetaSat; });
            p.thetaRes = InterpolateByDepth(soilProfileRows, depthFromTop, [](const VnSoftSoilProfileRow &r) { return r.props.thetaRes; });
            return p;
        }
        if (useVnReferenceDefaults) {
            if (vnReferenceProfileLoaded) {
                const auto &primaryRows = underWellZone ? vnReferenceUwProfileRows : vnReferenceGProfileRows;
                const auto &fallbackRows = underWellZone ? vnReferenceGProfileRows : vnReferenceUwProfileRows;
                const auto &sourceRows = primaryRows.isEmpty() ? fallbackRows : primaryRows;
                if (sourceRows.isEmpty()) {
                    return kModelCreatorDefaults;
                }
                VnSoftSoilProps p;
                p.ksat = InterpolateByDepth(sourceRows, depthFromTop, [](const VnSoftSoilProfileRow &r) { return r.props.ksat; });
                p.alpha = InterpolateByDepth(sourceRows, depthFromTop, [](const VnSoftSoilProfileRow &r) { return r.props.alpha; });
                p.n = InterpolateByDepth(sourceRows, depthFromTop, [](const VnSoftSoilProfileRow &r) { return r.props.n; });
                p.thetaSat = InterpolateByDepth(sourceRows, depthFromTop, [](const VnSoftSoilProfileRow &r) { return r.props.thetaSat; });
                p.thetaRes = InterpolateByDepth(sourceRows, depthFromTop, [](const VnSoftSoilProfileRow &r) { return r.props.thetaRes; });
                return p;
            }
            return kModelCreatorDefaults;
        }
        if (useModelCreatorDefaults) {
            return kModelCreatorDefaults;
        }
        return manualProps;
    };

    ts << "create block;type=fixed_head,name=Ground Water,_width=" << (options.vnSoftRadiusOfInfluence * 1000.0)
       << ",_height=500,x=" << (-uwNx * 1000.0)
       << ",y=" << (37000.0 + (assumedNzC + gNy + uwNy) * 2000.0)
       << ",head=" << gwHead << ",Storage=100000\n";

    for (int y = 0; y < gNy; ++y) {
        for (int x = 0; x < gNx; ++x) {
            const double bottom = (topElevation - options.vnSoftDepthOfWellC) - ((y + 1) * gLayerThickness);
            const double actualDepth = (y + 0.5) * gLayerThickness + options.vnSoftDepthOfWellC;
            const VnSoftSoilProps props = soilPropsAtDepth(actualDepth, false);
            const double r1 = options.vnSoftRwG + x * gDr;
            const double r2 = options.vnSoftRwG + (x + 1) * gDr;
            const double area = kPi * (r2 * r2 - r1 * r1);
            ts << "create block;type=Soil,name=Soil-g (" << (x + 1) << "$" << y << "),"
               << "_width=" << (gDr * 500.0) << ",_height=" << (gDr * 500.0)
               << ",x=" << (-(x * gDr + options.vnSoftRwG) * 2000.0)
               << ",y=" << (y * gLayerThickness * 3000.0 + options.vnSoftDepthOfWellC * 2800.0)
               << ",act_X=" << ((x + 0.5) * gDr + options.vnSoftRwG)
               << ",act_Y=" << (-(y + 0.5) * gLayerThickness - options.vnSoftDepthOfWellC)
               << ",area=" << area
               << ",bottom_elevation=" << bottom << "[m],depth=" << gLayerThickness << "[m],"
               << "specific_storage=0.01,theta=0.2,theta_res=" << props.thetaRes
               << ",theta_sat=" << props.thetaSat
               << ",K_sat_original=" << props.ksat
               << ",K_sat_scale_factor=" << gScale
               << ",alpha=" << props.alpha
               << ",n=" << props.n
               << ",L=-0.5\n";
        }
    }
    for (int y = 0; y < uwNy; ++y) {
        for (int x = 0; x < uwNx; ++x) {
            const double bottom = (topElevation - depthWellT) - ((y + 1) * uwLayerThickness);
            const double actualDepth = (y + 0.5) * uwLayerThickness + depthWellT;
            const VnSoftSoilProps props = soilPropsAtDepth(actualDepth, true);
            const double r1 = options.vnSoftRwUw + x * uwDr;
            const double r2 = options.vnSoftRwUw + (x + 1) * uwDr;
            const double area = kPi * (r2 * r2 - r1 * r1);
            ts << "create block;type=Soil,name=Soil-uw (" << (x + 1) << "$" << y << "),"
               << "_width=" << (uwDr * 500.0) << ",_height=" << (uwDr * 500.0)
               << ",x=" << (-(x * uwDr + options.vnSoftRwUw) * 2000.0 - uwGapXOffset)
               << ",y=" << (37000.0 + (y * uwLayerThickness) * 2000.0)
               << ",act_X=" << ((x + 0.5) * uwDr + options.vnSoftRwUw)
               << ",act_Y=" << (-(y + 0.5) * uwLayerThickness - depthWellT)
               << ",area=" << area
               << ",bottom_elevation=" << bottom << "[m],depth=" << uwLayerThickness << "[m],"
               << "specific_storage=0.01,theta=0.2,theta_res=" << props.thetaRes
               << ",theta_sat=" << props.thetaSat
               << ",K_sat_original=" << props.ksat
               << ",K_sat_scale_factor=" << uwScale
               << ",alpha=" << props.alpha
               << ",n=" << props.n
               << ",L=-0.5\n";
        }
        const double bottomCenter = (topElevation - depthWellT) - ((y + 1) * uwLayerThickness);
        const double actualDepthCenter = (y + 0.5) * uwLayerThickness + depthWellT;
        const VnSoftSoilProps centerProps = soilPropsAtDepth(actualDepthCenter, true);
        const double centerArea = kPi * options.vnSoftRwUw * options.vnSoftRwUw;
        ts << "create block;type=Soil,name=Soil-uw (0$" << y << "),"
           << "_width=" << (uwDr * 500.0) << ",_height=" << (uwDr * 500.0)
           << ",x=" << (-options.vnSoftRwUw * 1000.0 + 2000.0 - uwGapXOffset)
           << ",y=" << (37000.0 + (y * uwLayerThickness) * 2000.0)
           << ",act_X=0,act_Y=" << (-(y + 0.5) * uwLayerThickness - depthWellT)
           << ",area=" << centerArea
           << ",bottom_elevation=" << bottomCenter << "[m],depth=" << uwLayerThickness << "[m],"
           << "specific_storage=0.01,theta=0.2,theta_res=" << centerProps.thetaRes
           << ",theta_sat=" << centerProps.thetaSat
           << ",K_sat_original=" << centerProps.ksat
           << ",K_sat_scale_factor=" << uwScale
           << ",alpha=" << centerProps.alpha
           << ",n=" << centerProps.n
           << ",L=-0.5\n";
    }

    for (int y = 0; y < gNy; ++y) {
        for (int x = 1; x < gNx; ++x) {
            ts << "create link;from=Soil-g (" << x << "$" << y << "),to=Soil-g (" << (x + 1) << "$" << y
               << "),type=soil_to_soil_link,name=HL-Soil-g (" << x << "$" << y << ") - Soil-g (" << (x + 1) << "$" << y << ")\n";
        }
    }
    for (int x = 1; x <= gNx; ++x) {
        for (int y = 0; y < gNy - 1; ++y) {
            ts << "create link;from=Soil-g (" << x << "$" << y << "),to=Soil-g (" << x << "$" << (y + 1)
               << "),type=soil_to_soil_link,name=VL-Soil-g (" << x << "$" << y << ") - Soil-g (" << x << "$" << (y + 1) << ")\n";
        }
    }

    for (int y = 0; y < uwNy; ++y) {
        for (int x = 0; x < uwNx; ++x) {
            ts << "create link;from=Soil-uw (" << x << "$" << y << "),to=Soil-uw (" << (x + 1) << "$" << y
               << "),type=soil_to_soil_link,name=HL-Soil-uw (" << x << "$" << y << ") - Soil-uw (" << (x + 1) << "$" << y << ")\n";
        }
    }
    for (int x = 0; x <= uwNx; ++x) {
        for (int y = 0; y < uwNy - 1; ++y) {
            ts << "create link;from=Soil-uw (" << x << "$" << y << "),to=Soil-uw (" << x << "$" << (y + 1)
               << "),type=soil_to_soil_link,name=VL-Soil-uw (" << x << "$" << y << ") - Soil-uw (" << x << "$" << (y + 1) << ")\n";
        }
    }

    if (uwNy > 0 && gNy > 0) {
        for (int x = 1; x <= qMin(gNx, uwNx); ++x) {
            ts << "create link;from=Soil-g (" << x << "$" << (gNy - 1)
               << "),to=Soil-uw (" << x << "$0)"
               << ",type=soil_to_soil_link,name=VL-Soil-g (" << x << "$" << (gNy - 1) << ") - Soil-uw (" << x << "$0)\n";
        }
    }

    for (int y = 0; y < gNy; ++y) {
        ts << "create link;from=Well_g,to=Soil-g (1$" << y
           << "),type=Well2soil horizontal link,length=" << (gDr / 2.0)
           << ",name=HL_Well_g - Soil-g (1$" << y << ")\n";
    }
    if (uwNy > 0) {
        ts << "create link;from=Well_g,to=Soil-uw (0$0),type=Well2soil vertical link,name=VL_Well_g - Soil-uw (0$0)\n";
    }

    for (int x = 0; x <= uwNx; ++x) {
        ts << "create link;from=Soil-uw (" << x << "$" << (uwNy - 1)
           << "),to=Ground Water,type=soil_to_fixedhead_link,name=Soil to Groundwater (" << x << ")\n";
    }
}

} // namespace

QString StarterScriptBuilder::VnReferenceSoilProfileCsv()
{
    QVector<VnSoftSoilProfileRow> gRows;
    QVector<VnSoftSoilProfileRow> uwRows;
    if (!LoadVnReferenceProfileRows(&gRows, &uwRows)) {
        return QString();
    }

    QString out;
    QTextStream ts(&out);
    ts << "zone,act_Y,depth_m,Ksat,alpha,n,theta_sat,theta_res\n";
    for (const auto &row : gRows) {
        ts << "Soil-g,"
           << row.actY << ","
           << row.depth << ","
           << row.props.ksat << ","
           << row.props.alpha << ","
           << row.props.n << ","
           << row.props.thetaSat << ","
           << row.props.thetaRes << "\n";
    }
    for (const auto &row : uwRows) {
        ts << "Soil-uw,"
           << row.actY << ","
           << row.depth << ","
           << row.props.ksat << ","
           << row.props.alpha << ","
           << row.props.n << ","
           << row.props.thetaSat << ","
           << row.props.thetaRes << "\n";
    }
    return out;
}

bool StarterScriptBuilder::BuildText(const StarterScriptOptions &options,
                                     QString *scriptText,
                                     QString *errorMessage)
{
    if (scriptText == nullptr) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Internal error: script output buffer is null.");
        }
        return false;
    }

    if (!IsKnownModelType(options.modelType)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Unknown model type: %1").arg(options.modelType);
        }
        return false;
    }

    const QFileInfo templateInfo(options.templateDirectory);
    if (!templateInfo.exists() || !templateInfo.isDir()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Template resources directory is not valid.");
        }
        return false;
    }

    const bool vnModelType = IsVnModel(options.modelType);
    const bool hqModelType = options.modelType.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0;
    const bool rBioswaleModelType = options.modelType.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0;
    const QString vnMode = vnModelType ? NormalizeVnBuildMode(options.vnBuildMode)
                                       : QStringLiteral("Preset");
    const QString hqMode = hqModelType ? NormalizeStructureBuildMode(options.hqBuildMode)
                                       : QStringLiteral("Preset");
    const QString rBioswaleMode = rBioswaleModelType ? NormalizeStructureBuildMode(options.rBioswaleBuildMode)
                                                     : QStringLiteral("Preset");

    const bool directScriptMode = (hqModelType && (hqMode == QStringLiteral("FullReference")
                                                   || hqMode == QStringLiteral("LoadFromOhq")
                                                   || hqMode == QStringLiteral("Preset")))
        || (rBioswaleModelType && (rBioswaleMode == QStringLiteral("FullReference")
                                   || rBioswaleMode == QStringLiteral("LoadFromOhq")
                                   || rBioswaleMode == QStringLiteral("Preset")));

    const QStringList requiredTemplates = directScriptMode
                                              ? QStringList{}
                                              : ((vnModelType && vnMode == QStringLiteral("FullReference"))
                                                     ? RequiredVnFullReferenceTemplates()
                                                     : RequiredTemplates());

    for (const QString &templateFile : requiredTemplates) {
        const QFileInfo fileInfo(TemplateFile(options.templateDirectory, templateFile));
        if (!fileInfo.exists() || !fileInfo.isFile()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Missing required template file: %1").arg(templateFile);
            }
            return false;
        }
    }

    if (!IsNumber(options.simulationStart) || !IsNumber(options.simulationEnd)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Simulation start/end must be numeric values.");
        }
        return false;
    }

    const auto validateOptionalNumeric = [&](const QString &value, const QString &label) -> bool {
        if (!value.trimmed().isEmpty() && !IsNumber(value)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("%1 must be numeric when provided.").arg(label);
            }
            return false;
        }
        return true;
    };
    if (!validateOptionalNumeric(options.ksatScaleAll, QStringLiteral("Ksat scale (all soils)"))
        || !validateOptionalNumeric(options.ksatScaleG, QStringLiteral("Ksat scale-g"))
        || !validateOptionalNumeric(options.ksatScaleUw, QStringLiteral("Ksat scale-uw"))) {
        return false;
    }

    if ((options.vnSoftCellSize > 0.0 && !std::isfinite(options.vnSoftCellSize))
        || (options.vnSoftUwCellSize > 0.0 && !std::isfinite(options.vnSoftUwCellSize))
        || (options.vnSoftGapSize > 0.0 && !std::isfinite(options.vnSoftGapSize))
        || !std::isfinite(options.vnSoftRwG)
        || !std::isfinite(options.vnSoftRwUw)
        || !std::isfinite(options.vnSoftRadiusOfInfluence)
        || !std::isfinite(options.vnSoftDepthOfWellC)
        || !std::isfinite(options.vnSoftDepthOfWellG)
        || !std::isfinite(options.vnSoftDepthToGroundWater)
        || (options.vnSoftLayerThickness > 0.0 && !std::isfinite(options.vnSoftLayerThickness))
        || !std::isfinite(options.vnSoftTopElevation)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("VN soft-grid controls contain invalid numeric values.");
        }
        return false;
    }

    if (options.outputSeriesFile.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Output series filename is required.");
        }
        return false;
    }

    if (!ValidateObservationOptions(options, errorMessage)) {
        return false;
    }

    QString inflow = options.inflowFile.trimmed();
    if (inflow.isEmpty()) {
        if (vnModelType) {
            inflow = (vnMode == QStringLiteral("FullReference") || vnMode == QStringLiteral("SoftReference"))
                         ? DetectStructureDefaultInflowFile(QStringLiteral("VN_Drywell"), options.templateDirectory)
                         : DefaultVnInflowFile();
        } else if (hqModelType && (hqMode == QStringLiteral("FullReference")
                                   || hqMode == QStringLiteral("SoftReference"))) {
            inflow = DetectStructureDefaultInflowFile(QStringLiteral("HQ_Drywell"), options.templateDirectory);
        } else if (rBioswaleModelType && (rBioswaleMode == QStringLiteral("FullReference")
                                          || rBioswaleMode == QStringLiteral("SoftReference"))) {
            inflow = DetectStructureDefaultInflowFile(QStringLiteral("R_Bioswale"), options.templateDirectory);
        }
    } else if ((vnModelType && IsKnownReferenceInflowForOtherModel(inflow, QStringLiteral("VN_Drywell")))
               || (hqModelType && IsKnownReferenceInflowForOtherModel(inflow, QStringLiteral("HQ_Drywell")))
               || (rBioswaleModelType && IsKnownReferenceInflowForOtherModel(inflow, QStringLiteral("R_Bioswale")))) {
        // Guard against stale inflow defaults carried across model switches in UI state.
        if (vnModelType && (vnMode == QStringLiteral("FullReference") || vnMode == QStringLiteral("SoftReference"))) {
            inflow = DetectStructureDefaultInflowFile(QStringLiteral("VN_Drywell"), options.templateDirectory);
        } else if (hqModelType && (hqMode == QStringLiteral("FullReference") || hqMode == QStringLiteral("SoftReference"))) {
            inflow = DetectStructureDefaultInflowFile(QStringLiteral("HQ_Drywell"), options.templateDirectory);
        } else if (rBioswaleModelType && (rBioswaleMode == QStringLiteral("FullReference") || rBioswaleMode == QStringLiteral("SoftReference"))) {
            inflow = DetectStructureDefaultInflowFile(QStringLiteral("R_Bioswale"), options.templateDirectory);
        }
    }
    const bool inflowRequired = vnModelType
        || (hqModelType && hqMode == QStringLiteral("SoftReference"))
        || (rBioswaleModelType && rBioswaleMode == QStringLiteral("SoftReference"));
    if (inflowRequired && inflow.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Inflow file is required.");
        }
        return false;
    }
    {
        const QFileInfo inflowInfo(inflow);
        if (inflowInfo.isAbsolute() && !inflowInfo.exists()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Inflow file was not found: %1").arg(inflow);
            }
            return false;
        }
    }

    if (hqModelType && hqMode == QStringLiteral("LoadFromOhq")) {
        if (options.hqBaseOhqFile.trimmed().isEmpty()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("HQ LoadFromOhq mode requires hqBaseOhqFile.");
            }
            return false;
        }
        QString hqText;
        if (!LoadEntireFile(options.hqBaseOhqFile, &hqText, errorMessage)) {
            return false;
        }
        if (!hqText.endsWith('\n')) {
            hqText += '\n';
        }
        hqText += QStringLiteral("setvalue; object=system, quantity=simulation_start_time, value=%1\n").arg(options.simulationStart);
        hqText += QStringLiteral("setvalue; object=system, quantity=simulation_end_time, value=%1\n").arg(options.simulationEnd);
        hqText += QStringLiteral("setvalue; object=system, quantity=outputfile, value=%1\n").arg(options.outputSeriesFile);
        const QString hqInflowTarget = HqDrywellBuilder::InflowTargetObject();
        if (!hqInflowTarget.trimmed().isEmpty() && !inflow.isEmpty()) {
            hqText += QStringLiteral("setvalue; object=%1, quantity=inflow, value=%2\n").arg(hqInflowTarget, inflow);
        }
        ApplyCommonScriptFixups(&hqText, inflow);
        *scriptText = hqText;
        return true;
    }

    if (rBioswaleModelType && rBioswaleMode == QStringLiteral("LoadFromOhq")) {
        if (options.rBioswaleBaseOhqFile.trimmed().isEmpty()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("R_Bioswale LoadFromOhq mode requires rBioswaleBaseOhqFile.");
            }
            return false;
        }
        QString rText;
        if (!LoadEntireFile(options.rBioswaleBaseOhqFile, &rText, errorMessage)) {
            return false;
        }
        if (!rText.endsWith('\n')) {
            rText += '\n';
        }
        rText += QStringLiteral("setvalue; object=system, quantity=simulation_start_time, value=%1\n").arg(options.simulationStart);
        rText += QStringLiteral("setvalue; object=system, quantity=simulation_end_time, value=%1\n").arg(options.simulationEnd);
        rText += QStringLiteral("setvalue; object=system, quantity=outputfile, value=%1\n").arg(options.outputSeriesFile);
        const QString rInflowTarget = RBioswaleBuilder::InflowTargetObject();
        if (!rInflowTarget.trimmed().isEmpty() && !inflow.isEmpty()) {
            rText += QStringLiteral("setvalue; object=%1, quantity=inflow, value=%2\n").arg(rInflowTarget, inflow);
        }
        ApplyCommonScriptFixups(&rText, inflow);
        *scriptText = rText;
        return true;
    }

    if (hqModelType && hqMode == QStringLiteral("FullReference")) {
        QString out = HqDrywellBuilder::FullReferenceScript();
        if (!out.endsWith('\n')) {
            out += '\n';
        }
        out += QStringLiteral("setvalue; object=system, quantity=simulation_start_time, value=%1\n").arg(options.simulationStart);
        out += QStringLiteral("setvalue; object=system, quantity=simulation_end_time, value=%1\n").arg(options.simulationEnd);
        out += QStringLiteral("setvalue; object=system, quantity=outputfile, value=%1\n").arg(options.outputSeriesFile);
        const QString hqInflowTarget = HqDrywellBuilder::InflowTargetObject();
        if (!hqInflowTarget.trimmed().isEmpty() && !inflow.isEmpty()) {
            out += QStringLiteral("setvalue; object=%1, quantity=inflow, value=%2\n").arg(hqInflowTarget, inflow);
        }
        ApplyCommonScriptFixups(&out, inflow);
        *scriptText = out;
        return true;
    }

    if (rBioswaleModelType && rBioswaleMode == QStringLiteral("FullReference")) {
        QString out = RBioswaleBuilder::FullReferenceScript();
        if (!out.endsWith('\n')) {
            out += '\n';
        }
        out += QStringLiteral("setvalue; object=system, quantity=simulation_start_time, value=%1\n").arg(options.simulationStart);
        out += QStringLiteral("setvalue; object=system, quantity=simulation_end_time, value=%1\n").arg(options.simulationEnd);
        out += QStringLiteral("setvalue; object=system, quantity=outputfile, value=%1\n").arg(options.outputSeriesFile);
        const QString rInflowTarget = RBioswaleBuilder::InflowTargetObject();
        if (!rInflowTarget.trimmed().isEmpty() && !inflow.isEmpty()) {
            out += QStringLiteral("setvalue; object=%1, quantity=inflow, value=%2\n").arg(rInflowTarget, inflow);
        }
        ApplyCommonScriptFixups(&out, inflow);
        *scriptText = out;
        return true;
    }

    if (hqModelType && hqMode == QStringLiteral("SoftReference")) {
        QString out;
        AppendTemplateLoads(&out, options.templateDirectory, RequiredTemplates());
        const QString embedded = HqDrywellBuilder::FullReferenceScript();
        AppendEmbeddedStructureSoftReferenceScaffold(embedded,
                                                     HqDrywellBuilder::InflowTargetObject(),
                                                     IsHqSoftReferenceSoilLine,
                                                     &out);
        out += QStringLiteral("setvalue; object=system, quantity=simulation_start_time, value=%1\n").arg(options.simulationStart);
        out += QStringLiteral("setvalue; object=system, quantity=simulation_end_time, value=%1\n").arg(options.simulationEnd);
        out += QStringLiteral("setvalue; object=system, quantity=outputfile, value=%1\n").arg(options.outputSeriesFile);
        QTextStream ts(&out);
        ts.seek(out.size());
        ts << "# HQ_Drywell soft reference soil scaffold generated from embedded drywell reference\n";
        const VnSoftSoilProps hqReferenceDefaults { 1.0, 1.0, 1.41, 0.4, 0.05 };
        const VnSoftSoilProps hqResolvedProps = ResolveSoftReferenceSoilOverrides(options, hqReferenceDefaults, false);
        AppendEmbeddedStructureSoftReferenceSoils(
            embedded,
            IsHqSoftReferenceSoilLine,
            [&](const QString &rawLine) -> QString {
                const QString trimmed = rawLine.trimmed();
                if (trimmed.startsWith(QStringLiteral("create block;type=Soil"), Qt::CaseInsensitive)) {
                    return ApplySoilOverridesToLine(rawLine, hqResolvedProps);
                }
                return rawLine;
            },
            &ts);

        const QString extra = options.additionalCommands.trimmed();
        if (!extra.isEmpty()) {
            out += "\n# user_additional_commands\n" + extra;
            if (!extra.endsWith('\n')) {
                out += "\n";
            }
        }
        ApplyCommonScriptFixups(&out, inflow);
        *scriptText = out;
        return true;
    }

    if (rBioswaleModelType && rBioswaleMode == QStringLiteral("SoftReference")) {
        QString out;
        AppendTemplateLoads(&out, options.templateDirectory, RequiredTemplates());
        const QString embedded = RBioswaleBuilder::FullReferenceScript();
        AppendEmbeddedStructureSoftReferenceScaffold(embedded,
                                                     RBioswaleBuilder::InflowTargetObject(),
                                                     IsRBioswaleSoftReferenceSoilLine,
                                                     &out);
        out += QStringLiteral("setvalue; object=system, quantity=simulation_start_time, value=%1\n").arg(options.simulationStart);
        out += QStringLiteral("setvalue; object=system, quantity=simulation_end_time, value=%1\n").arg(options.simulationEnd);
        out += QStringLiteral("setvalue; object=system, quantity=outputfile, value=%1\n").arg(options.outputSeriesFile);
        QTextStream ts(&out);
        ts.seek(out.size());
        ts << "# R_Bioswale soft reference soil scaffold generated from embedded bioswale reference\n";
        const VnSoftSoilProps rReferenceDefaults { 0.25, 3.6, 1.56, 0.43, 0.078 };
        const VnSoftSoilProps rResolvedProps = ResolveSoftReferenceSoilOverrides(options, rReferenceDefaults, false);
        AppendEmbeddedStructureSoftReferenceSoils(
            embedded,
            IsRBioswaleSoftReferenceSoilLine,
            [&](const QString &rawLine) -> QString {
                const QString trimmed = rawLine.trimmed();
                if (trimmed.startsWith(QStringLiteral("create block;type=Soil"), Qt::CaseInsensitive)) {
                    return ApplySoilOverridesToLine(rawLine, rResolvedProps);
                }
                return rawLine;
            },
            &ts);

        const QString extra = options.additionalCommands.trimmed();
        if (!extra.isEmpty()) {
            out += "\n# user_additional_commands\n" + extra;
            if (!extra.endsWith('\n')) {
                out += "\n";
            }
        }
        ApplyCommonScriptFixups(&out, inflow);
        *scriptText = out;
        return true;
    }

    if (hqModelType && hqMode == QStringLiteral("Preset")) {
        QString out = HqDrywellBuilder::FullReferenceScript();
        if (!out.endsWith('\n')) {
            out += '\n';
        }
        out += QStringLiteral("setvalue; object=system, quantity=simulation_start_time, value=%1\n").arg(options.simulationStart);
        out += QStringLiteral("setvalue; object=system, quantity=simulation_end_time, value=%1\n").arg(options.simulationEnd);
        out += QStringLiteral("setvalue; object=system, quantity=outputfile, value=%1\n").arg(options.outputSeriesFile);
        const QString hqInflowTarget = HqDrywellBuilder::InflowTargetObject();
        if (!hqInflowTarget.trimmed().isEmpty() && !inflow.isEmpty()) {
            out += QStringLiteral("setvalue; object=%1, quantity=inflow, value=%2\n").arg(hqInflowTarget, inflow);
        }
        const QString extra = options.additionalCommands.trimmed();
        if (!extra.isEmpty()) {
            out += "\n# user_additional_commands\n" + extra;
            if (!extra.endsWith('\n')) {
                out += "\n";
            }
        }
        ApplyCommonScriptFixups(&out, inflow);
        *scriptText = out;
        return true;
    }

    if (rBioswaleModelType && rBioswaleMode == QStringLiteral("Preset")) {
        QString out = RBioswaleBuilder::FullReferenceScript();
        if (!out.endsWith('\n')) {
            out += '\n';
        }
        out += QStringLiteral("setvalue; object=system, quantity=simulation_start_time, value=%1\n").arg(options.simulationStart);
        out += QStringLiteral("setvalue; object=system, quantity=simulation_end_time, value=%1\n").arg(options.simulationEnd);
        out += QStringLiteral("setvalue; object=system, quantity=outputfile, value=%1\n").arg(options.outputSeriesFile);
        const QString rInflowTarget = RBioswaleBuilder::InflowTargetObject();
        if (!rInflowTarget.trimmed().isEmpty() && !inflow.isEmpty()) {
            out += QStringLiteral("setvalue; object=%1, quantity=inflow, value=%2\n").arg(rInflowTarget, inflow);
        }
        const QString extra = options.additionalCommands.trimmed();
        if (!extra.isEmpty()) {
            out += "\n# user_additional_commands\n" + extra;
            if (!extra.endsWith('\n')) {
                out += "\n";
            }
        }
        ApplyCommonScriptFixups(&out, inflow);
        *scriptText = out;
        return true;
    }

    if (vnModelType && vnMode == QStringLiteral("LoadFromOhq")) {
        if (options.vnBaseOhqFile.trimmed().isEmpty()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("VN LoadFromOhq mode requires vnBaseOhqFile.");
            }
            return false;
        }

        QString vnBaseText;
        if (!LoadEntireFile(options.vnBaseOhqFile, &vnBaseText, errorMessage)) {
            return false;
        }
        ApplyVnKsatScaleOverrides(&vnBaseText, options);

        if (!vnBaseText.endsWith('\n')) {
            vnBaseText += '\n';
        }
        vnBaseText += QStringLiteral("setvalue; object=system, quantity=simulation_start_time, value=%1\n")
                          .arg(options.simulationStart);
        vnBaseText += QStringLiteral("setvalue; object=system, quantity=simulation_end_time, value=%1\n")
                          .arg(options.simulationEnd);
        vnBaseText += QStringLiteral("setvalue; object=system, quantity=outputfile, value=%1\n")
                          .arg(options.outputSeriesFile);
        if (!inflow.isEmpty()) {
            vnBaseText += QStringLiteral("setvalue; object=%1, quantity=inflow, value=%2\n")
                              .arg(VnDrywellBuilder::InflowTargetObject(), inflow);
        }

        if (!AppendSnippetFile(options.vnSoilLayersFile,
                               QStringLiteral("VN soil layers"),
                               &vnBaseText,
                               errorMessage)) {
            return false;
        }

        if (!AppendSnippetFile(options.vnMoistureLayersFile,
                               QStringLiteral("VN moisture layers"),
                               &vnBaseText,
                               errorMessage)) {
            return false;
        }

        if (!options.observationFile.trimmed().isEmpty()) {
            vnBaseText += QStringLiteral(
                "\ncreate observation;type=Observation,object=%1,name=%2,expression=%3,"
                "observed_data=%4,error_structure=normal,error_standard_deviation=1\n")
                    .arg(options.observationObject,
                         options.observationName,
                         options.observationExpression,
                         options.observationFile);
        }

        const QString extra = options.additionalCommands.trimmed();
        if (!extra.isEmpty()) {
            vnBaseText += "\n\n# additional_commands\n" + extra + "\n";
        }

        ApplyCommonScriptFixups(&vnBaseText, inflow);
        *scriptText = vnBaseText;
        return true;
    }

    if (vnModelType && vnMode == QStringLiteral("FullReference")) {
        QString out;
        QString vnFileHeader;
        QString vnFileError;
        if (VnDrywellBuilder::Build(options, &vnFileHeader, &vnFileError)) {
            out += vnFileHeader;
            if (!out.endsWith('\n')) {
                out += '\n';
            }
        }
        AppendTemplateLoads(&out, options.templateDirectory, RequiredVnFullReferenceTemplates());
        AppendEmbeddedVnFullReferenceScript(options, &out);
        out += QStringLiteral("setvalue; object=system, quantity=simulation_start_time, value=%1\n")
                   .arg(options.simulationStart);
        out += QStringLiteral("setvalue; object=system, quantity=simulation_end_time, value=%1\n")
                   .arg(options.simulationEnd);
        out += QStringLiteral("setvalue; object=system, quantity=outputfile, value=%1\n")
                   .arg(options.outputSeriesFile);
        out += QStringLiteral("setvalue; object=%1, quantity=inflow, value=%2\n")
                   .arg(VnDrywellBuilder::InflowTargetObject(), inflow);

        if (!options.observationFile.trimmed().isEmpty()) {
            out += QStringLiteral(
                "\ncreate observation;type=Observation,object=%1,name=%2,expression=%3,"
                "observed_data=%4,error_structure=normal,error_standard_deviation=1\n")
                    .arg(options.observationObject,
                         options.observationName,
                         options.observationExpression,
                         options.observationFile);
        }

        if (!AppendSnippetFile(options.vnSoilLayersFile,
                               QStringLiteral("VN soil layers"),
                               &out,
                               errorMessage)) {
            return false;
        }

        if (!AppendSnippetFile(options.vnMoistureLayersFile,
                               QStringLiteral("VN moisture layers"),
                               &out,
                               errorMessage)) {
            return false;
        }

        const QString extra = options.additionalCommands.trimmed();
        if (!extra.isEmpty()) {
            out += "\n# user_additional_commands\n" + extra;
            if (!extra.endsWith('\n')) {
                out += "\n";
            }
        }

        ApplyCommonScriptFixups(&out, inflow);
        *scriptText = out;
        return true;
    }

    if (vnModelType && vnMode == QStringLiteral("SoftReference")) {
        QString out;
        AppendTemplateLoads(&out, options.templateDirectory, RequiredVnFullReferenceTemplates());

        if (ShouldUseCanonicalVnSoftReference(options)) {
            AppendEmbeddedVnFullReferenceScript(options, &out);
            out += QStringLiteral("setvalue; object=system, quantity=simulation_start_time, value=%1\n")
                       .arg(options.simulationStart);
            out += QStringLiteral("setvalue; object=system, quantity=simulation_end_time, value=%1\n")
                       .arg(options.simulationEnd);
            out += QStringLiteral("setvalue; object=system, quantity=outputfile, value=%1\n")
                       .arg(options.outputSeriesFile);
            out += QStringLiteral("setvalue; object=%1, quantity=inflow, value=%2\n")
                       .arg(VnDrywellBuilder::InflowTargetObject(), inflow);
        } else {
            AppendEmbeddedVnSoftReferenceScaffold(options, &out);
            QTextStream ts(&out);
            ts.seek(out.size());
            ts << "setvalue; object=system, quantity=simulation_start_time, value=" << options.simulationStart << "\n";
            ts << "setvalue; object=system, quantity=simulation_end_time, value=" << options.simulationEnd << "\n";
            ts << "setvalue; object=system, quantity=outputfile, value=" << options.outputSeriesFile << "\n";
            ts << "setvalue; object=" << VnDrywellBuilder::InflowTargetObject() << ", quantity=inflow, value=" << inflow << "\n";
            ts << "# VN_Drywell soft reference scaffold generated from embedded VN reference + controllable Soil-uw grid\n";
            AppendVnSoftReferenceGrid(ts, options);
        }

        if (!AppendSnippetFile(options.vnSoilLayersFile,
                               QStringLiteral("VN soil layers"),
                               &out,
                               errorMessage)) {
            return false;
        }
        if (!AppendSnippetFile(options.vnMoistureLayersFile,
                               QStringLiteral("VN moisture layers"),
                               &out,
                               errorMessage)) {
            return false;
        }

        if (!options.observationFile.trimmed().isEmpty()) {
            out += QStringLiteral(
                "\ncreate observation;type=Observation,object=%1,name=%2,expression=%3,"
                "observed_data=%4,error_structure=normal,error_standard_deviation=1\n")
                    .arg(options.observationObject,
                         options.observationName,
                         options.observationExpression,
                         options.observationFile);
        }

        const QString extra = options.additionalCommands.trimmed();
        if (!extra.isEmpty()) {
            out += "\n# user_additional_commands\n" + extra;
            if (!extra.endsWith('\n')) {
                out += "\n";
            }
        }
        ApplyVnKsatScaleOverrides(&out, options);
        ApplyCommonScriptFixups(&out, inflow);
        *scriptText = out;
        return true;
    }

    QString enrichmentPreset = options.enrichmentPreset.trimmed();

    if (vnModelType) {
        enrichmentPreset = ResolveVnPreset(options);
    }

    if (!enrichmentPreset.isEmpty() && !IsKnownPreset(enrichmentPreset)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Unknown enrichment preset: %1").arg(enrichmentPreset);
        }
        return false;
    }

    if (!IsPresetCompatibleWithModel(enrichmentPreset, options.modelType)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Enrichment preset '%1' is not compatible with model type '%2'.")
                                .arg(enrichmentPreset, options.modelType);
        }
        return false;
    }

    QString out;
    AppendTemplateLoads(&out, options.templateDirectory, RequiredTemplates());
    QTextStream ts(&out);
    ts.seek(out.size());
    ts << "setvalue; object=system, quantity=simulation_start_time, value=" << options.simulationStart << "\n";
    ts << "setvalue; object=system, quantity=simulation_end_time, value=" << options.simulationEnd << "\n";
    ts << "setvalue; object=system, quantity=outputfile, value=" << options.outputSeriesFile << "\n";

    if (options.modelType.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0) {
        QString bioswaleBase;
        if (!RBioswaleBuilder::AppendBaseInflowBlock(options, inflow, &bioswaleBase, errorMessage)) {
            return false;
        }
        ts << bioswaleBase;
    } else if (vnModelType) {
        ts << "# VN_Drywell base generated via VN preset block\n";
    } else {
        QString hqDrywellBase;
        if (!HqDrywellBuilder::AppendBaseInflowBlock(options, inflow, &hqDrywellBase, errorMessage)) {
            return false;
        }
        ts << hqDrywellBase;
    }

    AppendObservationIfAny(ts, options);
    AppendEnrichmentPreset(ts, enrichmentPreset, inflow);
    AppendAdditionalCommandsIfAny(ts, options);

    *scriptText = out;
    return true;
}

bool StarterScriptBuilder::Write(const StarterScriptOptions &options,
                                 QString *errorMessage)
{
    if (options.outputFile.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Output .ohq file path is empty.");
        }
        return false;
    }

    QString scriptText;
    if (!BuildText(options, &scriptText, errorMessage)) {
        return false;
    }
    PrependStarterMetadata(options, &scriptText);

    QSaveFile outFile(options.outputFile);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Unable to open output file for writing.");
        }
        return false;
    }

    QTextStream ts(&outFile);
    ts << scriptText;

    if (!outFile.commit()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to commit generated script to disk.");
        }
        return false;
    }

    return true;
}
