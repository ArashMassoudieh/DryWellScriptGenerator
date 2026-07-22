#include "jm_bioretention_builder.h"

#include <QTextStream>
#include <QVector>
#include <QtGlobal>
#include <cmath>

namespace {

QString n(double value)
{
    return QString::number(value, 'g', 12);
}

int nativeColumnForFacilityColumn(int facilityColumn, int nativeNx)
{
    return qBound(1,
                  static_cast<int>(std::floor((facilityColumn - 0.5) * nativeNx / 4.0)) + 1,
                  nativeNx);
}

QString nativeName(int ix, int iz)
{
    return QStringLiteral("JM Native Soil %1-%2").arg(ix).arg(iz);
}

QString BuildModel(const StarterScriptOptions &options,
                   bool useOptions,
                   bool useCurbChannels,
                   bool dtSimple)
{
    constexpr int columnCount = 4;

    // The existing R-domain controls are intentionally shared with JM so the
    // current UI can control the remaining native-soil discretization without
    // requiring a second duplicate set of fields:
    //   rVerticalLayers -> JM native horizontal cells (nx)
    //   rNativeSoilNz  -> JM native vertical layers (nz)
    // Defaults reproduce the original JM 4 x 3 bottom-native grid.
    const int nativeNx = useOptions && options.rVerticalLayers > 0
        ? qMax(1, options.rVerticalLayers) : 4;
    const int nativeNz = useOptions && options.rNativeSoilNz > 0
        ? qMax(1, options.rNativeSoilNz) : 3;

    const double totalLength = useOptions && options.jmLength > 0.0
        ? options.jmLength : 12.192; // 40 ft
    const double requestedWidth = useOptions ? options.jmWidth : 0.0;
    const double width = requestedWidth > 0.0
        && qAbs(requestedWidth - 1.524) > 1.0e-9
        ? requestedWidth : 3.7084; // 12 ft 2 in
    const double columnLength = totalLength / columnCount;
    const double columnArea = columnLength * width;
    const double nativeCellLength = totalLength / nativeNx;
    const double nativeCellArea = nativeCellLength * width;

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

    const QVector<double> pondingDepths = {
        8.27 * 0.0254,
        6.45 * 0.0254,
        4.86 * 0.0254,
        6.47 * 0.0254
    };
    const QVector<double> stationZ = {0.09, 0.06, 0.03, 0.0};

    // Preserve the original 2.4-m modeled native domain while allowing nz.
    const double nativeTotalDepth = 2.4;
    const double nativeLayerDepth = nativeTotalDepth / nativeNz;

    const double baseMediaTop = -mulch;
    const double baseMediaBottom = baseMediaTop - media;
    const double baseAggregateBottom = baseMediaBottom - aggregateDepth;
    const double groundwaterHead = baseAggregateBottom - nativeTotalDepth;

    QString out;
    QTextStream ts(&out);
    ts.setRealNumberPrecision(12);

    ts << "# JM_Bioretention: four-column model based on BP-01\n";
    ts << "# Columns: CC-101-1, CC-102-1, CC-103-1, CC-104-1\n";
    ts << "# Native grid: nx=" << nativeNx << ", nz=" << nativeNz << "\n";
    if (dtSimple) {
        ts << "# Mode: DT Simple; side native soils omitted and underdrain is routed serially.\n";
    }

    ts << "loadtemplate; filename=<template_dir>/main_components.json\n";
    ts << "addtemplate; filename=<template_dir>/Pond_Plugin.json\n";
    ts << "addtemplate; filename=<template_dir>/unsaturated_soil.json\n";
    ts << "addtemplate; filename=<template_dir>/Well.json\n";
    ts << "addtemplate; filename=<template_dir>/Sewer_system.json\n";
    ts << "addtemplate; filename=<template_dir>/soil_evapotranspiration_models.json\n";
    ts << "addtemplate; filename=<template_dir>/evapotranspiration_models.json\n";
    ts << "addtemplate; filename=<template_dir>/pipe_pump_tank.json\n";
    if (useCurbChannels) {
        ts << "addtemplate; filename=<template_dir>/open_channel.json\n";
    }

    ts << "create source;type=Precipitation,name=Rain,timeseries=Rain_JM.txt\n";
    ts << "create parameter;type=Parameter,high=20,low=1,name=JM_EngineeredSoilKsat,prior_distribution=log-normal,value=5\n";
    ts << "create parameter;type=Parameter,high=0.1,low=0.001,name=JM_NativeSoilKsat,prior_distribution=log-normal,value=0.01\n";
    ts << "create parameter;type=Parameter,high=5,low=0.5,name=JM_EngineeredSoilAlpha,prior_distribution=log-normal,value=1\n";
    ts << "create parameter;type=Parameter,high=1.8,low=1.2,name=JM_EngineeredSoilN,prior_distribution=normal,value=1.41\n";

    const auto writeSoil = [&](const QString &name,
                               double area,
                               double bottom,
                               double depth,
                               double actX,
                               double actY,
                               double uiX,
                               double uiY,
                               bool engineered) {
        ts << "create block;type=Soil,Evapotranspiration=,K_sat_original="
           << (engineered ? "5" : "0.01")
           << ",K_sat_scale_factor=1,MC_to_EC_Threshold_Moisture=0,"
              "MC_to_EC_coefficient=0,MC_to_EC_exponent=0,_height=150,_width=170,"
              "act_X=" << n(actX) << ",act_Y=" << n(actY)
           << ",alpha=1,aniso_ratio=1,area=" << n(area)
           << ",bottom_elevation=" << n(bottom)
           << ",depth=" << n(depth)
           << ",n=1.41,name=" << name
           << ",specific_storage=0.01,theta=0.25,theta_res=0.05,theta_sat=0.4,"
              "x=" << n(uiX) << ",y=" << n(uiY) << "\n";
    };

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
           << "[m~^2],depression_storage=0,depth=0,elevation=0,inflow=0,"
              "loss_coefficient=0,name=JM DA-0" << (i + 1)
           << ",x=" << n(-650.0 + i * 190.0) << ",y=-250\n";
    }

    ts << "create block;type=Catchment,Evapotranspiration=,ManningCoeff=0.03,"
          "Precipitation=Rain,Runoff_coeff=1,Slope=0.01,Width=1,_height=180,"
          "_width=220,area=1[m~^2],depression_storage=0,depth=0,elevation=0,"
          "inflow=0,loss_coefficient=0,name=JM Dummy Catchment,x=-1050,y=-250\n";

    if (useCurbChannels) {
        for (int c = 1; c <= columnCount; ++c) {
            const double uiX = (c - 1) * 230.0;
            ts << "create block;type=Trapezoidal Channel Segment,"
                  "ManningCoeff=0.015,base_width=0.6,side_slope=2,"
                  "bottom_elevation=" << n(stationZ[c - 1])
               << ",depth=0,dam_height=0,length=" << n(columnLength)
               << "[m],inflow=0,ag_area=0,non_ag_area=0,"
                  "ag_withdrawal_per_unit_area=0,non_ag_withdrawal_per_unit_area=0,"
                  "_height=100,_width=180,name=JM Curb Channel " << c
               << ",x=" << n(uiX) << ",y=-140\n";
        }
    }

    for (int c = 1; c <= columnCount; ++c) {
        const double uiX = (c - 1) * 230.0;
        const double actX = (c - 0.5) * columnLength;
        const double z = stationZ[c - 1];
        const double mediaTop = z + baseMediaTop;
        const double mediaBottom = z + baseMediaBottom;
        const double aggregateBottom = z + baseAggregateBottom;

        ts << "create block;type=Pond,Evapotranspiration=0,Precipitation=Rain,"
              "Storage=0,_height=130,_width=170,act_X=" << n(actX)
           << ",act_Y=" << n(z) << ",alpha=" << n(columnArea)
           << ",alpha_multiplier=1,beta=1,bottom_elevation=" << n(z)
           << ",inflow=0,name=JM Pond " << c
           << ",x=" << n(uiX) << ",y=0\n";

        writeSoil(QStringLiteral("JM Engineered Soil %1").arg(c),
                  columnArea, mediaBottom, media,
                  actX, 0.5 * (mediaTop + mediaBottom),
                  uiX, 210.0, true);

        ts << "create block;type=Aggregate_storage_layer,K_sat=5000,"
              "_height=150,_width=170,act_X=" << n(actX)
           << ",act_Y=" << n(mediaBottom - 0.5 * aggregateDepth)
           << ",area=" << n(columnArea)
           << ",bottom_elevation=" << n(aggregateBottom)
           << ",depth=" << n(aggregateDepth)
           << ",inflow=0,name=JM Aggregate " << c
           << ",porosity=0.4,x=" << n(uiX) << ",y=420\n";
    }

    // Bottom native domain. Its horizontal and vertical counts are independent
    // from the four fixed pond/media/aggregate stations.
    for (int iz = 1; iz <= nativeNz; ++iz) {
        const double topOffset = (iz - 1) * nativeLayerDepth;
        const double bottomOffset = iz * nativeLayerDepth;
        for (int ix = 1; ix <= nativeNx; ++ix) {
            const double actX = (ix - 0.5) * nativeCellLength;
            // Interpolate the profile offset along the facility length.
            const double f = qBound(0.0, actX / totalLength, 1.0);
            const double profileZ = stationZ.first()
                + f * (stationZ.last() - stationZ.first());
            const double top = profileZ + baseAggregateBottom - topOffset;
            const double bottom = profileZ + baseAggregateBottom - bottomOffset;
            const double uiX = nativeNx == 1 ? 345.0
                : (ix - 1) * (690.0 / (nativeNx - 1));
            writeSoil(nativeName(ix, iz), nativeCellArea, bottom,
                      nativeLayerDepth, actX, 0.5 * (top + bottom),
                      uiX, 420.0 + iz * 190.0, false);
        }
    }

    // The older modes retain the surrounding left/right native blocks.
    if (!dtSimple) {
        const double leftZ = stationZ.first();
        const double rightZ = stationZ.last();
        const double sideArea = columnArea;
        const double leftActX = -0.5 * columnLength;
        const double rightActX = totalLength + 0.5 * columnLength;

        writeSoil(QStringLiteral("JM Left Native Media"), sideArea,
                  leftZ + baseMediaBottom, media,
                  leftActX, leftZ + baseMediaTop - 0.5 * media,
                  -230.0, 210.0, false);
        writeSoil(QStringLiteral("JM Right Native Media"), sideArea,
                  rightZ + baseMediaBottom, media,
                  rightActX, rightZ + baseMediaTop - 0.5 * media,
                  920.0, 210.0, false);
        writeSoil(QStringLiteral("JM Left Native Aggregate"), sideArea,
                  leftZ + baseAggregateBottom, aggregateDepth,
                  leftActX, leftZ + baseMediaBottom - 0.5 * aggregateDepth,
                  -230.0, 420.0, false);
        writeSoil(QStringLiteral("JM Right Native Aggregate"), sideArea,
                  rightZ + baseAggregateBottom, aggregateDepth,
                  rightActX, rightZ + baseMediaBottom - 0.5 * aggregateDepth,
                  920.0, 420.0, false);

        for (int iz = 1; iz <= nativeNz; ++iz) {
            const double topOffset = (iz - 1) * nativeLayerDepth;
            const double bottomOffset = iz * nativeLayerDepth;
            writeSoil(QStringLiteral("JM Left Native %1").arg(iz), sideArea,
                      leftZ + baseAggregateBottom - bottomOffset,
                      nativeLayerDepth, leftActX,
                      leftZ + baseAggregateBottom - 0.5 * (topOffset + bottomOffset),
                      -230.0, 420.0 + iz * 190.0, false);
            writeSoil(QStringLiteral("JM Right Native %1").arg(iz), sideArea,
                      rightZ + baseAggregateBottom - bottomOffset,
                      nativeLayerDepth, rightActX,
                      rightZ + baseAggregateBottom - 0.5 * (topOffset + bottomOffset),
                      920.0, 420.0 + iz * 190.0, false);
        }
    }

    ts << "create block;type=Catch basin,_height=150,_width=190,area=1,"
          "bottom_elevation=" << n(baseAggregateBottom)
       << ",inflow=0,name=JM Catch Basin,x=1150,y=200\n";
    ts << "create block;type=fixed_head,Storage=100000,_height=150,_width=220,"
          "head=0,name=JM Receiving Water,x=1400,y=200\n";
    ts << "create block;type=fixed_head,Storage=100000,_height=150,_width=900,"
          "head=" << n(groundwaterHead)
       << ",name=JM Groundwater,x=345,y=1200\n";

    for (int c = 1; c <= columnCount; ++c) {
        ts << "setasparameter; object=JM Engineered Soil " << c
           << ", parametername=JM_EngineeredSoilKsat, quantity=K_sat_original\n";
        ts << "setasparameter; object=JM Engineered Soil " << c
           << ", parametername=JM_EngineeredSoilAlpha, quantity=alpha\n";
        ts << "setasparameter; object=JM Engineered Soil " << c
           << ", parametername=JM_EngineeredSoilN, quantity=n\n";
    }
    for (int ix = 1; ix <= nativeNx; ++ix) {
        for (int iz = 1; iz <= nativeNz; ++iz) {
            ts << "setasparameter; object=" << nativeName(ix, iz)
               << ", parametername=JM_NativeSoilKsat, quantity=K_sat_original\n";
        }
    }
    if (!dtSimple) {
        const QStringList sideNames = {
            QStringLiteral("JM Left Native Media"),
            QStringLiteral("JM Right Native Media"),
            QStringLiteral("JM Left Native Aggregate"),
            QStringLiteral("JM Right Native Aggregate")
        };
        for (const QString &name : sideNames) {
            ts << "setasparameter; object=" << name
               << ", parametername=JM_NativeSoilKsat, quantity=K_sat_original\n";
        }
        for (int iz = 1; iz <= nativeNz; ++iz) {
            ts << "setasparameter; object=JM Left Native " << iz
               << ", parametername=JM_NativeSoilKsat, quantity=K_sat_original\n";
            ts << "setasparameter; object=JM Right Native " << iz
               << ", parametername=JM_NativeSoilKsat, quantity=K_sat_original\n";
        }
    }

    if (!useCurbChannels) {
        for (int i = 0; i < drainageAreas.size(); ++i) {
            ts << "create link;from=JM DA-0" << (i + 1)
               << ",to=JM Pond " << drainageTargets[i]
               << ",type=Catchment_link,name=JM DA-0" << (i + 1)
               << " to Pond " << drainageTargets[i] << "\n";
        }
    } else {
        for (int i = 0; i < drainageAreas.size(); ++i) {
            const int channel = drainageTargets[i];
            ts << "create link;from=JM DA-0" << (i + 1)
               << ",to=JM Curb Channel " << channel
               << ",type=Catchment_link,name=JM DA-0" << (i + 1)
               << " to Curb Channel " << channel << "\n";
        }
        for (int c = 1; c < columnCount; ++c) {
            ts << "create link;from=JM Curb Channel " << c
               << ",to=JM Curb Channel " << (c + 1)
               << ",type=Trapezoidal_Channel_link,name=JM Curb Channel "
               << c << " to " << (c + 1) << "\n";
        }
        for (int c = 1; c <= columnCount; ++c) {
            ts << "create link;from=JM Curb Channel " << c
               << ",to=JM Pond " << c
               << ",type=wier,alpha=10000,beta=1.5,crest_elevation="
               << n(stationZ[c - 1])
               << ",name=JM Curb Channel " << c << " to Pond " << c << "\n";
        }
        ts << "create link;from=JM Curb Channel 4,to=JM Catch Basin,"
              "type=wier,alpha=10000,beta=1.5,crest_elevation=0.15,"
              "name=JM Curb Channel Bypass\n";
    }

    for (int c = 1; c <= columnCount; ++c) {
        const int nativeIx = nativeColumnForFacilityColumn(c, nativeNx);
        ts << "create link;from=JM Pond " << c
           << ",to=JM Engineered Soil " << c
           << ",type=surfacewater_to_soil_link,name=JM Pond " << c
           << " to Engineered Soil " << c << "\n";
        ts << "create link;from=JM Engineered Soil " << c
           << ",to=JM Aggregate " << c
           << ",type=soil_to_aggregate_link,name=JM Engineered Soil " << c
           << " to Aggregate " << c << "\n";
        ts << "create link;from=JM Aggregate " << c
           << ",to=" << nativeName(nativeIx, 1)
           << ",type=aggregate_to_soil_link,name=JM Aggregate " << c
           << " to " << nativeName(nativeIx, 1) << "\n";

        const double z = stationZ[c - 1];
        ts << "create link;from=JM Pond " << c
           << ",to=JM Catch Basin,type=wier,alpha=10000,beta=2.5,"
              "crest_elevation=" << n(z + pondingDepths[c - 1])
           << ",name=JM Overflow Weir " << c << "\n";

        // In the legacy modes every aggregate has its own pipe to the catch
        // basin. DT Simple instead uses a serial 4-in underdrain and only the
        // final station discharges to the catch basin, matching JM1.ohq.
        if (!dtSimple) {
            const double invert = z + baseMediaBottom - choker - gravel;
            ts << "create link;from=JM Aggregate " << c
               << ",to=JM Catch Basin,type=Sewer_pipe,ManningCoeff=0.011,"
                  "diameter=" << n(underdrainDiameter)
               << "[m],end_elevation=" << n(invert - 0.05)
               << "[m],length=" << n(columnLength)
               << "[m],name=JM Underdrain " << c
               << ",start_elevation=" << n(invert) << "[m]\n";
        }
    }

    // Engineered media exchanges remain between the four adjacent stations.
    for (int c = 1; c < columnCount; ++c) {
        ts << "create link;from=JM Engineered Soil " << c
           << ",to=JM Engineered Soil " << (c + 1)
           << ",type=soil_to_soil_H_link,length=" << n(columnLength)
           << "[m],area=" << n(media * width)
           << "[m~^2],name=JM Engineered Horizontal " << c << "\n";

        if (!dtSimple) {
            ts << "create link;from=JM Aggregate " << c
               << ",to=JM Aggregate " << (c + 1)
               << ",type=aggregate2aggregate_H_Link,length=" << n(columnLength)
               << "[m],width=" << n(width)
               << "[m],name=JM Aggregate Horizontal " << c << "\n";
        }
    }

    // Native grid links: only immediately adjacent blocks.
    for (int iz = 1; iz <= nativeNz; ++iz) {
        for (int ix = 1; ix < nativeNx; ++ix) {
            ts << "create link;from=" << nativeName(ix, iz)
               << ",to=" << nativeName(ix + 1, iz)
               << ",type=soil_to_soil_H_link,length=" << n(nativeCellLength)
               << "[m],area=" << n(nativeLayerDepth * width)
               << "[m~^2],name=JM Native Horizontal " << ix << "-" << iz << "\n";
        }
    }
    for (int ix = 1; ix <= nativeNx; ++ix) {
        for (int iz = 1; iz < nativeNz; ++iz) {
            ts << "create link;from=" << nativeName(ix, iz)
               << ",to=" << nativeName(ix, iz + 1)
               << ",type=soil_to_soil_link,name=JM Native Vertical "
               << ix << "-" << iz << "\n";
        }
    }

    if (dtSimple) {
        // Serial underdrain links added manually in JM1 are completed here
        // using the BP-01 4-in diameter and the actual 3.048-m station spacing.
        for (int c = 1; c < columnCount; ++c) {
            const double startInvert = stationZ[c - 1] + baseMediaBottom - choker - gravel;
            const double endInvert = stationZ[c] + baseMediaBottom - choker - gravel;
            ts << "create link;from=JM Aggregate " << c
               << ",to=JM Aggregate " << (c + 1)
               << ",type=Sewer_pipe,ManningCoeff=0.011,diameter="
               << n(underdrainDiameter) << "[m],end_elevation="
               << n(endInvert) << "[m],length=" << n(columnLength)
               << "[m],name=JM Aggregate " << c << " - JM Aggregate " << (c + 1)
               << ",start_elevation=" << n(startInvert) << "[m]\n";
        }
        const double finalInvert = stationZ.last() + baseMediaBottom - choker - gravel;
        ts << "create link;from=JM Aggregate 4,to=JM Catch Basin,"
              "type=Sewer_pipe,ManningCoeff=0.011,diameter="
           << n(underdrainDiameter) << "[m],end_elevation="
           << n(finalInvert - 0.05) << "[m],length=" << n(columnLength)
           << "[m],name=JM Underdrain 4,start_elevation="
           << n(finalInvert) << "[m]\n";
    } else {
        // Retain the original surrounding-native connections in legacy modes.
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

        for (int iz = 1; iz <= nativeNz; ++iz) {
            ts << "create link;from=JM Left Native " << iz
               << ",to=" << nativeName(1, iz)
               << ",type=soil_to_soil_H_link,length=" << n(width)
               << "[m],area=" << n(nativeLayerDepth * nativeCellLength)
               << "[m~^2],name=JM Left Native Boundary " << iz << "\n";
            ts << "create link;from=" << nativeName(nativeNx, iz)
               << ",to=JM Right Native " << iz
               << ",type=soil_to_soil_H_link,length=" << n(width)
               << "[m],area=" << n(nativeLayerDepth * nativeCellLength)
               << "[m~^2],name=JM Right Native Boundary " << iz << "\n";
            if (iz < nativeNz) {
                ts << "create link;from=JM Left Native " << iz
                   << ",to=JM Left Native " << (iz + 1)
                   << ",type=soil_to_soil_link,name=JM Left Native Vertical " << iz << "\n";
                ts << "create link;from=JM Right Native " << iz
                   << ",to=JM Right Native " << (iz + 1)
                   << ",type=soil_to_soil_link,name=JM Right Native Vertical " << iz << "\n";
            }
        }
    }

    for (int ix = 1; ix <= nativeNx; ++ix) {
        ts << "create link;from=" << nativeName(ix, nativeNz)
           << ",to=JM Groundwater,type=soil_to_fixedhead_link,"
              "name=JM Native Soil " << ix << " to Groundwater\n";
    }
    if (!dtSimple) {
        ts << "create link;from=JM Left Native " << nativeNz
           << ",to=JM Groundwater,type=soil_to_fixedhead_link,"
              "name=JM Left Native to Groundwater\n";
        ts << "create link;from=JM Right Native " << nativeNz
           << ",to=JM Groundwater,type=soil_to_fixedhead_link,"
              "name=JM Right Native to Groundwater\n";
    }

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
    return BuildModel(defaults, false, false, false);
}

QString ChannelReferenceScript()
{
    StarterScriptOptions defaults;
    return BuildModel(defaults, false, true, false);
}

QString CurbChannelReferenceScript()
{
    return ChannelReferenceScript();
}

QString DtSimpleReferenceScript()
{
    StarterScriptOptions defaults;
    return BuildModel(defaults, false, false, true);
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
            *errorMessage = QStringLiteral("Internal error: output script buffer is null.");
        }
        return false;
    }

    const QString mode = options.jmBuildMode.trimmed();
    if (mode.compare(QStringLiteral("FullReference"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("Direct"), Qt::CaseInsensitive) == 0) {
        *scriptText = FullReferenceScript();
        return true;
    }

    if (mode.compare(QStringLiteral("Channel"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("CurbChannel"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("ChannelReference"), Qt::CaseInsensitive) == 0) {
        *scriptText = BuildModel(options, true, true, false);
        return true;
    }

    if (mode.compare(QStringLiteral("DTSimple"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("DT_Simple"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("DigitalTwinSimple"), Qt::CaseInsensitive) == 0) {
        *scriptText = BuildModel(options, true, false, true);
        return true;
    }

    if (mode.isEmpty()
        || mode.compare(QStringLiteral("SoftReference"), Qt::CaseInsensitive) == 0) {
        *scriptText = BuildModel(options, true, false, false);
        return true;
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral(
            "JM_Bioretention builder does not handle mode: %1").arg(mode);
    }
    return false;
}

} // namespace JMBioretentionBuilder
