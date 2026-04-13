#include "hq_drywell_builder.h"

bool HqDrywellBuilder::AppendBaseInflowBlock(const StarterScriptOptions &,
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
        "create block;type=Pond,inflow=%1,_width=200,Evapotranspiration=,Precipitation=,"
        "bottom_elevation=0[m],Storage=0[m~^3],name=Infiltration_Pond,alpha=86.061,beta=2.766,x=0,y=0,_height=200\n")
                      .arg(inflow);
    return true;
}
