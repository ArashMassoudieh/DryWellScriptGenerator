#include "jm_bioretention_builder.h"

#include <QTextStream>
#include <QtGlobal>

#include <cmath>

namespace {

QString n(double value)
{
    return QString::number(value, 'g', 12);
}

QString BuildReference(const StarterScriptOptions &options)
{
    // John McCormack Road CC-101 bioretention reference geometry.
    // Keep this independent from the R_Bioswale cross-section: JM is a
    // longitudinal, layered system with sloped media elevations, choker/gravel
    // storage, sump, surrounding native soil, underdrain, a single surface
    // catchment, and a partial-height outlet.
    // All dimensions and elevations written to OHQ are SI (m).
    constexpr int nx = 4;
    const double length = options.jmLength > 0.0 ? options.jmLength : 12.192;       // 40 ft
    const double width = options.jmWidth > 0.0 ? options.jmWidth : 3.7084;          // 12 ft 2 in
    const double dx = length / nx;
    const double cellArea = width * dx;

    const double mulch = options.jmMulchDepth > 0.0 ? options.jmMulchDepth : 0.0762;
    const double media = options.jmMediaDepth > 0.0 ? options.jmMediaDepth : 0.9144;
    const double choker = options.jmChokerDepth > 0.0 ? options.jmChokerDepth : 0.0762;
    const double gravel = options.jmGravelDepth > 0.0 ? options.jmGravelDepth : 0.6096;
    const double sump = options.jmSumpDepth > 0.0 ? options.jmSumpDepth : 0.3048;
    const double underdrainDiameter = options.jmUnderdrainDiameter > 0.0
        ? options.jmUnderdrainDiameter : 0.1016;
    const double surroundingSoilDepth = media;
    const double verticalSoilDepth = media;
    // Native soil is represented on both sides of the facility.  Use a
    // half-bioswale-width strip on each side instead of accidentally assigning
    // every surrounding block the full bioswale plan area.
    const double surroundingWidth = width / 2.0;
    const double sideCellArea = surroundingWidth * dx;

    // Local datum: partial-height outlet crest = 0 m.
    const double surfaceZ[nx] = {0.835152, 0.70104, 0.512064, 0.280416};

    QString out;
    QTextStream ts(&out);
    ts.setRealNumberPrecision(12);

    ts << "# JM_Bioretention: John McCormack Road CC-101\n";
    ts << "# Units: SI; all lengths/elevations are metres.\n";
    ts << "# Local vertical datum: partial-height outlet crest = 0.0 m.\n";
    ts << "# JM grid: nx=" << nx << ", nz=5 material layers, primary_cells=" << (nx * 5) << "\n";
    ts << "# JM blocks: soil=" << (nx * 7) << ", aggregate_storage=" << (nx * 3)
       << ", surface=1, surrounding_soil=" << (nx * 5) << "\n";
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
    ts << "create block;type=Catchment,Evapotranspiration=,ManningCoeff=0.03,Precipitation=Rain,Runoff_coeff=0.8,"
          "Slope=0.02,Width=30,_height=300,_width=500,area=" << n(options.jmCatchmentArea > 0.0 ? options.jmCatchmentArea : 1000.0)
       << "[m~^2],depression_storage=0,depth=0,elevation=0,inflow=,loss_coefficient=0,name=JM Contributing Catchment,x=-800,y=-300\n";
    ts << "create block;type=Catchment,Evapotranspiration=,ManningCoeff=0.01,Precipitation=,Runoff_coeff=1,"
          "Slope=0.01,Width=" << n(width) << ",_height=140,_width=190,area=" << n(width * length)
       << "[m~^2],depression_storage=0,depth=0,elevation=" << n(surfaceZ[0])
       << ",inflow=,loss_coefficient=0,name=JM Catchment,x=0,y=0\n";

    for (int i = 0; i < nx; ++i) {
        const int k = i + 1;
        const double x = i * 260.0;
        const double zSurface = surfaceZ[i];
        const double zMediaBottom = zSurface - mulch - media;
        const double zChokerBottom = zMediaBottom - choker;
        const double zGravelBottom = zChokerBottom - gravel;
        const double zSumpBottom = zGravelBottom - sump;

        ts << "create block;type=Soil,Evapotranspiration=,K_sat_original=50,K_sat_scale_factor=JM_KS_scale_factor,"
              "MC_to_EC_Threshold_Moisture=0,MC_to_EC_coefficient=0,MC_to_EC_exponent=0,_height=120,_width=180,"
              "act_X=" << n((i + 0.5) * dx) << ",act_Y=" << n(zSurface - mulch - media / 2.0)
           << ",alpha=JM_Eng_Soil_alpha,aniso_ratio=1,area=" << n(cellArea)
           << ",bottom_elevation=" << n(zMediaBottom) << ",depth=" << n(media)
           << ",n=JM_Eng_Soil_n,name=JM Media (" << k << "),specific_storage=0.01,theta=0.12,theta_res=0.05,theta_sat=0.45,x="
           << n(x) << ",y=220\n";

        ts << "create block;type=Aggregate_storage_layer,K_sat=500,_height=100,_width=180,area=" << n(cellArea)
           << ",bottom_elevation=" << n(zChokerBottom) << ",depth=" << n(choker)
           << ",inflow=,name=JM Choker (" << k << "),porosity=0.40,x=" << n(x) << ",y=410\n";

        ts << "create block;type=Aggregate_storage_layer,K_sat=5000,_height=120,_width=180,area=" << n(cellArea)
           << ",bottom_elevation=" << n(zGravelBottom) << ",depth=" << n(gravel)
           << ",inflow=,name=JM Gravel (" << k << "),porosity=0.40,x=" << n(x) << ",y=590\n";

        ts << "create block;type=Soil,Evapotranspiration=,K_sat_original=0.25,K_sat_scale_factor=1,"
              "MC_to_EC_Threshold_Moisture=0,MC_to_EC_coefficient=0,MC_to_EC_exponent=0,_height=110,_width=180,"
              "act_X=" << n((i + 0.5) * dx) << ",act_Y=" << n(zGravelBottom - sump / 2.0)
           << ",alpha=3.6,aniso_ratio=1,area=" << n(cellArea)
           << ",bottom_elevation=" << n(zSumpBottom) << ",depth=" << n(sump)
           << ",n=1.56,name=JM Infiltration Sump (" << k << "),specific_storage=0.01,theta=0.12,theta_res=0.078,theta_sat=0.43,x="
           << n(x) << ",y=790\n";

        const auto writeNativeSoilBlock = [&](const QString &name,
                                             double bottomElevation,
                                             double depth,
                                             double actualX,
                                             double actualY,
                                             double uiX,
                                             double uiY,
                                             double planArea) {
            ts << "create block;type=Soil,Evapotranspiration=,K_sat_original=0.25,K_sat_scale_factor=1,"
                  "MC_to_EC_Threshold_Moisture=0,MC_to_EC_coefficient=0,MC_to_EC_exponent=0,_height=110,_width=180,"
                  "act_X=" << n(actualX) << ",act_Y=" << n(actualY)
               << ",alpha=3.6,aniso_ratio=1,area=" << n(planArea)
               << ",bottom_elevation=" << n(bottomElevation) << ",depth=" << n(depth)
               << ",n=1.56,name=" << name
               << ",specific_storage=0.01,theta=0.12,theta_res=0.078,theta_sat=0.43,x="
               << n(uiX) << ",y=" << n(uiY) << "\n";
        };
        const double centerX = (i + 0.5) * dx;
        const double sideOffset = width / 2.0 + surroundingWidth / 2.0;
        const double sideActualY = zSurface - mulch - surroundingSoilDepth / 2.0;
        writeNativeSoilBlock(QStringLiteral("JM Left Native Soil (%1)").arg(k),
                             zSurface - mulch - surroundingSoilDepth,
                             surroundingSoilDepth,
                             centerX - sideOffset,
                             sideActualY,
                             i * 300.0,
                             970.0,
                             sideCellArea);
        writeNativeSoilBlock(QStringLiteral("JM Right Native Soil (%1)").arg(k),
                             zSurface - mulch - surroundingSoilDepth,
                             surroundingSoilDepth,
                             centerX + sideOffset,
                             sideActualY,
                             i * 300.0,
                             1110.0,
                             sideCellArea);
        const double bottomActualY = zSumpBottom - verticalSoilDepth / 2.0;
        writeNativeSoilBlock(QStringLiteral("JM Bottom Native Soil (%1)").arg(k),
                             zSumpBottom - verticalSoilDepth,
                             verticalSoilDepth,
                             centerX,
                             bottomActualY,
                             i * 300.0,
                             1250.0,
                             cellArea);
        writeNativeSoilBlock(QStringLiteral("JM Left Bottom Native Soil (%1)").arg(k),
                             zSumpBottom - verticalSoilDepth,
                             verticalSoilDepth,
                             centerX - sideOffset,
                             bottomActualY,
                             i * 300.0,
                             1390.0,
                             sideCellArea);
        writeNativeSoilBlock(QStringLiteral("JM Right Bottom Native Soil (%1)").arg(k),
                             zSumpBottom - verticalSoilDepth,
                             verticalSoilDepth,
                             centerX + sideOffset,
                             bottomActualY,
                             i * 300.0,
                             1530.0,
                             sideCellArea);
    }

    const double lowestSurface = surfaceZ[nx - 1];
    const double lowestMediaBottom = lowestSurface - mulch - media;
    const double lowestChokerBottom = lowestMediaBottom - choker;
    const double lowestGravelBottom = lowestChokerBottom - gravel;
    const double underdrainInvert = lowestGravelBottom + underdrainDiameter / 2.0;
    const double groundwaterHead = lowestGravelBottom - sump - verticalSoilDepth - 0.5;

    // Represent the perforated underdrain as a small aggregate-storage conduit.
    // This permits valid aggregate-to-aggregate drainage links from the gravel
    // cells.  The conduit then discharges through a Sewer_pipe link.
    ts << "create block;type=Aggregate_storage_layer,K_sat=10000,_height=120,_width=220,area="
       << n(underdrainDiameter * length) << ",bottom_elevation=" << n(underdrainInvert)
       << ",depth=" << n(underdrainDiameter)
       << ",inflow=,name=JM Underdrain,porosity=1,x=1040,y=600\n";
    ts << "create block;type=fixed_head,name=JM Underdrain Outlet,_width=180,_height=120,x=1300,y=600,head="
       << n(underdrainInvert) << "[m],Storage=100000[m~^3]\n";
    ts << "create block;type=fixed_head,name=JM Surface Outlet,_width=180,_height=120,x=1300,y=120,head=0[m],Storage=100000[m~^3]\n";
    ts << "create block;type=fixed_head,name=JM Groundwater,_width=180,_height=120,x=450,y=1700,head="
       << n(groundwaterHead) << "[m],Storage=100000[m~^3]\n";

    // Use one surface catchment for runoff storage, comparable to R_Bioswale's
    // catchment-to-first-soil routing pattern, while preserving JM's separate
    // layered longitudinal media/choker/gravel/sump geometry.
    ts << "create link;from=JM Contributing Catchment,to=JM Catchment,"
          "type=Catchment_link,name=JM Contributing Catchment - JM Catchment\n";
    for (int i = 1; i <= nx; ++i) {
        ts << "create link;from=JM Catchment,to=JM Media (" << i
           << "),type=surfacewater_to_soil_link,name=JM Catchment - Media " << i << "\n";
        // aggregate_to_soil_link is the available soil/aggregate interface
        // link in the OHQ templates.  Put the aggregate block first; the link
        // remains hydraulically bidirectional.
        ts << "create link;from=JM Choker (" << i << "),to=JM Media (" << i
           << "),type=aggregate_to_soil_link,name=JM Choker - Media " << i << "\n";
        ts << "create link;from=JM Choker (" << i << "),to=JM Gravel (" << i
           << "),type=aggregate2aggregate_H_Link,length=" << n(choker)
           << ",name=JM Choker - Gravel " << i << ",width=" << n(width) << "\n";
        ts << "create link;from=JM Gravel (" << i << "),to=JM Infiltration Sump (" << i
           << "),type=aggregate_to_soil_link,name=JM Gravel - Sump " << i << "\n";
        ts << "create link;from=JM Media (" << i
           << "),to=JM Left Native Soil (" << i
           << "),type=soil_to_soil_H_link,name=JM Media - Left Native Soil " << i
           << ",length=" << n(width / 2.0) << "[m],area=" << n(media * dx) << "[m~^2]\n";
        ts << "create link;from=JM Media (" << i
           << "),to=JM Right Native Soil (" << i
           << "),type=soil_to_soil_H_link,name=JM Media - Right Native Soil " << i
           << ",length=" << n(width / 2.0) << "[m],area=" << n(media * dx) << "[m~^2]\n";
        ts << "create link;from=JM Infiltration Sump (" << i
           << "),to=JM Bottom Native Soil (" << i
           << "),type=soil_to_soil_link,name=JM Sump - Bottom Native Soil " << i << "\n";
        ts << "create link;from=JM Left Native Soil (" << i
           << "),to=JM Left Bottom Native Soil (" << i
           << "),type=soil_to_soil_link,name=JM Left Native Soil - Left Bottom Native Soil " << i << "\n";
        ts << "create link;from=JM Right Native Soil (" << i
           << "),to=JM Right Bottom Native Soil (" << i
           << "),type=soil_to_soil_link,name=JM Right Native Soil - Right Bottom Native Soil " << i << "\n";
        ts << "create link;from=JM Bottom Native Soil (" << i
           << "),to=JM Left Bottom Native Soil (" << i
           << "),type=soil_to_soil_H_link,name=JM Bottom Native Soil - Left Bottom Native Soil " << i
           << ",length=" << n(width / 2.0) << "[m],area=" << n(verticalSoilDepth * dx) << "[m~^2]\n";
        ts << "create link;from=JM Bottom Native Soil (" << i
           << "),to=JM Right Bottom Native Soil (" << i
           << "),type=soil_to_soil_H_link,name=JM Bottom Native Soil - Right Bottom Native Soil " << i
           << ",length=" << n(width / 2.0) << "[m],area=" << n(verticalSoilDepth * dx) << "[m~^2]\n";
        ts << "create link;from=JM Bottom Native Soil (" << i
           << "),to=JM Groundwater,type=soil_to_fixedhead_link,name=JM Bottom Native Soil - GW " << i << "\n";
        ts << "create link;from=JM Left Bottom Native Soil (" << i
           << "),to=JM Groundwater,type=soil_to_fixedhead_link,name=JM Left Bottom Native Soil - GW " << i << "\n";
        ts << "create link;from=JM Right Bottom Native Soil (" << i
           << "),to=JM Groundwater,type=soil_to_fixedhead_link,name=JM Right Bottom Native Soil - GW " << i << "\n";
    }

    for (int i = 1; i < nx; ++i) {
        ts << "create link;from=JM Media (" << i << "),to=JM Media (" << i + 1
           << "),type=soil_to_soil_H_link,name=JM Media Horizontal " << i << "\n";
        ts << "create link;from=JM Choker (" << i << "),to=JM Choker (" << i + 1
           << "),type=aggregate2aggregate_H_Link,length=" << n(dx)
           << ",name=JM Choker Horizontal " << i << ",width=" << n(width) << "\n";
        ts << "create link;from=JM Gravel (" << i << "),to=JM Gravel (" << i + 1
           << "),type=aggregate2aggregate_H_Link,length=" << n(dx)
           << ",name=JM Gravel Horizontal " << i << ",width=" << n(width) << "\n";
        ts << "create link;from=JM Infiltration Sump (" << i << "),to=JM Infiltration Sump (" << i + 1
           << "),type=soil_to_soil_H_link,name=JM Sump Horizontal " << i << "\n";
        ts << "create link;from=JM Left Native Soil (" << i << "),to=JM Left Native Soil (" << i + 1
           << "),type=soil_to_soil_H_link,name=JM Left Native Soil Horizontal " << i
           << ",length=" << n(dx) << "[m],area=" << n(surroundingSoilDepth * surroundingWidth) << "[m~^2]\n";
        ts << "create link;from=JM Right Native Soil (" << i << "),to=JM Right Native Soil (" << i + 1
           << "),type=soil_to_soil_H_link,name=JM Right Native Soil Horizontal " << i
           << ",length=" << n(dx) << "[m],area=" << n(surroundingSoilDepth * surroundingWidth) << "[m~^2]\n";
        ts << "create link;from=JM Bottom Native Soil (" << i << "),to=JM Bottom Native Soil (" << i + 1
           << "),type=soil_to_soil_H_link,name=JM Bottom Native Soil Horizontal " << i
           << ",length=" << n(dx) << "[m],area=" << n(verticalSoilDepth * surroundingWidth) << "[m~^2]\n";
        ts << "create link;from=JM Left Bottom Native Soil (" << i << "),to=JM Left Bottom Native Soil (" << i + 1
           << "),type=soil_to_soil_H_link,name=JM Left Bottom Native Soil Horizontal " << i
           << ",length=" << n(dx) << "[m],area=" << n(verticalSoilDepth * surroundingWidth) << "[m~^2]\n";
        ts << "create link;from=JM Right Bottom Native Soil (" << i << "),to=JM Right Bottom Native Soil (" << i + 1
           << "),type=soil_to_soil_H_link,name=JM Right Bottom Native Soil Horizontal " << i
           << ",length=" << n(dx) << "[m],area=" << n(verticalSoilDepth * surroundingWidth) << "[m~^2]\n";
    }

    // Underdrain receives drainage from each gravel cell and discharges to a
    // dedicated subsurface outlet.  Do not connect gravel to a Pipe block with
    // aggregate_to_soil_link; that endpoint pairing is invalid.
    for (int i = 1; i <= nx; ++i) {
        ts << "create link;from=JM Gravel (" << i
           << "),to=JM Underdrain,type=aggregate2aggregate_H_Link,length=" << n(dx / 2.0)
           << ",name=JM Gravel - Underdrain " << i << ",width=" << n(underdrainDiameter) << "\n";
    }
    ts << "create link;from=JM Underdrain,to=JM Underdrain Outlet,type=Sewer_pipe,ManningCoeff=0.011,diameter="
       << n(underdrainDiameter) << ",end_elevation=" << n(underdrainInvert)
       << ",length=" << n(length)
       << ",name=JM Underdrain - Outlet,start_elevation=" << n(underdrainInvert) << "\n";

    // Partial-height surface outlet at the local crest datum.
    ts << "create link;from=JM Catchment,to=JM Surface Outlet,type=Sewer_pipe,ManningCoeff=0.011,diameter=0.15,"
          "end_elevation=0,length=1,name=JM Partial Height Outlet,start_elevation=0\n";

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
