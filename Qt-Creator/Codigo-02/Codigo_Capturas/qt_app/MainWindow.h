#pragma once

#include <memory>

#include <QImage>
#include <QMainWindow>

#include "CaptureManager.h"
#include "VideoWidget.h"

class QLineEdit;
class QTextEdit;
class QPushButton;
class QCheckBox;
class QLabel;
class YoloProcessController;
struct FramePayload;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void startYolo();
    void stopYolo();
    void appendLogMessage(const QString &message);
    void onFrameReady(const FramePayload &payload);
    void onRunningChanged(bool running, const QString &statusMessage);
    void onProcessFinished(int exitCode);
    void onDetectionClicked(int detectionIndex, QPoint imagePoint);

private:
    void setupUi();

    QLineEdit *sourceEdit_;
    QCheckBox *showWindowCheck_;
    QPushButton *startBtn_;
    QPushButton *stopBtn_;
    VideoWidget *videoWidget_;
    QTextEdit *logView_;
    QLabel *statusLabel_;
    YoloProcessController *processController_;
    std::unique_ptr<CaptureManager> captureManager_;
    QImage currentFrame_;
    DetectionList currentDetections_;
    int currentFrameId_ = 0;
    double currentTimestamp_ = 0.0;
};
