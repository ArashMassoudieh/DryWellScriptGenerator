#include "ohqdiscovery.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QtGlobal>

namespace {

void AppendUniquePath(QStringList *paths, const QString &path)
{
    if (paths == nullptr || path.trimmed().isEmpty()) {
        return;
    }
    const QString normalized = QFileInfo(path).absoluteFilePath();
    if (!paths->contains(normalized)) {
        paths->push_back(normalized);
    }
}

QString FirstExistingDirectory(const QStringList &candidates)
{
    for (const QString &path : candidates) {
        const QFileInfo info(path);
        if (!path.trimmed().isEmpty() && info.exists() && info.isDir()) {
            return info.absoluteFilePath();
        }
    }
    return QString();
}

} // namespace

namespace OhqDiscovery {

QString FindProjectRoot()
{
    const QStringList startingDirectories = {
        QDir::currentPath(),
        QCoreApplication::applicationDirPath()
    };
    for (const QString &startingDirectory : startingDirectories) {
        QDir dir(startingDirectory);
        for (int i = 0; i < 8; ++i) {
            if (QFileInfo::exists(dir.filePath("model_creator_runner.pro"))
                || QFileInfo::exists(dir.filePath("DryWellScriptGenerator.pro"))) {
                return dir.absolutePath();
            }
            if (!dir.cdUp()) {
                break;
            }
        }
    }
    return QDir::currentPath();
}

QStringList CandidateRoots(const QString &projectRoot, const QStringList &hintRoots)
{
    QStringList roots;
    AppendUniquePath(&roots, qEnvironmentVariable("OHQ_ROOT"));
    AppendUniquePath(&roots, qEnvironmentVariable("OPENHYDROQUAL_ROOT"));
    for (const QString &hint : hintRoots) {
        const QFileInfo info(hint);
        if (info.exists()) {
            AppendUniquePath(&roots, info.isDir() ? info.absoluteFilePath() : info.absolutePath());
        }
    }

    const QDir projectDir(projectRoot);
    AppendUniquePath(&roots, projectDir.filePath("OpenHydroQual"));
    AppendUniquePath(&roots, projectDir.filePath("../OpenHydroQual"));
    AppendUniquePath(&roots, projectDir.filePath("../../OpenHydroQual"));
    return roots;
}

QString DetectTemplateDirectory(const QStringList &rootCandidates,
                                const QString &workingDirectory)
{
    QStringList candidates = {qEnvironmentVariable("OHQ_TEMPLATE_DIR")};
    for (const QString &rootPath : rootCandidates) {
        const QDir root(rootPath);
        if (!root.exists()) {
            continue;
        }
        candidates << root.filePath("resources")
                   << root.filePath("templates")
                   << root.filePath("template_resources")
                   << root.filePath("aquifolium/examples/templates")
                   << root.filePath("aquifolium/templates");
    }

    if (!workingDirectory.trimmed().isEmpty()) {
        const QDir workingDir(workingDirectory);
        candidates << workingDir.filePath("templates")
                   << workingDir.filePath("template_resources");
    }
    return FirstExistingDirectory(candidates);
}

} // namespace OhqDiscovery
