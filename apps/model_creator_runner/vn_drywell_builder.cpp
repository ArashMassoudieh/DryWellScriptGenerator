#include "vn_drywell_builder.h"

namespace {

static const char *kVnFullReferenceBanner =
R"VN(# VN_Drywell full-reference scaffold generated from VN builder file
)VN";

}

bool VnDrywellBuilder::Build(const StarterScriptOptions &options,
                             QString *scriptText,
                             QString *errorMessage)
{
    if (scriptText == nullptr) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Internal error: output script buffer is null.");
        }
        return false;
    }

    const QString mode = options.vnBuildMode.trimmed();
    if (mode.compare(QStringLiteral("FullReference"), Qt::CaseInsensitive) != 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("VN builder currently handles FullReference mode only.");
        }
        return false;
    }

    scriptText->append(QString::fromUtf8(kVnFullReferenceBanner));
    return true;
}
