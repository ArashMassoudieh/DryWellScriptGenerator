#ifndef HQ_DRYWELL_BUILDER_H
#define HQ_DRYWELL_BUILDER_H

#include "starter_script_builder.h"

#include <QString>

namespace HqDrywellBuilder
{

struct SoilBlockSpec
{
    QString name;
    double thetaSat = 0.4;
    double thetaRes = 0.05;
    double n = 1.41;
    double kSatOriginal = 1.0;
    double alpha = 1.0;
    double area = 1.0;
    double x = 0.0;
    double y = 0.0;
    double bottomElevation = 0.0;
    double depth = 0.1;
    double actualX = 0.0;
    double actualY = 0.0;
};

QString FullReferenceScript();
QString InflowTargetObject();
QString BuildSoilBlockCommand(const SoilBlockSpec &spec);

bool AppendBaseInflowBlock(const StarterScriptOptions &options,
                           const QString &inflow,
                           QString *scriptText,
                           QString *errorMessage = nullptr);


bool Build(const StarterScriptOptions &options,
           QString *scriptText,
           QString *errorMessage = nullptr);

} // namespace HqDrywellBuilder

#endif // HQ_DRYWELL_BUILDER_H
