#include "jm_bioretention_builder.h"

#include <QTextStream>
#include <QtGlobal>

namespace {

QString n(double value)
{
    return QString::number(value, 'g', 12);
}

QString BuildReference(const StarterScriptOptions &options)
{
    // John McCormack Road bioretention model.
    //
    // Topology intentionally follows the clean R-bioswale cross-section:
    //   * one vertical engineered-soil column;
    //   * native-soil columns immediately to the left and right;
    //   * native soil below the facility;
    //   * links only between immediate horizontal/vertical neighbours;
    //   * one groundwater fixed-head boundary at the bottom;
    //   * one outlet fixed-head boundary for the underdrain and surface overflow.
    //
    // JM drawing dimensions (all SI):
    //   mulch              0.0762 m
    //   bioretention soil  0.9144 m
    //   choker             0.0762 m
    //   gravel             0.6096 m
    //   underdrain dia.    0.1016 m
    //   infiltration sump  0.3048 m
    //   ponding depth      0.20828 m

    constexpr int mediaNz = 8;
    constexpr int nativeBelowNz = 4;

    const double length = options.jmLength > 0.0 ? options.jmLength : 12.192;
    const double width = options.jmWidth > 0.0 ? options.jmWidth : 1.524;
    const double footprintArea = length * width;

    const double mulch = options.jmMulchDepth > 0.0 ? options.jmMulchDepth : 0.0762;
    const double media = options.jmMediaDepth > 0.0 ? options.jmMediaDepth : 0.9144;
    const double choker = options.jmChokerDepth > 0.0 ? options.jmChokerDepth : 0.0762;
    const double gravel = options.jmGravelDepth > 0.0 ? options.jmGravelDepth : 0.6096;
    const double sump = options.jmSumpDepth > 0.0 ? options.jmSumpDepth : 0.3048;
    const double underdrainDiameter = options.jmUnderdrainDiameter > 0.0
        ? options.jmUnderdrainDiameter : 0.1016;
    const double pondingDepth = 0.20828;

    const double mediaDz = media / mediaNz;
    const double nativeBelowDepth = 1.8288; // 6 ft represented below the infiltration sump.
    const double nativeBelowDz = nativeBelowDepth / nativeBelowNz;

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

    ts << "# JM_Bioretention: R-style cross-section using JM dimensions\n";
    ts << "# Units: SI; all dimensions and elevations are metres.\n";
    ts << "# One engineered-soil column; media_nz=" << mediaNz
       << ", native_below_nz=" << nativeBelowNz << "\n";
    ts << "# JM footprint: length=" << n(length) << " m, width=" << n(width)
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

    const double contributingArea = options.jmCatchmentArea > 0.0
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
       << ",depth=0,elevation=0,inflow=,loss_coefficient=0,name=JM Catchment,x=0,y=-250\n";

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
           << ",K_sat_scale_factor=" << (engineered ? "JM_KS_scale_factor" : "1")
           << ",MC_to_EC_Threshold_Moisture=0,MC_to_EC_coefficient=0,MC_to_EC_exponent=0,"
              "_height=100,_width=150,act_X=" << n(actualX)
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

    // Engineered media and immediate native-soil neighbours.
    for (int k = 1; k <= mediaNz; ++k) {
        const double top = mediaTop - (k - 1) * mediaDz;
        const double bottom = top - mediaDz;
        const double actualY = (top + bottom) / 2.0;
        const double uiY = (k - 1) * 180.0;

        writeSoil(QStringLiteral("JM Engineered Soil (%1)").arg(k),
                  footprintArea, bottom, mediaDz, 0.0, actualY, 0.0, uiY, true);

        // Side columns use a representative native-soil plan area equal to the
        // JM footprint. This keeps the R-style topology without longitudinal
        // duplication of the bioretention column.
        writeSoil(QStringLiteral("JM Left Native Soil (%1)").arg(k),
                  footprintArea, bottom, mediaDz, -width, actualY, -220.0, uiY, false);
        writeSoil(QStringLiteral("JM Right Native Soil (%1)").arg(k),
                  footprintArea, bottom, mediaDz, width, actualY, 220.0, uiY, false);
    }

    ts << "create block;type=Aggregate_storage_layer,K_sat=500,_height=100,_width=150,area="
       << n(footprintArea) << ",bottom_elevation=" << n(chokerBottom)
       << ",depth=" << n(choker) << ",inflow=,name=JM Choker,x=0,y="
       << n(mediaNz * 180.0) << ",porosity=0.40\n";

    ts << "create block;type=Aggregate_storage_layer,K_sat=5000,_height=110,_width=150,area="
       << n(footprintArea) << ",bottom_elevation=" << n(gravelBottom)
       << ",depth=" << n(gravel) << ",inflow=,name=JM Gravel,x=0,y="
       << n((mediaNz + 1) * 180.0) << ",porosity=0.40\n";

    writeSoil(QStringLiteral("JM Infiltration Sump"), footprintArea, sumpBottom, sump,
              0.0, gravelBottom - sump / 2.0, 0.0, (mediaNz + 2) * 180.0, false);

    // Three-column native-soil grid below the sump.
    for (int k = 1; k <= nativeBelowNz; ++k) {
        const double top = sumpBottom - (k - 1) * nativeBelowDz;
        const double bottom = top - nativeBelowDz;
        const double actualY = (top + bottom) / 2.0;
        const double uiY = (mediaNz + 2 + k) * 180.0;

        writeSoil(QStringLiteral("JM Left Bottom Native Soil (%1)").arg(k),
                  footprintArea, bottom, nativeBelowDz, -width, actualY, -220.0, uiY, false);
        writeSoil(QStringLiteral("JM Bottom Native Soil (%1)").arg(k),
                  footprintArea, bottom, nativeBelowDz, 0.0, actualY, 0.0, uiY, false);
        writeSoil(QStringLiteral("JM Right Bottom Native Soil (%1)").arg(k),
                  footprintArea, bottom, nativeBelowDz, width, actualY, 220.0, uiY, false);
    }

    ts << "create block;type=Pipe,name=JM Underdrain,_width=220,_height=120,x=500,y="
       << n((mediaNz + 1) * 180.0)
       << ",diameter=" << n(underdrainDiameter) << "[m],length=" << n(length)
       << "[m],slope=0.005\n";

    ts << "create block;type=fixed_head,name=JM Outlet,_width=180,_height=120,x=760,y=-250,"
          "head=0[m],Storage=100000[m~^3]\n";
    ts << "create block;type=fixed_head,name=JM Groundwater,_width=180,_height=120,x=0,y="
       << n((mediaNz + nativeBelowNz + 4) * 180.0)
       << ",head=" << n(groundwaterHead) << "[m],Storage=100000[m~^3]\n";

    // Surface routing.
    ts << "create link;from=JM Contributing Catchment,to=JM Catchment,type=Catchment_link,"
          "name=JM Contributing Catchment - JM Catchment\n";
    ts << "create link;from=JM Catchment,to=JM Engineered Soil (1),type=surfacewater_to_soil_link,"
          "name=JM Catchment - Engineered Soil 1\n";

    // Media grid: vertical center/side connections and horizontal adjacency.
    for (int k = 1; k <= mediaNz; ++k) {
        ts << "create link;from=JM Engineered Soil (" << k << "),to=JM Left Native Soil (" << k
           << "),type=soil_to_soil_H_link,name=JM Engineered - Left Native " << k
           << ",length=" << n(width / 2.0) << "[m],area=" << n(mediaDz * length) << "[m~^2]\n";
        ts << "create link;from=JM Engineered Soil (" << k << "),to=JM Right Native Soil (" << k
           << "),type=soil_to_soil_H_link,name=JM Engineered - Right Native " << k
           << ",length=" << n(width / 2.0) << "[m],area=" << n(mediaDz * length) << "[m~^2]\n";

        if (k < mediaNz) {
            ts << "create link;from=JM Engineered Soil (" << k << "),to=JM Engineered Soil (" << k + 1
               << "),type=soil_to_soil_link,name=JM Engineered Vertical " << k << "\n";
            ts << "create link;from=JM Left Native Soil (" << k << "),to=JM Left Native Soil (" << k + 1
               << "),type=soil_to_soil_link,name=JM Left Native Vertical " << k << "\n";
            ts << "create link;from=JM Right Native Soil (" << k << "),to=JM Right Native Soil (" << k + 1
               << "),type=soil_to_soil_link,name=JM Right Native Vertical " << k << "\n";
        }
    }

    // Material stack beneath the final engineered-soil cell.
    ts << "create link;from=JM Choker,to=JM Engineered Soil (" << mediaNz
       << "),type=aggregate_to_soil_link,name=JM Choker - Engineered Soil\n";
    ts << "create link;from=JM Choker,to=JM Gravel,type=aggregate2aggregate_H_Link,length="
       << n(choker) << "[m],name=JM Choker - Gravel,width=" << n(width) << "[m]\n";
    ts << "create link;from=JM Gravel,to=JM Infiltration Sump,type=aggregate_to_soil_link,"
          "name=JM Gravel - Infiltration Sump\n";

    // Join the three media-level columns to the three native columns below.
    ts << "create link;from=JM Left Native Soil (" << mediaNz
       << "),to=JM Left Bottom Native Soil (1),type=soil_to_soil_link,"
          "name=JM Left Native - Left Bottom Native\n";
    ts << "create link;from=JM Infiltration Sump,to=JM Bottom Native Soil (1),type=soil_to_soil_link,"
          "name=JM Sump - Bottom Native\n";
    ts << "create link;from=JM Right Native Soil (" << mediaNz
       << "),to=JM Right Bottom Native Soil (1),type=soil_to_soil_link,"
          "name=JM Right Native - Right Bottom Native\n";

    // Native grid below: each block connects only to its immediate neighbours.
    for (int k = 1; k <= nativeBelowNz; ++k) {
        ts << "create link;from=JM Bottom Native Soil (" << k << "),to=JM Left Bottom Native Soil (" << k
           << "),type=soil_to_soil_H_link,name=JM Bottom - Left Bottom " << k
           << ",length=" << n(width / 2.0) << "[m],area=" << n(nativeBelowDz * length) << "[m~^2]\n";
        ts << "create link;from=JM Bottom Native Soil (" << k << "),to=JM Right Bottom Native Soil (" << k
           << "),type=soil_to_soil_H_link,name=JM Bottom - Right Bottom " << k
           << ",length=" << n(width / 2.0) << "[m],area=" << n(nativeBelowDz * length) << "[m~^2]\n";

        if (k < nativeBelowNz) {
            ts << "create link;from=JM Left Bottom Native Soil (" << k << "),to=JM Left Bottom Native Soil (" << k + 1
               << "),type=soil_to_soil_link,name=JM Left Bottom Vertical " << k << "\n";
            ts << "create link;from=JM Bottom Native Soil (" << k << "),to=JM Bottom Native Soil (" << k + 1
               << "),type=soil_to_soil_link,name=JM Bottom Vertical " << k << "\n";
            ts << "create link;from=JM Right Bottom Native Soil (" << k << "),to=JM Right Bottom Native Soil (" << k + 1
               << "),type=soil_to_soil_link,name=JM Right Bottom Vertical " << k << "\n";
        }
    }

    // One underdrain connection, one groundwater boundary connection, and one
    // surface overflow. No star-shaped links to fixed-head blocks.
    ts << "create link;from=JM Gravel,to=JM Underdrain,type=aggregate2aggregate_H_Link,length="
       << n(width / 2.0) << "[m],name=JM Gravel - Underdrain,width=" << n(length) << "[m]\n";
    ts << "create link;from=JM Bottom Native Soil (" << nativeBelowNz
       << "),to=JM Groundwater,type=soil_to_fixedhead_link,"
          "name=JM Bottom Native Soil - Groundwater\n";
    ts << "create link;from=JM Underdrain,to=JM Outlet,type=Sewer_pipe,ManningCoeff=0.011,diameter="
       << n(underdrainDiameter) << "[m],end_elevation=0,length=" << n(length)
       << "[m],name=JM Underdrain - Outlet,start_elevation=0.05[m]\n";
    ts << "create link;from=JM Catchment,to=JM Outlet,type=Sewer_pipe,ManningCoeff=0.011,diameter=0.15[m],"
          "end_elevation=0,length=1[m],name=JM Surface Overflow,start_elevation="
       << n(pondingDepth) << "[m]\n";

    return out;
}

} // namespace

namespace JMBioretentionBuilder
{

QString FullReferenceScript()
{
    StarterScriptOptions defaults;
    return BuildReference(defaults);
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
            *errorMessage = QStringLiteral("Internal error: output script buffer is null.");
        }
        return false;
    }

    const QString mode = options.jmBuildMode.trimmed();
    if (mode.isEmpty()
        || mode.compare(QStringLiteral("SoftReference"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("FullReference"), Qt::CaseInsensitive) == 0) {
        *scriptText = BuildReference(options);
        return true;
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral("JM_Bioretention builder does not handle mode: %1").arg(mode);
    }
    return false;
}

} // namespace JMBioretentionBuilder
