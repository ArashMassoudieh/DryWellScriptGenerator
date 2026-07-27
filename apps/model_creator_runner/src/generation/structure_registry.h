#ifndef STRUCTURE_REGISTRY_H
#define STRUCTURE_REGISTRY_H

#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

namespace StructureRegistry
{

QStringList ModelTypes();

bool IsKnownModelType(const QString &modelType);
bool IsKnownPreset(const QString &preset);
bool IsPresetCompatibleWithModel(const QString &preset, const QString &modelType);

QList<QPair<QString, QString>> PresetOptionsForModel(const QString &modelType);

} // namespace StructureRegistry

#endif // STRUCTURE_REGISTRY_H
