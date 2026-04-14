#ifndef R_BIOSWALE_BUILDER_H
#define R_BIOSWALE_BUILDER_H

#include "starter_script_builder.h"

#include <QString>

namespace RBioswaleBuilder
{

QString FullReferenceScript();
QString InflowTargetObject();

bool AppendBaseInflowBlock(const StarterScriptOptions &options,
                           const QString &inflow,
                           QString *scriptText,
                           QString *errorMessage = nullptr);

} // namespace RBioswaleBuilder

#endif // R_BIOSWALE_BUILDER_H
