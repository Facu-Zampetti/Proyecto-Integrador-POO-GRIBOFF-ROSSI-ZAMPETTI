#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

#include "FramePayload.h"

class YoloProcessController : public QObject {
    Q_OBJECT

public:
    explicit YoloProcessController(QObject *parent = nullptr);
    ~YoloProcessController() override;

    bool isRunning() const;
    bool start(const QString &source, bool showWindow, QString *errorMessage);
    void stop();
    QString projectRoot() const;

signals:
    void logMessage(const QString &message);
    void frameReady(const FramePayload &payload);
    void runningChanged(bool running, const QString &statusMessage);
    void processFinished(int exitCode);

private slots:
    void appendStdOut();
    void appendStdErr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QString findProjectRoot() const;
    QString resolvePythonExe(const QString &projectRoot) const;
    QString resolveScriptPath(const QString &projectRoot) const;
    void handleStdOutLine(const QByteArray &line);

    QProcess *process_;
    QByteArray stdOutBuffer_;
    QString projectRoot_;
};