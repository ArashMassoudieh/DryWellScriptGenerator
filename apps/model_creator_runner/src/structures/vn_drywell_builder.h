#ifndef VN_DRYWELL_BUILDER_H
#define VN_DRYWELL_BUILDER_H

#include "starter_script_builder.h"

#include <QString>

namespace VnDrywellBuilder
{
    QString VnFullReferenceScript();
    QString InflowTargetObject();

    bool Build(const StarterScriptOptions &options,
               QString *scriptText,
               QString *errorMessage = nullptr);
}

#endif // VN_DRYWELL_BUILDER_H
