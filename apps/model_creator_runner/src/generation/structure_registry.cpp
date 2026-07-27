#include "structure_registry.h"

namespace {

const QStringList kModelTypes = {
    QStringLiteral("VN_Drywell"),
    QStringLiteral("R_Bioswale"),
    QStringLiteral("HQ_Drywell"),
    QStringLiteral("JM_Bioretention")
};

const QStringList kAllPresets = {
    QStringLiteral("HQ_Drywell_MonitoringWell"),
    QStringLiteral("HQ_Drywell_GroundwaterBoundary"),
    QStringLiteral("HQ_Drywell_PretreatmentChambers"),
    QStringLiteral("HQ_Drywell_SuiteStyle"),
    QStringLiteral("HQ_Drywell_LegacyStyle"),
    QStringLiteral("VN_Drywell"),
    QStringLiteral("VN_Drywell_Pro"),
    QStringLiteral("R_Bioswale_Underdrain"),
    QStringLiteral("R_Bioswale_Underdrain_GW"),
    QStringLiteral("R_Bioswale_SuiteStyle"),
    QStringLiteral("R_Bioswale_LegacyStyle"),
    QStringLiteral("JM_Bioretention_Underdrain"),
    QStringLiteral("JM_Bioretention_GW")
};

bool IsVnModel(const QString &modelType)
{
    return modelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0;
}

bool IsHqDrywellLikeModel(const QString &modelType)
{
    return modelType.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0
        || IsVnModel(modelType);
}

} // namespace

QStringList StructureRegistry::ModelTypes()
{
    return kModelTypes;
}

bool StructureRegistry::IsKnownModelType(const QString &modelType)
{
    for (const QString &known : kModelTypes) {
        if (modelType.compare(known, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

bool StructureRegistry::IsKnownPreset(const QString &preset)
{
    return kAllPresets.contains(preset.trimmed());
}

bool StructureRegistry::IsPresetCompatibleWithModel(const QString &preset, const QString &modelType)
{
    const QString trimmedPreset = preset.trimmed();
    if (trimmedPreset.isEmpty()) {
        return true;
    }

    const bool hqDrywellModel = IsHqDrywellLikeModel(modelType);
    const bool rBioswaleModel = modelType.compare(QStringLiteral("R_Bioswale"), Qt::CaseInsensitive) == 0;
    const bool jmModel = modelType.compare(QStringLiteral("JM_Bioretention"), Qt::CaseInsensitive) == 0;
    const bool hqDrywellPreset = trimmedPreset.startsWith(QStringLiteral("HQ_Drywell_"));
    const bool rBioswalePreset = trimmedPreset.startsWith(QStringLiteral("R_Bioswale_"));
    const bool jmPreset = trimmedPreset.startsWith(QStringLiteral("JM_Bioretention_"));

    if ((hqDrywellModel && (rBioswalePreset || jmPreset))
        || (rBioswaleModel && (hqDrywellPreset || jmPreset))
        || (jmModel && (hqDrywellPreset || rBioswalePreset))) {
        return false;
    }

    if ((trimmedPreset == QStringLiteral("VN_Drywell")
         || trimmedPreset == QStringLiteral("VN_Drywell_Pro")) && !IsVnModel(modelType)) {
        return false;
    }

    return true;
}

QList<QPair<QString, QString>> StructureRegistry::PresetOptionsForModel(const QString &modelType)
{
    if (modelType.compare(QStringLiteral("HQ_Drywell"), Qt::CaseInsensitive) == 0) {
        return {
            {QStringLiteral("SoftReference"), QStringLiteral("HQ_MODE:SoftReference")},
            {QStringLiteral("LoadFromOhq"), QStringLiteral("HQ_MODE:LoadFromOhq")},
            {QStringLiteral("FullReference"), QStringLiteral("HQ_MODE:FullReference")}
            // {QStringLiteral("HQ_Drywell_SuiteStyle"), QStringLiteral("HQ_Drywell_SuiteStyle")},
            // {QStringLiteral("HQ_Drywell_LegacyStyle"), QStringLiteral("HQ_Drywell_LegacyStyle")},
            // {QStringLiteral("HQ_Drywell_MonitoringWell"), QStringLiteral("HQ_Drywell_MonitoringWell")},
            // {QStringLiteral("HQ_Drywell_GroundwaterBoundary"), QStringLiteral("HQ_Drywell_GroundwaterBoundary")},
            // {QStringLiteral("HQ_Drywell_PretreatmentChambers"), QStringLiteral("HQ_Drywell_PretreatmentChambers")}
        };
    }

    if (IsVnModel(modelType)) {
        return {
            {QStringLiteral("SoftReference"), QStringLiteral("VN_MODE:SoftReference")},
            {QStringLiteral("LoadFromOhq"), QStringLiteral("VN_MODE:LoadFromOhq")},
            {QStringLiteral("FullReference"), QStringLiteral("VN_MODE:FullReference")}
            // {QStringLiteral("VN_Drywell_Pro"), QStringLiteral("VN_Drywell_Pro")},
            // {QStringLiteral("VN_Drywell"), QStringLiteral("VN_Drywell")}
        };
    }

    if (modelType.compare(QStringLiteral("JM_Bioretention"), Qt::CaseInsensitive) == 0) {
        return {
            {QStringLiteral("SoftReference"), QStringLiteral("JM_MODE:SoftReference")},
            {QStringLiteral("LoadFromOhq"), QStringLiteral("JM_MODE:LoadFromOhq")},
            {QStringLiteral("FullReference"), QStringLiteral("JM_MODE:FullReference")}
        };
    }

    return {
        {QStringLiteral("SoftReference"), QStringLiteral("R_MODE:SoftReference")},
        {QStringLiteral("LoadFromOhq"), QStringLiteral("R_MODE:LoadFromOhq")},
        {QStringLiteral("FullReference"), QStringLiteral("R_MODE:FullReference")}
        // {QStringLiteral("R_Bioswale_SuiteStyle"), QStringLiteral("R_Bioswale_SuiteStyle")},
        // {QStringLiteral("R_Bioswale_LegacyStyle"), QStringLiteral("R_Bioswale_LegacyStyle")},
        // {QStringLiteral("R_Bioswale_Underdrain"), QStringLiteral("R_Bioswale_Underdrain")},
        // {QStringLiteral("R_Bioswale_Underdrain_GW"), QStringLiteral("R_Bioswale_Underdrain_GW")}
    };
}
