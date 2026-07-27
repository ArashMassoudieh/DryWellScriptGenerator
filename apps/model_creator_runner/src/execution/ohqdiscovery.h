#ifndef OHQDISCOVERY_H
#define OHQDISCOVERY_H

#include <QString>
#include <QStringList>

namespace OhqDiscovery {

QString FindProjectRoot();
QStringList CandidateRoots(const QString &projectRoot, const QStringList &hintRoots = {});
QString DetectTemplateDirectory(const QStringList &rootCandidates,
                                const QString &workingDirectory);

} // namespace OhqDiscovery

#endif // OHQDISCOVERY_H
