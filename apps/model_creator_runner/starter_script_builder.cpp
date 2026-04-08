// NOTE: This file is part of the DryWellSuite/OpenHydroQual codebase.
#include "starter_script_builder.h"

#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QTextStream>

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

bool IsNumber(const QString &value)
{
    bool ok = false;
    value.toDouble(&ok);
    return ok;
}

bool IsKnownPreset(const QString &preset)
{
    static const QStringList knownPresets = {
        QStringLiteral("Drywell_MonitoringWell"),
        QStringLiteral("Drywell_GroundwaterBoundary"),
        QStringLiteral("Drywell_PretreatmentChambers"),
        QStringLiteral("Drywell_SuiteStyle"),
        QStringLiteral("Drywell_LegacyStyle"),
        QStringLiteral("VN_Drywell"),
        QStringLiteral("Bioswale_Underdrain"),
        QStringLiteral("Bioswale_Underdrain_GW"),
        QStringLiteral("Bioswale_SuiteStyle"),
        QStringLiteral("Bioswale_LegacyStyle")
    };
    return knownPresets.contains(preset.trimmed());
}

bool IsDrywellLikeModel(const QString &modelType)
{
    return modelType.compare(QStringLiteral("Drywell"), Qt::CaseInsensitive) == 0
        || modelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0;
}

bool IsKnownModelType(const QString &modelType)
{
    return IsDrywellLikeModel(modelType)
        || modelType.compare(QStringLiteral("Bioswale"), Qt::CaseInsensitive) == 0;
}

bool IsPresetCompatibleWithModel(const QString &preset, const QString &modelType)
{
    const QString trimmedPreset = preset.trimmed();
    if (trimmedPreset.isEmpty()) {
        return true;
    }

    const bool drywellModel = IsDrywellLikeModel(modelType);
    const bool bioswaleModel = modelType.compare(QStringLiteral("Bioswale"), Qt::CaseInsensitive) == 0;
    const bool drywellPreset = trimmedPreset.startsWith(QStringLiteral("Drywell_"));
    const bool bioswalePreset = trimmedPreset.startsWith(QStringLiteral("Bioswale_"));

    if ((drywellModel && bioswalePreset) || (bioswaleModel && drywellPreset)) {
        return false;
    }
    if (trimmedPreset == QStringLiteral("VN_Drywell") && !drywellModel) {
        return false;
    }
    return true;
}

void AppendEnrichmentPreset(QTextStream &ts, const QString &preset, const QString &inflowFile = QString())
{
    if (preset == QStringLiteral("Drywell_MonitoringWell")) {
        ts << "\n# enrichment_preset: Drywell_MonitoringWell\n";
        ts << "create block;type=Well,name=Monitoring_Well,_width=180,_height=180,x=350,y=-120,bottom_elevation=-2[m],depth=4[m],diameter=0.3[m]\n";
        ts << "create link;from=Infiltration_Pond,to=Monitoring_Well,type=soil_to_well_link,name=Pond_to_MonitoringWell\n";
    } else if (preset == QStringLiteral("Drywell_GroundwaterBoundary")) {
        ts << "\n# enrichment_preset: Drywell_GroundwaterBoundary\n";
        ts << "create block;type=fixed_head,name=GW,_width=180,_height=180,x=0,y=-420,head=-3[m],Storage=100000[m~^3]\n";
        ts << "create link;from=Infiltration_Pond,to=GW,type=soil_to_fixedhead_link,name=Pond_to_GW\n";
    } else if (preset == QStringLiteral("Drywell_PretreatmentChambers")) {
        ts << "\n# enrichment_preset: Drywell_PretreatmentChambers\n";
        ts << "create block;type=Pond,name=Side_Settling_Chamber,_width=180,_height=180,x=-260,y=40,bottom_elevation=0[m],Storage=0[m~^3],alpha=50,beta=2.2\n";
        ts << "create block;type=Pond,name=Sedimentation_Chamber,_width=180,_height=180,x=-120,y=20,bottom_elevation=0[m],Storage=0[m~^3],alpha=60,beta=2.3\n";
        ts << "create link;from=Side_Settling_Chamber,to=Sedimentation_Chamber,type=surfacewater_to_surfacewater_link,name=Pretreat_Link_1\n";
        ts << "create link;from=Sedimentation_Chamber,to=Infiltration_Pond,type=surfacewater_to_surfacewater_link,name=Pretreat_Link_2\n";
    } else if (preset == QStringLiteral("Drywell_SuiteStyle")) {
        ts << "\n# enrichment_preset: Drywell_SuiteStyle\n";
        ts << "create block;type=Well,name=Monitoring_Well,_width=180,_height=180,x=350,y=-120,bottom_elevation=-2[m],depth=4[m],diameter=0.3[m]\n";
        ts << "create block;type=fixed_head,name=GW,_width=180,_height=180,x=0,y=-420,head=-3[m],Storage=100000[m~^3]\n";
        ts << "create link;from=Infiltration_Pond,to=Monitoring_Well,type=soil_to_well_link,name=Suite_Pond_to_MonitoringWell\n";
        ts << "create link;from=Infiltration_Pond,to=GW,type=soil_to_fixedhead_link,name=Suite_Pond_to_GW\n";
    } else if (preset == QStringLiteral("Drywell_LegacyStyle")) {
        ts << "\n# enrichment_preset: Drywell_LegacyStyle\n";
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
    } else if (preset == QStringLiteral("Bioswale_Underdrain")) {
        ts << "\n# enrichment_preset: Bioswale_Underdrain\n";
        ts << "create block;type=Pipe,name=Underdrain,_width=180,_height=180,x=320,y=-320,diameter=0.15[m],length=40[m],slope=0.01\n";
        ts << "create link;from=Catchment (1),to=Underdrain,type=surfacewater_to_pipe_link,name=Catchment_to_Underdrain\n";
    } else if (preset == QStringLiteral("Bioswale_Underdrain_GW")) {
        ts << "\n# enrichment_preset: Bioswale_Underdrain_GW\n";
        ts << "create block;type=Pipe,name=Underdrain,_width=180,_height=180,x=320,y=-320,diameter=0.15[m],length=40[m],slope=0.01\n";
        ts << "create block;type=fixed_head,name=GW,_width=180,_height=180,x=420,y=-360,head=-2[m],Storage=100000[m~^3]\n";
        ts << "create link;from=Catchment (1),to=Underdrain,type=surfacewater_to_pipe_link,name=Catchment_to_Underdrain\n";
        ts << "create link;from=Underdrain,to=GW,type=pipe_to_fixedhead_link,name=Underdrain_to_GW\n";
    } else if (preset == QStringLiteral("Bioswale_SuiteStyle")) {
        ts << "\n# enrichment_preset: Bioswale_SuiteStyle\n";
        ts << "create block;type=Pipe,name=Underdrain,_width=180,_height=180,x=320,y=-320,diameter=0.15[m],length=40[m],slope=0.01\n";
        ts << "create block;type=fixed_head,name=GW,_width=180,_height=180,x=420,y=-360,head=-2[m],Storage=100000[m~^3]\n";
        ts << "create link;from=Catchment (1),to=Underdrain,type=surfacewater_to_pipe_link,name=Suite_Catchment_to_Underdrain\n";
        ts << "create link;from=Underdrain,to=GW,type=pipe_to_fixedhead_link,name=Suite_Underdrain_to_GW\n";
    } else if (preset == QStringLiteral("Bioswale_LegacyStyle")) {
        ts << "\n# enrichment_preset: Bioswale_LegacyStyle\n";
        ts << "create block;type=Pipe,name=Underdrain,_width=180,_height=180,x=300,y=-300,diameter=0.15[m],length=35[m],slope=0.01\n";
        ts << "create block;type=fixed_head,name=GW,_width=180,_height=180,x=420,y=-360,head=-2[m],Storage=100000[m~^3]\n";
        ts << "create link;from=Catchment (1),to=Underdrain,type=surfacewater_to_pipe_link,name=Legacy_Catchment_to_Underdrain\n";
        ts << "create link;from=Underdrain,to=GW,type=pipe_to_fixedhead_link,name=Legacy_Underdrain_to_GW\n";
    }
}
}

bool StarterScriptBuilder::BuildText(const StarterScriptOptions &options,
                                     QString *scriptText,
                                     QString *errorMessage)
{
    if (scriptText == nullptr) {
        if (errorMessage) *errorMessage = QStringLiteral("Internal error: script output buffer is null.");
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
        if (errorMessage) *errorMessage = QStringLiteral("Template resources directory is not valid.");
        return false;
    }

    for (const QString &templateFile : RequiredTemplates()) {
        const QFileInfo fileInfo(TemplateFile(options.templateDirectory, templateFile));
        if (!fileInfo.exists() || !fileInfo.isFile()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Missing required template file: %1").arg(templateFile);
            }
            return false;
        }
    }

    if (!IsNumber(options.simulationStart) || !IsNumber(options.simulationEnd)) {
        if (errorMessage) *errorMessage = QStringLiteral("Simulation start/end must be numeric values.");
        return false;
    }

    if (options.inflowFile.trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("Inflow file is required.");
        return false;
    }

    if (options.outputSeriesFile.trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("Output series filename is required.");
        return false;
    }

    QString enrichmentPreset = options.enrichmentPreset.trimmed();
    if (enrichmentPreset.isEmpty()
        && options.modelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0) {
        enrichmentPreset = QStringLiteral("VN_Drywell");
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

    if (!options.observationFile.trimmed().isEmpty()) {
        if (options.observationObject.trimmed().isEmpty()) {
            if (errorMessage) *errorMessage = QStringLiteral("Observation object is required when observation file is provided.");
            return false;
        }
        if (options.observationExpression.trimmed().isEmpty()) {
            if (errorMessage) *errorMessage = QStringLiteral("Observation expression is required when observation file is provided.");
            return false;
        }
        if (options.observationName.trimmed().isEmpty()) {
            if (errorMessage) *errorMessage = QStringLiteral("Observation name is required when observation file is provided.");
            return false;
        }
    }

    QString out;
    QTextStream ts(&out);
    ts << "loadtemplate; filename=" << TemplateFile(options.templateDirectory, "main_components.json") << "\n";
    ts << "addtemplate; filename=" << TemplateFile(options.templateDirectory, "Pond_Plugin.json") << "\n";
    ts << "addtemplate; filename=" << TemplateFile(options.templateDirectory, "unsaturated_soil.json") << "\n";
    ts << "addtemplate; filename=" << TemplateFile(options.templateDirectory, "Well.json") << "\n";
    ts << "addtemplate; filename=" << TemplateFile(options.templateDirectory, "Sewer_system.json") << "\n";
    ts << "addtemplate; filename=" << TemplateFile(options.templateDirectory, "soil_evapotranspiration_models.json") << "\n";
    ts << "addtemplate; filename=" << TemplateFile(options.templateDirectory, "evapotranspiration_models.json") << "\n";
    ts << "addtemplate; filename=" << TemplateFile(options.templateDirectory, "pipe_pump_tank.json") << "\n";
    ts << "setvalue; object=system, quantity=simulation_start_time, value=" << options.simulationStart << "\n";
    ts << "setvalue; object=system, quantity=simulation_end_time, value=" << options.simulationEnd << "\n";
    ts << "setvalue; object=system, quantity=outputfile, value=" << options.outputSeriesFile << "\n";

    const QString inflow = options.inflowFile.trimmed();
    if (options.modelType.compare("Bioswale", Qt::CaseInsensitive) == 0) {
        ts << "create block;type=Catchment,_width=200,_height=200,name=Catchment (1),"
              "loss_coefficient=0[1/day],x=0,Evapotranspiration=,Precipitation=,ManningCoeff=0.01,"
              "inflow=" << inflow << ",Slope=0.02,Width=1[m],y=-200,area=1[m~^2],"
              "depression_storage=0[m],depth=0[m],elevation=0[m]\n";
    } else if (options.modelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0) {
        ts << "# VN_Drywell base generated via enrichment preset block\n";
    } else {
        ts << "create block;type=Pond,inflow=" << inflow
           << ",_width=200,Evapotranspiration=,Precipitation=,bottom_elevation=0[m],"
              "Storage=0[m~^3],name=Infiltration_Pond,alpha=86.061,beta=2.766,x=0,y=0,_height=200\n";
    }

    if (!options.observationFile.trimmed().isEmpty()) {
        ts << "create observation;type=Observation,object=" << options.observationObject
           << ",name=" << options.observationName
           << ",expression=" << options.observationExpression
           << ",observed_data=" << options.observationFile
           << ",error_structure=normal,error_standard_deviation=1\n";
    }

    AppendEnrichmentPreset(ts, enrichmentPreset, inflow);

    const QString extra = options.additionalCommands.trimmed();
    if (!extra.isEmpty()) {
        ts << "\n# user_additional_commands\n" << extra;
        if (!extra.endsWith('\n')) {
            ts << "\n";
        }
    }

    *scriptText = out;
    return true;
}

bool StarterScriptBuilder::Write(const StarterScriptOptions &options,
                                 QString *errorMessage)
{
    if (options.outputFile.trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("Output .ohq file path is empty.");
        return false;
    }

    QString scriptText;
    if (!BuildText(options, &scriptText, errorMessage)) {
        return false;
    }

    QSaveFile outFile(options.outputFile);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) *errorMessage = QStringLiteral("Unable to open output file for writing.");
        return false;
    }

    QTextStream ts(&outFile);
    ts << scriptText;

    if (!outFile.commit()) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to commit generated script to disk.");
        return false;
    }

    return true;
}
