#ifndef HQ_DRYWELL_BUILDER_H
#define HQ_DRYWELL_BUILDER_H

#include "starter_script_builder.h"

#include <QString>

namespace HqDrywellBuilder
{

QString FullReferenceScript();
QString InflowTargetObject();

bool AppendBaseInflowBlock(const StarterScriptOptions &options,
                           const QString &inflow,
                           QString *scriptText,
                           QString *errorMessage = nullptr);

} // namespace HqDrywellBuilder

#endif // HQ_DRYWELL_BUILDER_H
