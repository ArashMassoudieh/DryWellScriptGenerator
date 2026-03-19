#ifndef OHQPROCESSRUNNER_H
#define OHQPROCESSRUNNER_H

#include <QObject>
#include <QString>

class QProcess;

class OHQProcessRunner : public QObject
{
    Q_OBJECT

public:
    explicit OHQProcessRunner(QObject *parent = nullptr);
    ~OHQProcessRunner() override;

    void setExecutablePath(const QString &path);
    QString executablePath() const;

    bool isRunning() const;
    void stop();

    void runScript(const QString &scriptFile, const QString &workingDirectory);

signals:
    void runStarted();
    void outputReady(const QString &text);
    void runFinished(int exitCode);
    void runFailed(const QString &reason);

private:
    QProcess *process;
    QString executable;
    bool stopRequested = false;
};

#endif // OHQPROCESSRUNNER_H
