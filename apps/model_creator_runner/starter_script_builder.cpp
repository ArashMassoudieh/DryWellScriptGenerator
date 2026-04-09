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

bool AppendSnippetFile(const QString &path, const QString &label, QString *scriptText, QString *errorMessage)
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
    static const QStringList knownPresets = {
        QStringLiteral("Drywell_MonitoringWell"),
        QStringLiteral("Drywell_GroundwaterBoundary"),
        QStringLiteral("Drywell_PretreatmentChambers"),
        QStringLiteral("Drywell_SuiteStyle"),
        QStringLiteral("Drywell_LegacyStyle"),
        QStringLiteral("VN_Drywell"),
        QStringLiteral("VN_Drywell_Pro"),
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
    const bool vnModel = modelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0;
    if ((trimmedPreset == QStringLiteral("VN_Drywell")
         || trimmedPreset == QStringLiteral("VN_Drywell_Pro")) && !vnModel) {
        return false;
    }
    return true;
}

void AppendVnSuiteProDeterministicSoils(QTextStream &ts)
{
    // Match VN-DrywellOHQ baseline geometry used by DryWellSuite Pro defaults.
    const double wellRadius = 1.2192;   // 4 ft
    const double pondRadius = 20.0;     // m
    const int nr = 20;
    const int nLayers = 5;
    const int nLayerDeep = 45;
    const double wellDepth = 12.192;    // 40 ft
    const double depthToGw = 43.2816;   // 142 ft
    const double surfaceElevation = 0.0;
    const double dr = (pondRadius - wellRadius) / static_cast<double>(nr);
    const double dy = wellDepth / static_cast<double>(nLayers);
    const double dyDeep = (depthToGw - wellDepth) / static_cast<double>(nLayerDeep);
    const double pi = 3.1415;

    ts << "\n# VN_Drywell_Pro deterministic soil layers\n";
    for (int r = 0; r < nr; ++r) {
        const double rIn = r * dr + wellRadius;
        const double rOut = (r + 1) * dr + wellRadius;
        const double area = pi * (rOut * rOut - rIn * rIn);
        for (int layer = 0; layer < nLayers; ++layer) {
            const double x = 200 + r * 300;
            const double y = 300 + layer * 300;
            const double bottom = -dy * (layer + 1);
            const double actualY = surfaceElevation - dy * (layer + 0.5);
            ts << "create block;type=Soil,theta_sat=0.4,theta_res=0.05,specific_storage=0.01,x=" << x
               << ",Evapotranspiration=,n=1.41,y=" << y
               << ",area=" << area
               << ",theta=0.1343,K_sat_original=1,_width=200,alpha=1,name=Soil (" << (layer + 1)
               << "$" << (r + 1) << "),_height=100,bottom_elevation=" << bottom
               << ",depth=" << dy
               << ",actual_x=" << (0.5 * (rIn + rOut))
               << ",actual_y=" << actualY << "\n";
        }
    }

    for (int r = 0; r < nr; ++r) {
        const double rIn = r * dr + wellRadius;
        const double rOut = (r + 1) * dr + wellRadius;
        const double area = pi * (rOut * rOut - rIn * rIn);
        for (int layer = 0; layer < nLayerDeep; ++layer) {
            const double x = 200 + r * 300;
            const double y = 300 + layer * 300 + nLayers * 300;
            const double bottom = -dyDeep * (layer + 1) - wellDepth;
            const double actualY = surfaceElevation - dyDeep * (layer + 0.5) - wellDepth;
            ts << "create block;type=Soil,theta_sat=0.4,theta_res=0.05,specific_storage=0.01,x=" << x
               << ",Evapotranspiration=,n=1.41,y=" << y
               << ",area=" << area
               << ",theta=0.1343,K_sat_original=1,_width=200,alpha=1,name=SoilDeep (" << (layer + 1)
               << "$" << (r + 1) << "),_height=100,bottom_elevation=" << bottom
               << ",depth=" << dyDeep
               << ",actual_x=" << (0.5 * (rIn + rOut))
               << ",actual_y=" << actualY << "\n";
        }
    }
}

void AppendVnDrywellProReferenceScript(QTextStream &ts,
                                       const StarterScriptOptions &options,
                                       const QString &inflowFile)
{
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
    ts << "setvalue; object=system, quantity=shakescalered, value=0.75\n";
    ts << "setvalue; object=system, quantity=shakescale, value=0.05\n";
    ts << "setvalue; object=system, quantity=pmute, value=0.02\n";
    ts << "setvalue; object=system, quantity=ngen, value=40\n";
    ts << "setvalue; object=system, quantity=pcross, value=1\n";
    ts << "setvalue; object=system, quantity=outputfile, value=" << options.outputSeriesFile << "\n";
    ts << "setvalue; object=system, quantity=maxpop, value=40\n";
    ts << "setvalue; object=system, quantity=write_solution_details, value=No\n";
    ts << "setvalue; object=system, quantity=nr_tolerance, value=0.001\n";
    ts << "setvalue; object=system, quantity=nr_timestep_reduction_factor_fail, value=0.2\n";
    ts << "setvalue; object=system, quantity=nr_timestep_reduction_factor, value=0.75\n";
    ts << "setvalue; object=system, quantity=n_threads, value=4\n";
    ts << "setvalue; object=system, quantity=minimum_timestep, value=1e-06\n";
    ts << "setvalue; object=system, quantity=initial_time_step, value=0.01\n";
    ts << "setvalue; object=system, quantity=c_n_weight, value=1\n";
    ts << "setvalue; object=system, quantity=maximum_time_allowed, value=4800\n";
    ts << "create block;type=Pond,inflow=" << inflowFile
       << ",_width=200,Evapotranspiration=,Precipitation=,bottom_elevation=0[m],"
          "Storage=0[m~^3],name=Infiltration_Pond,alpha=86.061,beta=2.766,x=-5971,y=-249,_height=200\n";
    ts << "create block;type=Well_aggregate,name=Well_c,_height=9753.6,"
          "_width=1219.2,bottom_elevation=-4.8768[m],diameter=2.4384[m],"
          "depth=0[m],porosity=1,x=780.8,y=975.36\n";
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
    ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_1,low=5,high=10\n";
    ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_2,low=5,high=10\n";
    ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_3,low=5,high=10\n";
    ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_4,low=5,high=10\n";
    ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_5,low=5,high=10\n";
    ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_6,low=5,high=10\n";
    ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_7,low=5,high=10\n";
    ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_8,low=5,high=10\n";
    ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_9,low=5,high=10\n";
    ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_10,low=5,high=10\n";
    ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_11,low=5,high=10\n";
    ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_12,low=5,high=10\n";
    ts << "create parameter;type=Parameter,value=0.26,prior_distribution=log-normal,name=alpha,low=0.00001,high=10\n";
    ts << "create parameter;type=Parameter,value=0.26,prior_distribution=log-normal,name=new_Van_alpha,low=0.00001,high=10\n";
    ts << "create parameter;type=Parameter,value=2,prior_distribution=log-normal,name=beta,low=0.5,high=5\n";
    ts << "create parameter;type=Parameter,value=2,prior_distribution=log-normal,name=theta_t,low=0.01,high=0.13\n";
    ts << "create parameter;type=Parameter,value=100,prior_distribution=log-normal,name=Transmissivity_Coeff_Drywell,low=50,high=500\n";
    ts << "create parameter;type=Parameter,value=100,prior_distribution=log-normal,name=Transmissivity_Coeff_Sed_Chamber,low=20,high=500\n";
    AppendVnSuiteProDeterministicSoils(ts);
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
    } else if (preset == QStringLiteral("VN_Drywell_Pro")) {
        ts << "\n# enrichment_preset: VN_Drywell_Pro\n";
        ts << "setvalue; object=system, quantity=shakescalered, value=0.75\n";
        ts << "setvalue; object=system, quantity=shakescale, value=0.05\n";
        ts << "setvalue; object=system, quantity=pmute, value=0.02\n";
        ts << "setvalue; object=system, quantity=ngen, value=40\n";
        ts << "setvalue; object=system, quantity=pcross, value=1\n";
        ts << "setvalue; object=system, quantity=maxpop, value=40\n";
        ts << "setvalue; object=system, quantity=write_solution_details, value=No\n";
        ts << "setvalue; object=system, quantity=nr_tolerance, value=0.001\n";
        ts << "setvalue; object=system, quantity=nr_timestep_reduction_factor_fail, value=0.2\n";
        ts << "setvalue; object=system, quantity=nr_timestep_reduction_factor, value=0.75\n";
        ts << "setvalue; object=system, quantity=n_threads, value=4\n";
        ts << "setvalue; object=system, quantity=minimum_timestep, value=1e-06\n";
        ts << "setvalue; object=system, quantity=initial_time_step, value=0.01\n";
        ts << "setvalue; object=system, quantity=c_n_weight, value=1\n";
        ts << "setvalue; object=system, quantity=maximum_time_allowed, value=4800\n";
        ts << "create block;type=Pond,name=Infiltration_Pond,_width=200,_height=200,"
              "x=-5971,y=-249,bottom_elevation=0[m],Storage=0[m~^3],alpha=86.061,"
              "beta=2.766,inflow=" << inflowFile << "\n";
        ts << "create block;type=Well_aggregate,name=Well_c,_height=9753.6,"
              "_width=1219.2,bottom_elevation=-4.8768[m],diameter=2.4384[m],"
              "depth=0[m],porosity=1,x=780.8,y=975.36\n";
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
        ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_1,low=5,high=10\n";
        ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_2,low=5,high=10\n";
        ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_3,low=5,high=10\n";
        ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_4,low=5,high=10\n";
        ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_5,low=5,high=10\n";
        ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_6,low=5,high=10\n";
        ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_7,low=5,high=10\n";
        ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_8,low=5,high=10\n";
        ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_9,low=5,high=10\n";
        ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_10,low=5,high=10\n";
        ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_11,low=5,high=10\n";
        ts << "create parameter;type=Parameter,value=6.722232,prior_distribution=normal,name=Ks_12,low=5,high=10\n";
        ts << "create parameter;type=Parameter,value=0.26,prior_distribution=log-normal,name=alpha,low=0.00001,high=10\n";
        ts << "create parameter;type=Parameter,value=0.26,prior_distribution=log-normal,name=new_Van_alpha,low=0.00001,high=10\n";
        ts << "create parameter;type=Parameter,value=2,prior_distribution=log-normal,name=beta,low=0.5,high=5\n";
        ts << "create parameter;type=Parameter,value=2,prior_distribution=log-normal,name=theta_t,low=0.01,high=0.13\n";
        ts << "create parameter;type=Parameter,value=100,prior_distribution=log-normal,name=Transmissivity_Coeff_Drywell,low=50,high=500\n";
        ts << "create parameter;type=Parameter,value=100,prior_distribution=log-normal,name=Transmissivity_Coeff_Sed_Chamber,low=20,high=500\n";
        AppendVnSuiteProDeterministicSoils(ts);
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

    const bool vnModelType = options.modelType.compare(QStringLiteral("VN_Drywell"), Qt::CaseInsensitive) == 0;
    if (vnModelType && !options.vnBaseOhqFile.trimmed().isEmpty()) {
        QString vnBaseText;
        if (!LoadEntireFile(options.vnBaseOhqFile, &vnBaseText, errorMessage)) {
            return false;
        }
        if (!AppendSnippetFile(options.vnSoilLayersFile, QStringLiteral("VN soil layers"), &vnBaseText, errorMessage)) {
            return false;
        }
        if (!AppendSnippetFile(options.vnMoistureLayersFile, QStringLiteral("VN moisture layers"), &vnBaseText, errorMessage)) {
            return false;
        }
        const QString extra = options.additionalCommands.trimmed();
        if (!extra.isEmpty()) {
            vnBaseText += "\n\n# additional_commands\n" + extra + "\n";
        }
        *scriptText = vnBaseText;
        return true;
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
        enrichmentPreset = QStringLiteral("VN_Drywell_Pro");
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

    const QString inflow = options.inflowFile.trimmed();
    if (vnModelType && enrichmentPreset == QStringLiteral("VN_Drywell_Pro")) {
        QString out;
        QTextStream ts(&out);
        AppendVnDrywellProReferenceScript(ts, options, inflow);
        if (!options.observationFile.trimmed().isEmpty()) {
            ts << "create observation;type=Observation,object=" << options.observationObject
               << ",name=" << options.observationName
               << ",expression=" << options.observationExpression
               << ",observed_data=" << options.observationFile
               << ",error_structure=normal,error_standard_deviation=1\n";
        }
        const QString extra = options.additionalCommands.trimmed();
        if (!extra.isEmpty()) {
            ts << "\n# user_additional_commands\n" << extra;
            if (!extra.endsWith('\n')) {
                ts << "\n";
            }
        }
        if (!AppendSnippetFile(options.vnSoilLayersFile, QStringLiteral("VN soil layers"), &out, errorMessage)) {
            return false;
        }
        if (!AppendSnippetFile(options.vnMoistureLayersFile, QStringLiteral("VN moisture layers"), &out, errorMessage)) {
            return false;
        }
        *scriptText = out;
        return true;
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
