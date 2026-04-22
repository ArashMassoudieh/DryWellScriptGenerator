#ifndef R_BIOSWALE_BUILDER_H
#define R_BIOSWALE_BUILDER_H

#include "starter_script_builder.h"

#include <QString>

namespace RBioswaleBuilder
{

struct SoilBlockSpec
{
    QString name;
    double thetaSat = 0.43;
    double thetaRes = 0.078;
    double n = 1.56;
    double kSatOriginal = 0.25;
    double alpha = 3.6;
    double area = 4.0;
    double x = 0.0;
    double y = 0.0;
    double bottomElevation = 0.0;
    double depth = 0.1016;
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

} // namespace RBioswaleBuilder

#endif // R_BIOSWALE_BUILDER_H
