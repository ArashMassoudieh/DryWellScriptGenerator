#ifndef DRYWELL_BUILDER_H
#define DRYWELL_BUILDER_H

#include "starter_script_builder.h"
#include <QString>

namespace DrywellBuilder
{
    bool Build(const StarterScriptOptions &options,
               QString *scriptText,
               QString *errorMessage = nullptr);
}

#endif
