#include "jm_bioretention_builder.h"

#include <QTextStream>
#include <QVector>
#include <QtGlobal>

namespace {

QString n(double value)
{
    return QString::number(value, 'g', 12);
}

QString BuildModel(const StarterScriptOptions &options, bool useOptions)
{
    // John McCormack Road bioretention model based on BP-01 of the plan set.
    //
    // The longitudinal profile contains four inlet/profile stations:
    // CC-101-1, CC-102-1, CC-103-1, and CC-104-1.  The model therefore uses
    // four adjacent bioretention columns rather than one central column.
    //
    // Detail section (SI conversion):
    //   mulch                         3 in  = 0.0762 m
    //   bioretention soil media      36 in  = 0.9144 m
    //   choker + gravel + sump       39 in  = 0.9906 m
    //   perforated underdrain         4 in  = 0.1016 m
    //
    // The 3-in choker is not represented as a separate block.  It is included
    // in the aggregate storage depth together with the 24-in gravel layer and
    // the 12-in infiltration sump.  Native-soil blocks are placed below all
    // four columns and on both sides.  All soil/aggregate links connect only
    // immediately adjacent blocks.

    constexpr int columnCount = 4;
    constexpr int nativeBelowNz = 3;

    const double totalLength = useOptions && options.jmLength > 0.0
        ? options.jmLength : 12.192;       // 40 ft
    const double width = useOptions && options.jmWidth > 0.0
        ? options.jmWidth : 1.524;         // 5 ft
    const double columnLength = totalLength / columnCount;
    const double columnArea = columnLength * width;

    const double mulch = useOptions && options.jmMulchDepth > 0.0
        ? options.jmMulchDepth : 0.0762;
    const double media = useOptions && options.jmMediaDepth > 0.0
        ? options.jmMediaDepth : 0.9144;
    const double choker = useOptions && options.jmChokerDepth > 0.0
        ? options.jmChokerDepth : 0.0762;
    const double gravel = useOptions && options.jmGravelDepth > 0.0
        ? options.jmGravelDepth : 0.6096;
    const double sump = useOptions && options.jmSumpDepth > 0.0
        ? options.jmSumpDepth : 0.3048;
    const double aggregateDepth = choker + gravel + sump;
    const double underdrainDiameter =
        useOptions && options.jmUnderdrainDiameter > 0.0
        ? options.jmUnderdrainDiameter : 0.1016;

    // Ponding depths read from the four profile stations on BP-01.
    const QVector<double> pondingDepths = {
        8.27 * 0.0254,
        6.45 * 0.0254,
        4.86 * 0.0254,
        6.47 * 0.0254
    };

    // Three native-soil layers below the aggregate, following the simple
    // Bioretention.ohq reference structure.
    const QVector<double> nativeDepths = {0.8, 0.8, 0.8};
    const double nativeTotalDepth = 2.4;

    const double mediaTop = -mulch;
    const double mediaBottom = mediaTop - media;
    const double aggregateBottom = mediaBottom - aggregateDepth;
    const double groundwaterHead = aggregateBottom - nativeTotalDepth;

    QString out;
    QTextStream ts(&out);
    ts.setRealNumberPrecision(12);

    ts << "# JM_Bioretention: four-column model based on BP-01\n";
    ts << "# Columns: CC-101-1, CC-102-1, CC-103-1, CC-104-1\n";
    ts << "# Choker is included in aggregate storage; units are SI.\n";

    ts << "loadtemplate; filename=<template_dir>/main_components.json\n";
    ts << "addtemplate; filename=<template_dir>/Pond_Plugin.json\n";
    ts << "addtemplate; filename=<template_dir>/unsaturated_soil.json\n";
    ts << "addtemplate; filename=<template_dir>/Well.json\n";
    ts << "addtemplate; filename=<template_dir>/Sewer_system.json\n";
    ts << "addtemplate; filename=<template_dir>/soil_evapotranspiration_models.json\n";
    ts << "addtemplate; filename=<template_dir>/evapotranspiration_models.json\n";
    ts << "addtemplate; filename=<template_dir>/pipe_pump_tank.json\n";

    ts << "create source;type=Precipitation,name=Rain,timeseries=Rain_JM.txt\n";
    ts << "create parameter;type=Parameter,high=20,low=1,name=JM_EngineeredSoilKsat,prior_distribution=log-normal,value=5\n";
    ts << "create parameter;type=Parameter,high=0.1,low=0.001,name=JM_NativeSoilKsat,prior_distribution=log-normal,value=0.01\n";
    ts << "create parameter;type=Parameter,high=5,low=0.5,name=JM_EngineeredSoilAlpha,prior_distribution=log-normal,value=1\n";
    ts << "create parameter;type=Parameter,high=1.8,low=1.2,name=JM_EngineeredSoilN,prior_distribution=normal,value=1.41\n";

    const auto writeSoil = [&](const QString &name,
                               double area,
                               double bottom,
                               double depth,
                               double uiX,
                               double uiY,
                               bool engineered) {
        ts << "create block;type=Soil,Evapotranspiration=,K_sat_original="
           << (engineered ? "5" : "0.01")
           << ",K_sat_scale_factor=1,MC_to_EC_Threshold_Moisture=0,"
              "MC_to_EC_coefficient=0,MC_to_EC_exponent=0,_height=150,_width=170,"
              "act_X=" << n(uiX / 200.0) << ",act_Y=" << n(-uiY / 200.0)
           << ",alpha=1"
           << ",aniso_ratio=1,area=" << n(area)
           << ",bottom_elevation=" << n(bottom)
           << ",depth=" << n(depth)
           << ",n=1.41"
           << ",name=" << name
           << ",specific_storage=0.01,theta=0.25,theta_res=0.05,theta_sat=0.4,"
              "x=" << n(uiX) << ",y=" << n(uiY) << "\n";
    };

    // Five drainage areas shown on DA-01 (impervious portions, ft2 -> m2).
    const QVector<double> drainageAreas = {
        3403.0 * 0.09290304,
        613.0 * 0.09290304,
        478.0 * 0.09290304,
        487.0 * 0.09290304,
        404.0 * 0.09290304
    };
    const QVector<int> drainageTargets = {1, 2, 2, 3, 4};

    for (int i = 0; i < drainageAreas.size(); ++i) {
        ts << "create block;type=Catchment,Evapotranspiration=,ManningCoeff=0.011,"
              "Precipitation=Rain,Runoff_coeff=0.95,Slope=0.01,Width=10,"
              "_height=120,_width=180,area=" << n(drainageAreas[i])
           << "[m~^2],depression_storage=0,depth=0,elevation=0,inflow=,"
              "loss_coefficient=0,name=JM DA-0" << (i + 1)
           << ",x=" << n(-650.0 + i * 190.0) << ",y=-250\n";
    }

    // Four surface, media, and aggregate blocks.
    for (int c = 1; c <= columnCount; ++c) {
        const double x = (c - 1) * 230.0;

        ts << "create block;type=Pond,Evapotranspiration=0,Precipitation=Rain,"
              "Storage=0,_height=130,_width=170,alpha=" << n(columnArea)
           << ",alpha_multiplier=1,beta=1,bottom_elevation=0,inflow=,"
              "name=JM Pond " << c << ",x=" << n(x) << ",y=0\n";

        writeSoil(QStringLiteral("JM Engineered Soil %1").arg(c),
                  columnArea, mediaBottom, media, x, 210.0, true);

        ts << "create block;type=Aggregate_storage_layer,K_sat=5000,"
              "_height=150,_width=170,area=" << n(columnArea)
           << ",bottom_elevation=" << n(aggregateBottom)
           << ",depth=" << n(aggregateDepth)
           << ",inflow=,name=JM Aggregate " << c
           << ",porosity=0.4,x=" << n(x) << ",y=420\n";
    }

    // Native soil below each of the four cells.
    double nativeTop = aggregateBottom;
    for (int k = 1; k <= nativeBelowNz; ++k) {
        const double bottom = nativeTop - nativeDepths[k - 1];
        for (int c = 1; c <= columnCount; ++c) {
            writeSoil(QStringLiteral("JM Native Soil %1-%2").arg(c).arg(k),
                      columnArea, bottom, nativeDepths[k - 1],
                      (c - 1) * 230.0, 420.0 + k * 190.0, false);
        }
        nativeTop = bottom;
    }

    // Native soil beside the outer walls at each principal depth.
    writeSoil(QStringLiteral("JM Left Native Media"), columnArea,
              mediaBottom, media, -230.0, 210.0, false);
    writeSoil(QStringLiteral("JM Right Native Media"), columnArea,
              mediaBottom, media, columnCount * 230.0, 210.0, false);
    writeSoil(QStringLiteral("JM Left Native Aggregate"), columnArea,
              aggregateBottom, aggregateDepth, -230.0, 420.0, false);
    writeSoil(QStringLiteral("JM Right Native Aggregate"), columnArea,
              aggregateBottom, aggregateDepth, columnCount * 230.0, 420.0, false);

    nativeTop = aggregateBottom;
    for (int k = 1; k <= nativeBelowNz; ++k) {
        const double bottom = nativeTop - nativeDepths[k - 1];
        writeSoil(QStringLiteral("JM Left Native %1").arg(k), columnArea,
                  bottom, nativeDepths[k - 1], -230.0,
                  420.0 + k * 190.0, false);
        writeSoil(QStringLiteral("JM Right Native %1").arg(k), columnArea,
                  bottom, nativeDepths[k - 1], columnCount * 230.0,
                  420.0 + k * 190.0, false);
        nativeTop = bottom;
    }

    ts << "create block;type=Catch basin,_height=150,_width=190,area=1,"
          "bottom_elevation=" << n(aggregateBottom)
       << ",inflow=,name=JM Catch Basin,x=1150,y=200\n";
    ts << "create block;type=fixed_head,Storage=100000,_height=150,_width=220,"
          "head=0,name=JM Receiving Water,x=1400,y=200\n";
    ts << "create block;type=fixed_head,Storage=100000,_height=150,_width=900,"
          "head=" << n(groundwaterHead)
       << ",name=JM Groundwater,x=345,y=1200\n";

    // Parameter assignments.
    for (int c = 1; c <= columnCount; ++c) {
        ts << "setasparameter; object=JM Engineered Soil " << c
           << ", parametername=JM_EngineeredSoilKsat, quantity=K_sat_original\n";
        ts << "setasparameter; object=JM Engineered Soil " << c
           << ", parametername=JM_EngineeredSoilAlpha, quantity=alpha\n";
        ts << "setasparameter; object=JM Engineered Soil " << c
           << ", parametername=JM_EngineeredSoilN, quantity=n\n";
        for (int k = 1; k <= nativeBelowNz; ++k) {
            ts << "setasparameter; object=JM Native Soil " << c << "-" << k
               << ", parametername=JM_NativeSoilKsat, quantity=K_sat_original\n";
        }
    }
    const QVector<QString> sideNativeNames = {
        QStringLiteral("JM Left Native Media"),
        QStringLiteral("JM Right Native Media"),
        QStringLiteral("JM Left Native Aggregate"),
        QStringLiteral("JM Right Native Aggregate"),
        QStringLiteral("JM Left Native 1"), QStringLiteral("JM Left Native 2"),
        QStringLiteral("JM Left Native 3"), QStringLiteral("JM Right Native 1"),
        QStringLiteral("JM Right Native 2"), QStringLiteral("JM Right Native 3")
    };
    for (const QString &soilName : sideNativeNames) {
        ts << "setasparameter; object=" << soilName
           << ", parametername=JM_NativeSoilKsat, quantity=K_sat_original\n";
    }

    // Drainage-area routing.
    for (int i = 0; i < drainageAreas.size(); ++i) {
        ts << "create link;from=JM DA-0" << (i + 1)
           << ",to=JM Pond " << drainageTargets[i]
           << ",type=Catchment_link,name=JM DA-0" << (i + 1)
           << " to Pond " << drainageTargets[i] << "\n";
    }

    // Vertical flow paths and surface overflow/underdrains.
    for (int c = 1; c <= columnCount; ++c) {
        ts << "create link;from=JM Pond " << c
           << ",to=JM Engineered Soil " << c
           << ",type=surfacewater_to_soil_link,name=JM Pond " << c
           << " to Engineered Soil " << c << "\n";

        ts << "create link;from=JM Engineered Soil " << c
           << ",to=JM Aggregate " << c
           << ",type=soil_to_aggregate_link,name=JM Engineered Soil " << c
           << " to Aggregate " << c << "\n";

        ts << "create link;from=JM Aggregate " << c
           << ",to=JM Native Soil " << c << "-1"
           << ",type=aggregate_to_soil_link,name=JM Aggregate " << c
           << " to Native Soil " << c << "-1\n";

        for (int k = 1; k < nativeBelowNz; ++k) {
            ts << "create link;from=JM Native Soil " << c << "-" << k
               << ",to=JM Native Soil " << c << "-" << (k + 1)
               << ",type=soil_to_soil_link,name=JM Native Vertical "
               << c << "-" << k << "\n";
        }

        ts << "create link;from=JM Pond " << c
           << ",to=JM Catch Basin,type=wier,alpha=10000,beta=2.5,"
              "crest_elevation=" << n(pondingDepths[c - 1])
           << ",name=JM Overflow Weir " << c << "\n";

        ts << "create link;from=JM Aggregate " << c
           << ",to=JM Catch Basin,type=Sewer_pipe,ManningCoeff=0.011,"
              "diameter=" << n(underdrainDiameter)
           << "[m],end_elevation=" << n(mediaBottom - choker - gravel)
           << ",length=" << n(columnLength)
           << "[m],name=JM Underdrain " << c
           << ",start_elevation=" << n(mediaBottom - choker - gravel + 0.05)
           << "[m]\n";
    }

    // Adjacent horizontal exchanges among the four columns.
    for (int c = 1; c < columnCount; ++c) {
        ts << "create link;from=JM Engineered Soil " << c
           << ",to=JM Engineered Soil " << (c + 1)
           << ",type=soil_to_soil_H_link,length=" << n(columnLength)
           << "[m],area=" << n(media * width)
           << "[m~^2],name=JM Engineered Horizontal " << c << "\n";

        ts << "create link;from=JM Aggregate " << c
           << ",to=JM Aggregate " << (c + 1)
           << ",type=aggregate2aggregate_H_Link,length=" << n(columnLength)
           << "[m],width=" << n(width)
           << "[m],name=JM Aggregate Horizontal " << c << "\n";

        for (int k = 1; k <= nativeBelowNz; ++k) {
            ts << "create link;from=JM Native Soil " << c << "-" << k
               << ",to=JM Native Soil " << (c + 1) << "-" << k
               << ",type=soil_to_soil_H_link,length=" << n(columnLength)
               << "[m],area=" << n(nativeDepths[k - 1] * width)
               << "[m~^2],name=JM Native Horizontal " << c << "-" << k
               << "\n";
        }
    }

    // Native soils around the left and right edges.
    ts << "create link;from=JM Left Native Media,to=JM Engineered Soil 1,"
          "type=soil_to_soil_H_link,length=" << n(width)
       << "[m],area=" << n(media * columnLength)
       << "[m~^2],name=JM Left Media Boundary\n";
    ts << "create link;from=JM Engineered Soil 4,to=JM Right Native Media,"
          "type=soil_to_soil_H_link,length=" << n(width)
       << "[m],area=" << n(media * columnLength)
       << "[m~^2],name=JM Right Media Boundary\n";

    ts << "create link;from=JM Left Native Aggregate,to=JM Aggregate 1,"
          "type=aggregate_to_soil_link,name=JM Left Aggregate Boundary\n";
    ts << "create link;from=JM Aggregate 4,to=JM Right Native Aggregate,"
          "type=aggregate_to_soil_link,name=JM Right Aggregate Boundary\n";

    ts << "create link;from=JM Left Native Media,to=JM Left Native Aggregate,"
          "type=soil_to_soil_link,name=JM Left Native Media to Aggregate\n";
    ts << "create link;from=JM Right Native Media,to=JM Right Native Aggregate,"
          "type=soil_to_soil_link,name=JM Right Native Media to Aggregate\n";
    ts << "create link;from=JM Left Native Aggregate,to=JM Left Native 1,"
          "type=soil_to_soil_link,name=JM Left Native Aggregate to Native 1\n";
    ts << "create link;from=JM Right Native Aggregate,to=JM Right Native 1,"
          "type=soil_to_soil_link,name=JM Right Native Aggregate to Native 1\n";

    for (int k = 1; k <= nativeBelowNz; ++k) {
        ts << "create link;from=JM Left Native " << k
           << ",to=JM Native Soil 1-" << k
           << ",type=soil_to_soil_H_link,length=" << n(width)
           << "[m],area=" << n(nativeDepths[k - 1] * columnLength)
           << "[m~^2],name=JM Left Native Boundary " << k << "\n";
        ts << "create link;from=JM Native Soil 4-" << k
           << ",to=JM Right Native " << k
           << ",type=soil_to_soil_H_link,length=" << n(width)
           << "[m],area=" << n(nativeDepths[k - 1] * columnLength)
           << "[m~^2],name=JM Right Native Boundary " << k << "\n";

        if (k < nativeBelowNz) {
            ts << "create link;from=JM Left Native " << k
               << ",to=JM Left Native " << (k + 1)
               << ",type=soil_to_soil_link,name=JM Left Native Vertical "
               << k << "\n";
            ts << "create link;from=JM Right Native " << k
               << ",to=JM Right Native " << (k + 1)
               << ",type=soil_to_soil_link,name=JM Right Native Vertical "
               << k << "\n";
        }
    }

    // Groundwater and outlet routing.
    for (int c = 1; c <= columnCount; ++c) {
        ts << "create link;from=JM Native Soil " << c << "-" << nativeBelowNz
           << ",to=JM Groundwater,type=soil_to_fixedhead_link,"
              "name=JM Native Soil " << c << " to Groundwater\n";
    }
    ts << "create link;from=JM Left Native " << nativeBelowNz
       << ",to=JM Groundwater,type=soil_to_fixedhead_link,"
          "name=JM Left Native to Groundwater\n";
    ts << "create link;from=JM Right Native " << nativeBelowNz
       << ",to=JM Groundwater,type=soil_to_fixedhead_link,"
          "name=JM Right Native to Groundwater\n";

    ts << "create link;from=JM Catch Basin,to=JM Receiving Water,"
          "type=Sewer_pipe,ManningCoeff=0.011,diameter=0.2[m],"
          "end_elevation=0,length=2[m],name=JM Catch Basin Outlet,"
          "start_elevation=0.05[m]\n";

    return out;
}

} // namespace

namespace JMBioretentionBuilder
{

QString FullReferenceScript()
{
    StarterScriptOptions defaults;
    return BuildModel(defaults, false);
}

QString InflowTargetObject()
{
    return QStringLiteral("JM DA-01");
}

QString RainfallTargetObject()
{
    return QStringLiteral("Rain");
}

QString ContributingCatchmentObject()
{
    return QStringLiteral("JM DA-01");
}

bool Build(const StarterScriptOptions &options,
           QString *scriptText,
           QString *errorMessage)
{
    if (scriptText == nullptr) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Internal error: output script buffer is null.");
        }
        return false;
    }

    const QString mode = options.jmBuildMode.trimmed();
    if (mode.compare(QStringLiteral("FullReference"),
                     Qt::CaseInsensitive) == 0) {
        *scriptText = FullReferenceScript();
        return true;
    }

    if (mode.isEmpty()
        || mode.compare(QStringLiteral("SoftReference"),
                        Qt::CaseInsensitive) == 0) {
        *scriptText = BuildModel(options, true);
        return true;
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral(
            "JM_Bioretention builder does not handle mode: %1").arg(mode);
    }
    return false;
}

} // namespace JMBioretentionBuilder
