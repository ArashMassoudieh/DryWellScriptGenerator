#ifndef OHQPROCESSRUNNER_H
#define OHQPROCESSRUNNER_H

#include <QObject>
#include <QString>
#include <QStringList>

class QProcess;

class OHQProcessRunner : public QObject
{
    Q_OBJECT

public:
    /// Creates a runner wrapper around QProcess for OHQ script execution.
    explicit OHQProcessRunner(QObject *parent = nullptr);
    ~OHQProcessRunner() override;

    /// Sets absolute path to OHQ executable used by runScript().
    void setExecutablePath(const QString &path);
    /// Returns currently configured OHQ executable path.
    QString executablePath() const;

    /// Returns true while a process is active.
    bool isRunning() const;
    /// Requests stop/terminate for the active run process.
    void stop();

    /**
     * @brief Start OHQ with the given script file in a working directory.
     * @param scriptFile Script path passed to OHQ.
     * @param workingDirectory Process working directory.
     * @param executableArgs Full argument list passed to executable.
     */
    void runScript(const QString &scriptFile,
                   const QString &workingDirectory,
                   const QStringList &executableArgs);

signals:
    /// Emitted once a run process has started.
    void runStarted();
    /// Emitted for stdout/stderr text chunks.
    void outputReady(const QString &text);
    /// Emitted when process exits normally or after stop.
    void runFinished(int exitCode);
    /// Emitted when process start/run fails.
    void runFailed(const QString &reason);

private:
    QProcess *process;
    QString executable;
    bool stopRequested = false;
};

#endif // OHQPROCESSRUNNER_H
