#pragma once

#include <QMainWindow>
#include <QProcess>

#include "VideoWidget.h"

class QLineEdit;
class QTextEdit;
class QPushButton;
class QCheckBox;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void startYolo();
    void stopYolo();
    void appendStdOut();
    void appendStdErr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onDetectionClicked(int detectionIndex, QPoint imagePoint);

private:
    void setupUi();
    QString findProjectRoot() const;
    QString resolvePythonExe(const QString &projectRoot) const;
    QString resolveScriptPath(const QString &projectRoot) const;
    void handleStdOutLine(const QByteArray &line);
    void processFramePayload(const QJsonObject &payload);
    bool saveDetectionCapture(const DetectionBox &detection, const QPoint &imagePoint);

    QLineEdit *sourceEdit_;
    QCheckBox *showWindowCheck_;
    QPushButton *startBtn_;
    QPushButton *stopBtn_;
    VideoWidget *videoWidget_;
    QTextEdit *logView_;
    QLabel *statusLabel_;
    QProcess *process_;
    QByteArray stdOutBuffer_;
    QString projectRoot_;
    QImage currentFrame_;
    DetectionList currentDetections_;
    int currentFrameId_ = 0;
    double currentTimestamp_ = 0.0;
};
