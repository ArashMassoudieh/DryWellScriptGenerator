#ifndef JM_BIORETENTION_BUILDER_H
#define JM_BIORETENTION_BUILDER_H

#include "starter_script_builder.h"

#include <QString>

namespace JMBioretentionBuilder
{

QString FullReferenceScript();
QString InflowTargetObject();
QString RainfallTargetObject();
QString ContributingCatchmentObject();

bool Build(const StarterScriptOptions &options,
           QString *scriptText,
           QString *errorMessage = nullptr);

} // namespace JMBioretentionBuilder

#endif // JM_BIORETENTION_BUILDER_H
