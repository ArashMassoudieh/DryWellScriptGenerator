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
    // John McCormack Road bioretention model.
    //
    // R-style five-column cross-section at every elevation:
    //
    //   Left 2 -- Left 1 -- Center -- Right 1 -- Right 2
    //
    // The center material changes with depth:
    // engineered soil -> choker -> gravel -> infiltration sump -> native soil.
    // Side-native columns continue beside every center layer, so there are no
    // long links that jump across the aggregate/storage layers.
    //
    // JM drawing dimensions (SI):
    //   mulch              0.0762 m
    //   bioretention soil  0.9144 m
    //   choker             0.0762 m
    //   gravel             0.6096 m
    //   underdrain dia.    0.1016 m
    //   infiltration sump  0.3048 m
    //   ponding depth      0.20828 m

    constexpr int mediaNz = 8;
    constexpr int nativeBelowNz = 23;
    constexpr int nativeSideNx = 2;

    const double length = useOptions && options.jmLength > 0.0
        ? options.jmLength : 12.192;
    const double width = useOptions && options.jmWidth > 0.0
        ? options.jmWidth : 1.524;
    const double footprintArea = length * width;

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
    const double pondingDepth = 0.20828;

    const double mediaDz = media / mediaNz;

    // Match the deeper R native-soil profile: three transition rows followed
    // by 0.4572-m rows. The total is 23 rows below the JM sump.
    QVector<double> nativeDepths;
    nativeDepths.reserve(nativeBelowNz);
    nativeDepths << 0.1016 << 0.3048 << 0.3048;
    while (nativeDepths.size() < nativeBelowNz) {
        nativeDepths << 0.4572;
    }

    double nativeBelowDepth = 0.0;
    for (double depth : nativeDepths) {
        nativeBelowDepth += depth;
    }

    // Local datum: top of mulch = 0 m.
    const double mediaTop = -mulch;
    const double mediaBottom = mediaTop - media;
    const double chokerBottom = mediaBottom - choker;
    const double gravelBottom = chokerBottom - gravel;
    const double sumpBottom = gravelBottom - sump;
    const double groundwaterHead = sumpBottom - nativeBelowDepth;

    QString out;
    QTextStream ts(&out);
    ts.setRealNumberPrecision(12);

    ts << "# JM_Bioretention: continuous R-style five-column cross-section\n";
    ts << "# Units: SI; all dimensions and elevations are metres.\n";
    ts << "# media_nz=" << mediaNz
       << ", native_side_nx=" << nativeSideNx
       << ", native_below_nz=" << nativeBelowNz << "\n";
    ts << "# JM footprint: length=" << n(length)
       << " m, width=" << n(width)
       << " m, area=" << n(footprintArea) << " m2\n";

    ts << "loadtemplate; filename=<template_dir>/main_components.json\n";
    ts << "addtemplate; filename=<template_dir>/Pond_Plugin.json\n";
    ts << "addtemplate; filename=<template_dir>/unsaturated_soil.json\n";
    ts << "addtemplate; filename=<template_dir>/Well.json\n";
    ts << "addtemplate; filename=<template_dir>/Sewer_system.json\n";
    ts << "addtemplate; filename=<template_dir>/soil_evapotranspiration_models.json\n";
    ts << "addtemplate; filename=<template_dir>/evapotranspiration_models.json\n";
    ts << "addtemplate; filename=<template_dir>/pipe_pump_tank.json\n";

    ts << "create source;type=Precipitation,name=Rain,timeseries=Rain_JM.txt\n";
    ts << "create parameter;type=Parameter,high=10,low=0.1,name=JM_KS_scale_factor,prior_distribution=log-normal,value=1\n";
    ts << "create parameter;type=Parameter,high=10,low=0.1,name=JM_Eng_Soil_alpha,prior_distribution=log-normal,value=1\n";
    ts << "create parameter;type=Parameter,high=3,low=1.01,name=JM_Eng_Soil_n,prior_distribution=log-normal,value=1.56\n";

    const double contributingArea =
        useOptions && options.jmCatchmentArea > 0.0
        ? options.jmCatchmentArea : 1000.0;

    ts << "create block;type=Catchment,Evapotranspiration=,ManningCoeff=0.03,Precipitation=Rain,"
          "Runoff_coeff=0.8,Slope=0.02,Width=30,_height=300,_width=500,area="
       << n(contributingArea)
       << "[m~^2],depression_storage=0,depth=0,elevation=0,inflow=,loss_coefficient=0,"
          "name=JM Contributing Catchment,x=-700,y=-250\n";

    ts << "create block;type=Catchment,Evapotranspiration=,ManningCoeff=0.01,Precipitation=,"
          "Runoff_coeff=1,Slope=0.01,Width=" << n(width)
       << ",_height=150,_width=220,area=" << n(footprintArea)
       << "[m~^2],depression_storage=" << n(pondingDepth)
       << ",depth=0,elevation=0,inflow=,loss_coefficient=0,"
          "name=JM Catchment,x=0,y=-250\n";

    const auto writeSoil = [&](const QString &name,
                               double area,
                               double bottom,
                               double depth,
                               double actualX,
                               double actualY,
                               double uiX,
                               double uiY,
                               bool engineered) {
        ts << "create block;type=Soil,Evapotranspiration=,K_sat_original="
           << (engineered ? "50" : "0.25")
           << ",K_sat_scale_factor="
           << (engineered ? "JM_KS_scale_factor" : "1")
           << ",MC_to_EC_Threshold_Moisture=0,MC_to_EC_coefficient=0,"
              "MC_to_EC_exponent=0,_height=100,_width=150,act_X="
           << n(actualX)
           << ",act_Y=" << n(actualY)
           << ",alpha=" << (engineered ? "JM_Eng_Soil_alpha" : "3.6")
           << ",aniso_ratio=1,area=" << n(area)
           << ",bottom_elevation=" << n(bottom)
           << ",depth=" << n(depth)
           << ",n=" << (engineered ? "JM_Eng_Soil_n" : "1.56")
           << ",name=" << name
           << ",specific_storage=0.01,theta=0.12,theta_res="
           << (engineered ? "0.05" : "0.078")
           << ",theta_sat=" << (engineered ? "0.45" : "0.43")
           << ",x=" << n(uiX) << ",y=" << n(uiY) << "\n";
    };

    const auto writeAggregate = [&](const QString &name,
                                    double kSat,
                                    double porosity,
                                    double bottom,
                                    double depth,
                                    double uiY) {
        ts << "create block;type=Aggregate_storage_layer,K_sat=" << n(kSat)
           << ",_height=105,_width=150,area=" << n(footprintArea)
           << ",bottom_elevation=" << n(bottom)
           << ",depth=" << n(depth)
           << ",inflow=,name=" << name
           << ",porosity=" << n(porosity)
           << ",x=0,y=" << n(uiY) << "\n";
    };

    const auto writeSideRow = [&](const QString &rowName,
                                  double top,
                                  double depth,
                                  double uiY) {
        const double bottom = top - depth;
        const double actualY = (top + bottom) / 2.0;
        for (int c = 1; c <= nativeSideNx; ++c) {
            writeSoil(QStringLiteral("JM Left Native Soil (%1$%2)")
                          .arg(rowName).arg(c),
                      footprintArea, bottom, depth,
                      -c * width, actualY, -220.0 * c, uiY, false);
            writeSoil(QStringLiteral("JM Right Native Soil (%1$%2)")
                          .arg(rowName).arg(c),
                      footprintArea, bottom, depth,
                      c * width, actualY, 220.0 * c, uiY, false);
        }
    };

    // Eight engineered-media rows, each with two native columns per side.
    for (int k = 1; k <= mediaNz; ++k) {
        const double top = mediaTop - (k - 1) * mediaDz;
        const double bottom = top - mediaDz;
        const double actualY = (top + bottom) / 2.0;
        const double uiY = (k - 1) * 180.0;

        writeSoil(QStringLiteral("JM Engineered Soil (%1)").arg(k),
                  footprintArea, bottom, mediaDz,
                  0.0, actualY, 0.0, uiY, true);

        for (int c = 1; c <= nativeSideNx; ++c) {
            writeSoil(QStringLiteral("JM Left Native Soil (%1$%2)").arg(k).arg(c),
                      footprintArea, bottom, mediaDz,
                      -c * width, actualY, -220.0 * c, uiY, false);
            writeSoil(QStringLiteral("JM Right Native Soil (%1$%2)").arg(k).arg(c),
                      footprintArea, bottom, mediaDz,
                      c * width, actualY, 220.0 * c, uiY, false);
        }
    }

    // Continue all five columns through the choker, gravel and sump depths.
    const double chokerUiY = mediaNz * 180.0;
    const double gravelUiY = (mediaNz + 1) * 180.0;
    const double sumpUiY = (mediaNz + 2) * 180.0;

    writeAggregate(QStringLiteral("JM Choker"), 500.0, 0.40,
                   chokerBottom, choker, chokerUiY);
    writeSideRow(QStringLiteral("Choker"), mediaBottom, choker, chokerUiY);

    writeAggregate(QStringLiteral("JM Gravel"), 5000.0, 0.40,
                   gravelBottom, gravel, gravelUiY);
    writeSideRow(QStringLiteral("Gravel"), chokerBottom, gravel, gravelUiY);

    // The 12-in infiltration sump is stone storage beneath the underdrain,
    // therefore it remains an aggregate-storage block rather than a Soil block.
    writeAggregate(QStringLiteral("JM Infiltration Sump"), 5000.0, 0.40,
                   sumpBottom, sump, sumpUiY);
    writeSideRow(QStringLiteral("Sump"), gravelBottom, sump, sumpUiY);

    // Five-column native-soil grid below the sump.
    double nativeTop = sumpBottom;
    for (int k = 1; k <= nativeBelowNz; ++k) {
        const double nativeDz = nativeDepths[k - 1];
        const double bottom = nativeTop - nativeDz;
        const double actualY = (nativeTop + bottom) / 2.0;
        const double uiY = (mediaNz + 2 + k) * 180.0;

        for (int c = 1; c <= nativeSideNx; ++c) {
            writeSoil(QStringLiteral("JM Left Bottom Native Soil (%1$%2)")
                          .arg(k).arg(c),
                      footprintArea, bottom, nativeDz,
                      -c * width, actualY, -220.0 * c, uiY, false);
        }

        writeSoil(QStringLiteral("JM Bottom Native Soil (%1)").arg(k),
                  footprintArea, bottom, nativeDz,
                  0.0, actualY, 0.0, uiY, false);

        for (int c = 1; c <= nativeSideNx; ++c) {
            writeSoil(QStringLiteral("JM Right Bottom Native Soil (%1$%2)")
                          .arg(k).arg(c),
                      footprintArea, bottom, nativeDz,
                      c * width, actualY, 220.0 * c, uiY, false);
        }

        nativeTop = bottom;
    }

    ts << "create block;type=Pipe,name=JM Underdrain,_width=220,_height=120,x=500,y="
       << n(gravelUiY)
       << ",diameter=" << n(underdrainDiameter)
       << "[m],length=" << n(length)
       << "[m],slope=0.005\n";

    ts << "create block;type=fixed_head,name=JM Outlet,_width=180,_height=120,"
          "x=760,y=-250,head=0[m],Storage=100000[m~^3]\n";

    ts << "create block;type=fixed_head,name=JM Groundwater,_width=180,_height=120,x=0,y="
       << n((mediaNz + nativeBelowNz + 4) * 180.0)
       << ",head=" << n(groundwaterHead)
       << "[m],Storage=100000[m~^3]\n";

    // Surface routing.
    ts << "create link;from=JM Contributing Catchment,to=JM Catchment,"
          "type=Catchment_link,name=JM Contributing Catchment - JM Catchment\n";
    ts << "create link;from=JM Catchment,to=JM Engineered Soil (1),"
          "type=surfacewater_to_soil_link,name=JM Catchment - Engineered Soil 1\n";

    // Engineered-media rows: L2--L1--Center--R1--R2.
    for (int k = 1; k <= mediaNz; ++k) {
        const double hArea = mediaDz * length;

        ts << "create link;from=JM Engineered Soil (" << k
           << "),to=JM Left Native Soil (" << k << "$1),"
              "type=soil_to_soil_H_link,name=JM Engineered - Left Native " << k
           << ",length=" << n(width / 2.0)
           << "[m],area=" << n(hArea) << "[m~^2]\n";

        ts << "create link;from=JM Engineered Soil (" << k
           << "),to=JM Right Native Soil (" << k << "$1),"
              "type=soil_to_soil_H_link,name=JM Engineered - Right Native " << k
           << ",length=" << n(width / 2.0)
           << "[m],area=" << n(hArea) << "[m~^2]\n";

        for (int c = 1; c < nativeSideNx; ++c) {
            ts << "create link;from=JM Left Native Soil (" << k << "$" << c
               << "),to=JM Left Native Soil (" << k << "$" << c + 1
               << "),type=soil_to_soil_H_link,name=JM Left Native Horizontal "
               << k << "-" << c << ",length=" << n(width)
               << "[m],area=" << n(hArea) << "[m~^2]\n";

            ts << "create link;from=JM Right Native Soil (" << k << "$" << c
               << "),to=JM Right Native Soil (" << k << "$" << c + 1
               << "),type=soil_to_soil_H_link,name=JM Right Native Horizontal "
               << k << "-" << c << ",length=" << n(width)
               << "[m],area=" << n(hArea) << "[m~^2]\n";
        }

        if (k < mediaNz) {
            ts << "create link;from=JM Engineered Soil (" << k
               << "),to=JM Engineered Soil (" << k + 1
               << "),type=soil_to_soil_link,name=JM Engineered Vertical "
               << k << "\n";

            for (int c = 1; c <= nativeSideNx; ++c) {
                ts << "create link;from=JM Left Native Soil (" << k << "$" << c
                   << "),to=JM Left Native Soil (" << k + 1 << "$" << c
                   << "),type=soil_to_soil_link,name=JM Left Native Vertical "
                   << k << "-" << c << "\n";

                ts << "create link;from=JM Right Native Soil (" << k << "$" << c
                   << "),to=JM Right Native Soil (" << k + 1 << "$" << c
                   << "),type=soil_to_soil_link,name=JM Right Native Vertical "
                   << k << "-" << c << "\n";
            }
        }
    }

    // Center material stack. `from` and `to` are consistently ordered from
    // the shallower block to the deeper block so the displayed arrows point
    // downward. These exchange links remain hydraulically bidirectional.
    ts << "create link;from=JM Engineered Soil (" << mediaNz
       << "),to=JM Choker,type=aggregate_to_soil_link,"
          "name=JM Engineered Soil - Choker\n";

    ts << "create link;from=JM Choker,to=JM Gravel,"
          "type=aggregate2aggregate_H_Link,length=" << n(choker)
       << "[m],name=JM Choker - Gravel,width=" << n(width) << "[m]\n";

    ts << "create link;from=JM Gravel,to=JM Infiltration Sump,"
          "type=aggregate2aggregate_H_Link,length=" << n(gravel)
       << "[m],name=JM Gravel - Infiltration Sump,width="
       << n(width) << "[m]\n";

    ts << "create link;from=JM Infiltration Sump,to=JM Bottom Native Soil (1),"
          "type=aggregate_to_soil_link,name=JM Infiltration Sump - Bottom Native Soil\n";

    const auto writeTransitionRowLinks = [&](const QString &rowName,
                                             const QString &centerName,
                                             double rowDepth) {
        const double hArea = rowDepth * length;

        // Center aggregate/storage to the immediately adjacent native soils.
        ts << "create link;from=" << centerName
           << ",to=JM Left Native Soil (" << rowName << "$1),"
              "type=aggregate_to_soil_link,name=" << centerName
           << " - Left Native " << rowName << "\n";

        ts << "create link;from=" << centerName
           << ",to=JM Right Native Soil (" << rowName << "$1),"
              "type=aggregate_to_soil_link,name=" << centerName
           << " - Right Native " << rowName << "\n";

        for (int c = 1; c < nativeSideNx; ++c) {
            ts << "create link;from=JM Left Native Soil (" << rowName << "$" << c
               << "),to=JM Left Native Soil (" << rowName << "$" << c + 1
               << "),type=soil_to_soil_H_link,name=JM Left Native Horizontal "
               << rowName << "-" << c << ",length=" << n(width)
               << "[m],area=" << n(hArea) << "[m~^2]\n";

            ts << "create link;from=JM Right Native Soil (" << rowName << "$" << c
               << "),to=JM Right Native Soil (" << rowName << "$" << c + 1
               << "),type=soil_to_soil_H_link,name=JM Right Native Horizontal "
               << rowName << "-" << c << ",length=" << n(width)
               << "[m],area=" << n(hArea) << "[m~^2]\n";
        }
    };

    writeTransitionRowLinks(QStringLiteral("Choker"),
                            QStringLiteral("JM Choker"), choker);
    writeTransitionRowLinks(QStringLiteral("Gravel"),
                            QStringLiteral("JM Gravel"), gravel);
    writeTransitionRowLinks(QStringLiteral("Sump"),
                            QStringLiteral("JM Infiltration Sump"), sump);

    // Continue each side-native column through every intermediate layer.
    for (int c = 1; c <= nativeSideNx; ++c) {
        ts << "create link;from=JM Left Native Soil (" << mediaNz << "$" << c
           << "),to=JM Left Native Soil (Choker$" << c
           << "),type=soil_to_soil_link,name=JM Left Media - Choker Side "
           << c << "\n";
        ts << "create link;from=JM Left Native Soil (Choker$" << c
           << "),to=JM Left Native Soil (Gravel$" << c
           << "),type=soil_to_soil_link,name=JM Left Choker - Gravel Side "
           << c << "\n";
        ts << "create link;from=JM Left Native Soil (Gravel$" << c
           << "),to=JM Left Native Soil (Sump$" << c
           << "),type=soil_to_soil_link,name=JM Left Gravel - Sump Side "
           << c << "\n";
        ts << "create link;from=JM Left Native Soil (Sump$" << c
           << "),to=JM Left Bottom Native Soil (1$" << c
           << "),type=soil_to_soil_link,name=JM Left Sump Side - Bottom Native "
           << c << "\n";

        ts << "create link;from=JM Right Native Soil (" << mediaNz << "$" << c
           << "),to=JM Right Native Soil (Choker$" << c
           << "),type=soil_to_soil_link,name=JM Right Media - Choker Side "
           << c << "\n";
        ts << "create link;from=JM Right Native Soil (Choker$" << c
           << "),to=JM Right Native Soil (Gravel$" << c
           << "),type=soil_to_soil_link,name=JM Right Choker - Gravel Side "
           << c << "\n";
        ts << "create link;from=JM Right Native Soil (Gravel$" << c
           << "),to=JM Right Native Soil (Sump$" << c
           << "),type=soil_to_soil_link,name=JM Right Gravel - Sump Side "
           << c << "\n";
        ts << "create link;from=JM Right Native Soil (Sump$" << c
           << "),to=JM Right Bottom Native Soil (1$" << c
           << "),type=soil_to_soil_link,name=JM Right Sump Side - Bottom Native "
           << c << "\n";
    }

    // Deeper five-column native grid.
    for (int k = 1; k <= nativeBelowNz; ++k) {
        const double hArea = nativeDepths[k - 1] * length;

        ts << "create link;from=JM Bottom Native Soil (" << k
           << "),to=JM Left Bottom Native Soil (" << k << "$1),"
              "type=soil_to_soil_H_link,name=JM Bottom - Left Bottom " << k
           << ",length=" << n(width / 2.0)
           << "[m],area=" << n(hArea) << "[m~^2]\n";

        ts << "create link;from=JM Bottom Native Soil (" << k
           << "),to=JM Right Bottom Native Soil (" << k << "$1),"
              "type=soil_to_soil_H_link,name=JM Bottom - Right Bottom " << k
           << ",length=" << n(width / 2.0)
           << "[m],area=" << n(hArea) << "[m~^2]\n";

        for (int c = 1; c < nativeSideNx; ++c) {
            ts << "create link;from=JM Left Bottom Native Soil (" << k << "$" << c
               << "),to=JM Left Bottom Native Soil (" << k << "$" << c + 1
               << "),type=soil_to_soil_H_link,name=JM Left Bottom Horizontal "
               << k << "-" << c << ",length=" << n(width)
               << "[m],area=" << n(hArea) << "[m~^2]\n";

            ts << "create link;from=JM Right Bottom Native Soil (" << k << "$" << c
               << "),to=JM Right Bottom Native Soil (" << k << "$" << c + 1
               << "),type=soil_to_soil_H_link,name=JM Right Bottom Horizontal "
               << k << "-" << c << ",length=" << n(width)
               << "[m],area=" << n(hArea) << "[m~^2]\n";
        }

        if (k < nativeBelowNz) {
            ts << "create link;from=JM Bottom Native Soil (" << k
               << "),to=JM Bottom Native Soil (" << k + 1
               << "),type=soil_to_soil_link,name=JM Bottom Vertical "
               << k << "\n";

            for (int c = 1; c <= nativeSideNx; ++c) {
                ts << "create link;from=JM Left Bottom Native Soil (" << k << "$" << c
                   << "),to=JM Left Bottom Native Soil (" << k + 1 << "$" << c
                   << "),type=soil_to_soil_link,name=JM Left Bottom Vertical "
                   << k << "-" << c << "\n";

                ts << "create link;from=JM Right Bottom Native Soil (" << k << "$" << c
                   << "),to=JM Right Bottom Native Soil (" << k + 1 << "$" << c
                   << "),type=soil_to_soil_link,name=JM Right Bottom Vertical "
                   << k << "-" << c << "\n";
            }
        }
    }

    // Underdrain and boundaries.
    ts << "create link;from=JM Gravel,to=JM Underdrain,"
          "type=aggregate2aggregate_H_Link,length=" << n(width / 2.0)
       << "[m],name=JM Gravel - Underdrain,width=" << n(length) << "[m]\n";

    ts << "create link;from=JM Bottom Native Soil (" << nativeBelowNz
       << "),to=JM Groundwater,type=soil_to_fixedhead_link,"
          "name=JM Bottom Native Soil - Groundwater\n";

    ts << "create link;from=JM Underdrain,to=JM Outlet,type=Sewer_pipe,"
          "ManningCoeff=0.011,diameter=" << n(underdrainDiameter)
       << "[m],end_elevation=0,length=" << n(length)
       << "[m],name=JM Underdrain - Outlet,start_elevation=0.05[m]\n";

    ts << "create link;from=JM Catchment,to=JM Outlet,type=Sewer_pipe,"
          "ManningCoeff=0.011,diameter=0.15[m],end_elevation=0,length=1[m],"
          "name=JM Surface Overflow,start_elevation="
       << n(pondingDepth) << "[m]\n";

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
    return QStringLiteral("JM Contributing Catchment");
}

QString RainfallTargetObject()
{
    return QStringLiteral("Rain");
}

QString ContributingCatchmentObject()
{
    return QStringLiteral("JM Contributing Catchment");
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
