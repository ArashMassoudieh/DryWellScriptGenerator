#include "jm_bioretention_builder.h"

#include <QTextStream>
#include <QVector>
#include <QtGlobal>

namespace {

QString n(double value)
{
    return QString::number(value, 'g', 12);
}

int nativeColumnForFacilityColumn(int facilityColumn, int nativeNx)
{
    const int mapped = static_cast<int>((facilityColumn - 0.5) * nativeNx / 4.0) + 1;
    return qBound(1, mapped, nativeNx);
}

QString nativeName(int ix, int iz)
{
    return QStringLiteral("JM Native Soil %1-%2").arg(ix).arg(iz);
}

QString BuildModel(const StarterScriptOptions &options,
                   bool useOptions,
                   bool useCurbChannels,
                   bool useStreetGutters,
                   bool dtSimple,
                   bool test2010 = false)
{
    constexpr int columnCount = 4;

    // Dedicated JM controls for the centered native-soil domain.
    // Examples: nx=4,nz=1 -> one row; nx=1,nz=1 -> one block.
    // Defaults reproduce the original JM 4 x 3 bottom-native grid.
    const int nativeNx = test2010 ? 1 : (useOptions
        ? qMax(1, options.jmNativeHorizontalCells) : 4);
    const int nativeNz = test2010 ? 1 : (useOptions
        ? qMax(1, options.jmNativeVerticalLayers) : 3);

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
    const double underdrainDiameter =
        useOptions && options.jmUnderdrainDiameter > 0.0
        ? options.jmUnderdrainDiameter : 0.1016;

    // BP-01 surveyed/profile geometry.  All elevations are converted to a
    // local datum at CC-104-1 TOP OF MULCH = 155.62 ft.  This keeps the
    // numbers compact while preserving every vertical difference and slope.
    constexpr double ftToM = 0.3048;
    constexpr double datumFt = 155.62;
    const auto localZ = [=](double elevFt) { return (elevFt - datumFt) * ftToM; };

    // Surface/profile values from Section A-A.
    const QVector<double> gutterZ = {
        localZ(158.90), localZ(158.46), localZ(157.84), localZ(157.08)
    };
    const QVector<double> pondBottomZ = {
        localZ(157.49), localZ(157.01), localZ(156.38), localZ(155.62)
    };
    const QVector<double> soilTopZ = {
        localZ(157.24), localZ(156.76), localZ(156.13), localZ(155.37)
    };
    const QVector<double> soilBottomZ = {
        localZ(154.24), localZ(153.76), localZ(153.13), localZ(152.37)
    };
    const QVector<double> chokerBottomZ = {
        localZ(153.99), localZ(153.51), localZ(152.88), localZ(152.12)
    };
    const QVector<double> gravelBottomZ = {
        localZ(151.58), localZ(150.94), localZ(150.15), localZ(149.36)
    };

    // The Section A-A profile explicitly labels the 4-in underdrain inverts
    // and the corresponding 12-in infiltration-sump bottoms.  The sump is
    // measured below the pipe invert and overlaps the gravel storage zone;
    // it is NOT an extra 12 in stacked below the tabulated gravel depth.
    const QVector<double> underdrainInvertZ = {
        localZ(152.40), localZ(151.97), localZ(151.33), localZ(150.54)
    };
    const QVector<double> aggregateBottomZ = {
        localZ(151.40), localZ(150.97), localZ(150.33), localZ(149.54)
    };

    // Ponding-depth labels provide an independent cross-check:
    // 157.49 ft + 8.27 in = 158.18 ft (shown on the profile), etc.
    const QVector<double> pondingDepths = {
        8.27 * 0.0254, 6.45 * 0.0254, 4.86 * 0.0254, 6.47 * 0.0254
    };

    // Preserve the original modeled native-soil domain.  In JM_test this is
    // deliberately one 2.4-m block at the same location/elevation as the
    // supplied reference script; it is not re-sloped with the surface profile.
    const double nativeTotalDepth = 2.4;
    const double nativeLayerDepth = nativeTotalDepth / nativeNz;
    const double originalNativeBottom = -4.3362;
    const double originalNativeTop = originalNativeBottom + nativeTotalDepth;
    const double groundwaterHead = -4.3812;

    // The downstream catch basin is outside the BP-01 profile and its surveyed
    // elevation is not available.  Do not tie it to a pond/aggregate survey
    // elevation.  Instead use a small, explicit downstream drop from the final
    // known underdrain invert so the external boundary remains hydraulically
    // consistent without claiming a field-survey value.
    const double finalUnderdrainInvert = underdrainInvertZ.last();
    const double catchBasinPipeInvert = finalUnderdrainInvert - 0.05;
    const double catchBasinBottom = catchBasinPipeInvert - 0.05;
    const double receivingWaterHead = catchBasinBottom - 0.10;

    QString out;
    QTextStream ts(&out);
    ts.setRealNumberPrecision(12);

    ts << "# JM_Bioretention: four-column model based on BP-01\n";
    ts << "# Columns: CC-101-1, CC-102-1, CC-103-1, CC-104-1\n";
    ts << "# Native grid: nx=" << nativeNx << ", nz=" << nativeNz << "\n";
    if (dtSimple && useStreetGutters) {
        ts << "# Mode: DT Simple with street gutters; side native soils omitted, runoff is routed through gutters and curb cuts, and underdrain is routed serially.\n";
    } else if (dtSimple) {
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

    if (test2010) {
        ts << "# Preset: JM_test / 2010 measured meteorological forcing\n";
        ts << "create source;type=Precipitation,name=Rain,timeseries=Rain_2010.txt\n";
        ts << "create source;type=Evapotranspiration_Penman (Soil),"
              "R_h=Humidity_2010.csv,Temperature=Temp_2010.csv,gamma=66.8,"
              "name=Evapotranspiration_Penman (Soil),solar_radiation=Solar_2010.csv,"
              "solar_scale_fact=0.8,wind_scale_fact=0.8,wind_speed=Wind_2010.csv,"
              "z0=0.0003,z2=2\n";
    } else {
        ts << "create source;type=Precipitation,name=Rain,timeseries=Rain_JM.txt\n";
    }
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
        ts << "create block;type=Soil,Evapotranspiration="
           << ((engineered && test2010) ? "Evapotranspiration_Penman (Soil)" : "")
           << ",K_sat_original="
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
                  "bottom_elevation=" << n(gutterZ[c - 1])
               << ",depth=0,dam_height=0,length=" << n(columnLength)
               << "[m],inflow=0,ag_area=0,non_ag_area=0,"
                  "ag_withdrawal_per_unit_area=0,non_ag_withdrawal_per_unit_area=0,"
                  "_height=100,_width=180,name=JM Curb Channel " << c
               << ",x=" << n(uiX) << ",y=-140\n";
        }
    }

    if (useStreetGutters) {
        for (int c = 1; c <= columnCount; ++c) {
            const double uiX = (c - 1) * 230.0;
            ts << "create block;type=Street Gutter Segment,"
                  "ManningCoeff=0.016,base_width=0,side_slope=40,"
                  "bottom_elevation=" << n(gutterZ[c - 1])
               << ",depth=0,inflow=0,length=" << n(columnLength)
               << "[m],_height=100,_width=180,name=JM Street Gutter " << c
               << ",x=" << n(uiX) << ",y=-140\n";
        }
    }

    for (int c = 1; c <= columnCount; ++c) {
        const double uiX = (c - 1) * 230.0;
        const double actX = (c - 0.5) * columnLength;
        const double pondZ = pondBottomZ[c - 1];
        const double sTop = soilTopZ[c - 1];
        const double sBottom = soilBottomZ[c - 1];
        const double aBottom = aggregateBottomZ[c - 1];
        const double aDepth = sBottom - aBottom;

        ts << "create block;type=Pond,Evapotranspiration=0,Precipitation=Rain,"
              "Storage=0,_height=130,_width=170,act_X=" << n(actX)
           << ",act_Y=" << n(pondZ) << ",alpha=" << n(columnArea)
           << ",alpha_multiplier=1,beta=1,bottom_elevation=" << n(pondZ)
           << ",inflow=0,name=JM Pond " << c
           << ",x=" << n(uiX) << ",y=0\n";

        writeSoil(QStringLiteral("JM Engineered Soil %1").arg(c),
                  columnArea, sBottom, sTop - sBottom,
                  actX, 0.5 * (sTop + sBottom),
                  uiX, 210.0, true);

        ts << "create block;type=Aggregate_storage_layer,K_sat=5000,"
              "_height=150,_width=170,act_X=" << n(actX)
           << ",act_Y=" << n(0.5 * (sBottom + aBottom))
           << ",area=" << n(columnArea)
           << ",bottom_elevation=" << n(aBottom)
           << ",depth=" << n(aDepth)
           << ",inflow=0,name=JM Aggregate " << c
           << ",porosity=0.4,x=" << n(uiX) << ",y=420\n";
    }

    // Bottom native domain.  JM_test and DT-simple/gutter with nx=nz=1
    // preserve the supplied single native block exactly. Other editable
    // grid selections retain the existing configurable-grid behavior.
    if ((test2010 || dtSimple) && nativeNx == 1 && nativeNz == 1) {
        writeSoil(nativeName(1, 1), totalLength * width, originalNativeBottom,
                  nativeTotalDepth, 0.5 * totalLength,
                  0.5 * (originalNativeTop + originalNativeBottom),
                  345.0, 610.0, false);
    } else {
        for (int iz = 1; iz <= nativeNz; ++iz) {
            const double topOffset = (iz - 1) * nativeLayerDepth;
            const double bottomOffset = iz * nativeLayerDepth;
            for (int ix = 1; ix <= nativeNx; ++ix) {
                const double actX = (ix - 0.5) * nativeCellLength;
                const double f = qBound(0.0, actX / totalLength, 1.0);
                const double profileBottom = aggregateBottomZ.first()
                    + f * (aggregateBottomZ.last() - aggregateBottomZ.first());
                const double top = profileBottom - topOffset;
                const double bottom = profileBottom - bottomOffset;
                const double uiX = nativeNx == 1 ? 345.0
                    : (ix - 1) * (690.0 / (nativeNx - 1));
                writeSoil(nativeName(ix, iz), nativeCellArea, bottom,
                          nativeLayerDepth, actX, 0.5 * (top + bottom),
                          uiX, 420.0 + iz * 190.0, false);
            }
        }
    }

    // The older modes retain the surrounding left/right native blocks.
    if (!dtSimple) {
        const double leftZ = pondBottomZ.first();
        const double rightZ = pondBottomZ.last();
        const double sideArea = columnArea;
        const double leftActX = -0.5 * columnLength;
        const double rightActX = totalLength + 0.5 * columnLength;

        writeSoil(QStringLiteral("JM Left Native Media"), sideArea,
                  leftZ - mulch - media, media,
                  leftActX, leftZ - mulch - 0.5 * media,
                  -230.0, 210.0, false);
        writeSoil(QStringLiteral("JM Right Native Media"), sideArea,
                  rightZ - mulch - media, media,
                  rightActX, rightZ - mulch - 0.5 * media,
                  920.0, 210.0, false);
        writeSoil(QStringLiteral("JM Left Native Aggregate"), sideArea,
                  leftZ - mulch - media - choker - gravel - sump, (choker + gravel + sump),
                  leftActX, leftZ - mulch - media - 0.5 * (choker + gravel + sump),
                  -230.0, 420.0, false);
        writeSoil(QStringLiteral("JM Right Native Aggregate"), sideArea,
                  rightZ - mulch - media - choker - gravel - sump, (choker + gravel + sump),
                  rightActX, rightZ - mulch - media - 0.5 * (choker + gravel + sump),
                  920.0, 420.0, false);

        for (int iz = 1; iz <= nativeNz; ++iz) {
            const double topOffset = (iz - 1) * nativeLayerDepth;
            const double bottomOffset = iz * nativeLayerDepth;
            writeSoil(QStringLiteral("JM Left Native %1").arg(iz), sideArea,
                      leftZ - mulch - media - choker - gravel - sump - bottomOffset,
                      nativeLayerDepth, leftActX,
                      leftZ - mulch - media - choker - gravel - sump - 0.5 * (topOffset + bottomOffset),
                      -230.0, 420.0 + iz * 190.0, false);
            writeSoil(QStringLiteral("JM Right Native %1").arg(iz), sideArea,
                      rightZ - mulch - media - choker - gravel - sump - bottomOffset,
                      nativeLayerDepth, rightActX,
                      rightZ - mulch - media - choker - gravel - sump - 0.5 * (topOffset + bottomOffset),
                      920.0, 420.0 + iz * 190.0, false);
        }
    }

    ts << "# JM Catch Basin is downstream of the detailed BP-01 profile; its elevation below is a modeling reference, not a surveyed BP-01 elevation.\n";
    ts << "create block;type=Catch basin,_height=150,_width=190,area=1,"
          "bottom_elevation=" << n(catchBasinBottom)
       << ",inflow=0,name=JM Catch Basin,x=1150,y=200\n";
    ts << "create block;type=fixed_head,Storage=100000,_height=150,_width=220,"
          "head=" << n(receivingWaterHead)
       << ",name=JM Receiving Water,x=1400,y=200\n";
    ts << "create block;type=fixed_head,Storage=100000,_height=150,_width=900,"
          "head=" << n(groundwaterHead)
       << ",name=JM Groundwater,x=0,y=1200\n";

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

    if (!useCurbChannels && !useStreetGutters) {
        for (int i = 0; i < drainageAreas.size(); ++i) {
            ts << "create link;from=JM DA-0" << (i + 1)
               << ",to=JM Pond " << drainageTargets[i]
               << ",type=Catchment_link,name=JM DA-0" << (i + 1)
               << " to Pond " << drainageTargets[i] << "\n";
        }
    } else if (useCurbChannels) {
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
               << n(gutterZ[c - 1])
               << ",name=JM Curb Channel " << c << " to Pond " << c << "\n";
        }
        ts << "create link;from=JM Curb Channel 4,to=JM Catch Basin,"
              "type=wier,alpha=10000,beta=1.5,crest_elevation=0.15,"
              "name=JM Curb Channel Bypass\n";
    } else {
        // Gutter mode is intentionally based on DT Simple: catchments discharge
        // to four street-gutter segments, adjacent gutters route downslope, and
        // each segment enters its corresponding bioretention pond through a
        // flush curb cut. Gutter types are supplied by Sewer_system.json.
        for (int i = 0; i < drainageAreas.size(); ++i) {
            const int gutter = drainageTargets[i];
            ts << "create link;from=JM DA-0" << (i + 1)
               << ",to=JM Street Gutter " << gutter
               << ",type=Catchment_link,name=JM DA-0" << (i + 1)
               << " to Street Gutter " << gutter << "\n";
        }
        for (int c = 1; c < columnCount; ++c) {
            ts << "create link;from=JM Street Gutter " << c
               << ",to=JM Street Gutter " << (c + 1)
               << ",type=Gutter2Gutter_link,name=JM Street Gutter "
               << c << " to " << (c + 1) << "\n";
        }
        for (int c = 1; c <= columnCount; ++c) {
            ts << "create link;from=JM Street Gutter " << c
               << ",to=JM Pond " << c
               << ",type=Curb_cut,crest_offset=0,discharge_coefficient=0.6,"
                  "width=0.2[m],name=JM Street Gutter " << c
               << " to Pond " << c << "\n";
        }

        // Final gutter bypass/outlet to the downstream catch basin.  The catch
        // basin is outside the surveyed BP-01 profile.  crest_offset=0 means
        // flush with the final gutter source datum; it is not an asserted
        // surveyed catch-basin rim elevation.
        ts << "create link;from=JM Street Gutter 4,to=JM Catch Basin,"
              "type=Curb_cut,crest_offset=0,discharge_coefficient=0.6,"
              "width=0.2[m],name=JM Street Gutter 4 to Catch Basin\n";
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

        // In the legacy modes every aggregate has its own pipe to the catch
        // basin. DT Simple instead uses a serial 4-in underdrain and only the
        // final station discharges to the catch basin, matching JM1.ohq.
        if (!dtSimple) {
            const double invert = underdrainInvertZ[c - 1];
            ts << "create link;from=JM Aggregate " << c
               << ",to=JM Catch Basin,type=Sewer_pipe,ManningCoeff=0.011,"
                  "diameter=" << n(underdrainDiameter)
               << "[m],end_elevation=" << n(catchBasinPipeInvert)
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
            const double startInvert = underdrainInvertZ[c - 1];
            const double endInvert = underdrainInvertZ[c];
            ts << "create link;from=JM Aggregate " << c
               << ",to=JM Aggregate " << (c + 1)
               << ",type=Sewer_pipe,ManningCoeff=0.011,diameter="
               << n(underdrainDiameter) << "[m],end_elevation="
               << n(endInvert) << "[m],length=" << n(columnLength)
               << "[m],name=JM Aggregate " << c << " - JM Aggregate " << (c + 1)
               << ",start_elevation=" << n(startInvert) << "[m]\n";
        }
        const double finalInvert = underdrainInvertZ.last();
        ts << "create link;from=JM Aggregate 4,to=JM Catch Basin,"
              "type=Sewer_pipe,ManningCoeff=0.011,diameter="
           << n(underdrainDiameter) << "[m],end_elevation="
           << n(catchBasinPipeInvert) << "[m],length=" << n(columnLength)
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
          "type=Sewer_pipe,ManningCoeff=0.011,diameter=0.2[m],end_elevation="
       << n(receivingWaterHead) << "[m],length=2[m],name=JM Catch Basin Outlet,"
          "start_elevation=" << n(catchBasinPipeInvert) << "[m]\n";

    if (test2010) {
        // 2010 real-data window used by the JM_test reference file.
        ts << "setvalue; object=system, quantity=simulation_start_time, value=40178.8\n";
        ts << "setvalue; object=system, quantity=simulation_end_time, value=40541.8\n";
        ts << "setvalue; object=system, quantity=outputfile, value=output.txt\n";
    }

    return out;
}

} // namespace

namespace JMBioretentionBuilder
{

QString FullReferenceScript()
{
    StarterScriptOptions defaults;
    return BuildModel(defaults, false, false, false, false);
}

QString ChannelReferenceScript()
{
    StarterScriptOptions defaults;
    return BuildModel(defaults, false, true, false, false);
}

QString CurbChannelReferenceScript()
{
    return ChannelReferenceScript();
}

QString DtSimpleReferenceScript()
{
    StarterScriptOptions defaults;
    return BuildModel(defaults, false, false, false, true);
}


QString Test2010ReferenceScript()
{
    StarterScriptOptions defaults;
    return BuildModel(defaults, false, false, true, true, true);
}

QString DtSimpleGutterReferenceScript()
{
    StarterScriptOptions defaults;
    return BuildModel(defaults, false, false, true, true);
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
        *scriptText = BuildModel(options, true, true, false, false);
        return true;
    }

    if (mode.compare(QStringLiteral("JMTest"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("JM_test"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("Test2010"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("DTSimpleGutter2010"), Qt::CaseInsensitive) == 0) {
        *scriptText = BuildModel(options, true, false, true, true, true);
        return true;
    }

    if (mode.compare(QStringLiteral("DTSimpleGutter"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("DT_Simple_Gutter"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("GutterSimple"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("SimpleGutter"), Qt::CaseInsensitive) == 0) {
        *scriptText = BuildModel(options, true, false, true, true);
        return true;
    }

    if (mode.compare(QStringLiteral("DTSimple"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("DT_Simple"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("DigitalTwinSimple"), Qt::CaseInsensitive) == 0) {
        *scriptText = BuildModel(options, true, false, false, true);
        return true;
    }

    if (mode.isEmpty()
        || mode.compare(QStringLiteral("SoftReference"), Qt::CaseInsensitive) == 0) {
        *scriptText = BuildModel(options, true, false, false, false);
        return true;
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral(
            "JM_Bioretention builder does not handle mode: %1").arg(mode);
    }
    return false;
}

} // namespace JMBioretentionBuilder
