#ifndef BIOSWALE_BUILDER_H
#define BIOSWALE_BUILDER_H

#include "starter_script_builder.h"
#include <QString>

// Isolated Bioswale builder.
// This does NOT modify Drywell or VN_Drywell logic.
// Supported modes:
//   - FullReference
//   - SoftReference
//   - LoadFromOhq
//   - Preset

namespace BioswaleBuilder
{
    bool Build(const StarterScriptOptions &options,
               QString *scriptText,
               QString *errorMessage = nullptr);
}

#endif // BIOSWALE_BUILDER_H
