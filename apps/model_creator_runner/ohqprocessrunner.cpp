#include "ohqprocessrunner.h"

#include <QFileInfo>
#include <QProcess>

OHQProcessRunner::OHQProcessRunner(QObject *parent)
    : QObject(parent), process(new QProcess(this))
{
    connect(process, &QProcess::started, this, &OHQProcessRunner::runStarted);

    connect(process, &QProcess::readyReadStandardOutput, this, [this]() {
        emit outputReady(QString::fromLocal8Bit(process->readAllStandardOutput()));
    });

    connect(process, &QProcess::readyReadStandardError, this, [this]() {
        emit outputReady(QString::fromLocal8Bit(process->readAllStandardError()));
    });

    connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
        const bool wasStoppedByUser = stopRequested;
        stopRequested = false;

        if (status == QProcess::NormalExit) {
            emit runFinished(exitCode);
        } else {
            if (wasStoppedByUser) {
                emit runFinished(exitCode);
            } else {
                emit runFailed(QStringLiteral("OHQ process crashed."));
            }
        }
    });

    connect(process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (!stopRequested && error != QProcess::Crashed) {
            emit runFailed(process->errorString());
        }
    });
}

OHQProcessRunner::~OHQProcessRunner() = default;

void OHQProcessRunner::setExecutablePath(const QString &path)
{
    executable = path;
}

QString OHQProcessRunner::executablePath() const
{
    return executable;
}

bool OHQProcessRunner::isRunning() const
{
    return process->state() != QProcess::NotRunning;
}

void OHQProcessRunner::stop()
{
    if (!isRunning()) {
        return;
    }

    stopRequested = true;
    process->terminate();
    if (!process->waitForFinished(3000)) {
        process->kill();
    }
}

void OHQProcessRunner::runScript(const QString &scriptFile, const QString &workingDirectory)
{
    if (isRunning()) {
        emit runFailed(QStringLiteral("OHQ process is already running."));
        return;
    }

    stopRequested = false;

    if (executable.isEmpty()) {
        emit runFailed(QStringLiteral("OHQ executable path is empty."));
        return;
    }

    const QFileInfo executableInfo(executable);
    if (!executableInfo.exists() || !executableInfo.isFile() || !executableInfo.isExecutable()) {
        emit runFailed(QStringLiteral("OHQ executable path is not a runnable file: %1").arg(executable));
        return;
    }

    const QFileInfo scriptInfo(scriptFile);
    if (!scriptInfo.exists()) {
        emit runFailed(QStringLiteral("Script file does not exist: %1").arg(scriptFile));
        return;
    }

    process->setWorkingDirectory(workingDirectory);
    process->start(executable, QStringList{scriptFile});
}
