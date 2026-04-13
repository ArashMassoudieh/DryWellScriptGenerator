#include "structure_registry.h"

namespace {

const QStringList kModelTypes = {
    QStringLiteral("VN_Drywell"),
    QStringLiteral("R_Bioswale"),
    QStringLiteral("HQ_Drywell")
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
    QStringLiteral("R_Bioswale_LegacyStyle")
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
    const bool hqDrywellPreset = trimmedPreset.startsWith(QStringLiteral("HQ_Drywell_"));
    const bool rBioswalePreset = trimmedPreset.startsWith(QStringLiteral("R_Bioswale_"));

    if ((hqDrywellModel && rBioswalePreset) || (rBioswaleModel && hqDrywellPreset)) {
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
            {QStringLiteral("HQ build mode: Preset"), QStringLiteral("HQ_MODE:Preset")},
            {QStringLiteral("HQ build mode: FullReference"), QStringLiteral("HQ_MODE:FullReference")},
            {QStringLiteral("HQ build mode: LoadFromOhq"), QStringLiteral("HQ_MODE:LoadFromOhq")},
            {QStringLiteral("HQ_Drywell (DryWellSuite style)"), QStringLiteral("HQ_Drywell_SuiteStyle")},
            {QStringLiteral("HQ_Drywell (Legacy ScriptGenerator style)"), QStringLiteral("HQ_Drywell_LegacyStyle")},
            {QStringLiteral("HQ_Drywell + Monitoring Well"), QStringLiteral("HQ_Drywell_MonitoringWell")},
            {QStringLiteral("HQ_Drywell + Groundwater Boundary"), QStringLiteral("HQ_Drywell_GroundwaterBoundary")},
            {QStringLiteral("HQ_Drywell + Pretreatment Chambers"), QStringLiteral("HQ_Drywell_PretreatmentChambers")}
        };
    }

    if (IsVnModel(modelType)) {
        return {
            {QStringLiteral("VN build mode: SoftReference (editable default)"), QStringLiteral("VN_MODE:SoftReference")},
            {QStringLiteral("VN build mode: FullReference (embedded canonical)"), QStringLiteral("VN_MODE:FullReference")},
            {QStringLiteral("VN build mode: LoadFromOhq (use VN base file)"), QStringLiteral("VN_MODE:LoadFromOhq")},
            {QStringLiteral("VN preset: DryWellSuite Pro default"), QStringLiteral("VN_Drywell_Pro")},
            {QStringLiteral("VN preset: legacy structure"), QStringLiteral("VN_Drywell")}
        };
    }

    return {
        {QStringLiteral("R build mode: Preset"), QStringLiteral("R_MODE:Preset")},
        {QStringLiteral("R build mode: FullReference"), QStringLiteral("R_MODE:FullReference")},
        {QStringLiteral("R build mode: LoadFromOhq"), QStringLiteral("R_MODE:LoadFromOhq")},
        {QStringLiteral("R_Bioswale (DryWellSuite style)"), QStringLiteral("R_Bioswale_SuiteStyle")},
        {QStringLiteral("R_Bioswale (Legacy ScriptGenerator style)"), QStringLiteral("R_Bioswale_LegacyStyle")},
        {QStringLiteral("R_Bioswale + Underdrain"), QStringLiteral("R_Bioswale_Underdrain")},
        {QStringLiteral("R_Bioswale + Underdrain + Groundwater"), QStringLiteral("R_Bioswale_Underdrain_GW")}
    };
}
