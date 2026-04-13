#include "r_bioswale_builder.h"

bool RBioswaleBuilder::AppendBaseInflowBlock(const StarterScriptOptions &,
                                             const QString &inflow,
                                             QString *scriptText,
                                             QString *errorMessage)
{
    if (scriptText == nullptr) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Internal error: output script buffer is null.");
        }
        return false;
    }

    *scriptText += QStringLiteral(
        "create block;type=Catchment,_width=200,_height=200,name=Catchment (1),"
        "loss_coefficient=0[1/day],x=0,Evapotranspiration=,Precipitation=,ManningCoeff=0.01,"
        "inflow=%1,Slope=0.02,Width=1[m],y=-200,area=1[m~^2],"
        "depression_storage=0[m],depth=0[m],elevation=0[m]\n")
                      .arg(inflow);
    return true;
}
