#ifndef RTSPWIDGET_H
#define RTSPWIDGET_H

#include <memory>

#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QTimer>
#include <QFutureWatcher>

#include "base/BaseWidget.h"
#include "utils/AppEnums.h"

#include "yolo/DetectionTypes.h"
#include "yolo/FramePayload.h"
#include "yolo/CaptureManager.h"          // CaptureResult / SnapshotPersistenceResult
#include "yolo/GeminiAnalysisService.h"   // VehicleAnalysisResult para el watcher

class VideoWidget;
class YoloProcessController;

/**
 * @file RtspWidget.h
 * @brief Pestaña RTSP con YOLO en vivo embebido (QProcess local), igual que la
 *        app qt_app original.
 *
 * Muestra el video con bounding boxes verdes + confianza y permite hacer clic
 * sobre un vehiculo para capturarlo.
 *
 * IMPORTANTE: la persistencia esta configurada como SOLO LOCAL
 * (FileSystemSnapshotPersistenceBackend). NO sube imagenes al VPS ni escribe en
 * la base de datos remota. Para activar el upload remoto, ver el comentario en
 * el constructor de CaptureManager dentro de setupUI().
 */
class RtspWidget : public BaseWidget {
    Q_OBJECT

public:
    explicit RtspWidget(QWidget* parent = nullptr);
    ~RtspWidget() override;

    QString widgetName() const override { return "RtspWidget"; }
    void    reset() override;

signals:
    void statusMessage(const QString& msg);
    void errorMessage(const QString& msg);

protected:
    void setupUI()        override;
    void connectSignals() override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onFrameReady(const FramePayload& payload);
    void onDetectionClicked(int detectionIndex, QPoint imagePoint);
    void onYoloRunningChanged(bool running, const QString& statusMessage);
    void onYoloFinished(int exitCode);
    void onYoloLog(const QString& message);
    void onPersistFinished();
    void onRunOpFinished();
    void onAnalysisFinished();
    void updateConnectionTime();

private:
    void setStatus(Carlens::StreamStatus status);
    QString statusToString(Carlens::StreamStatus status) const;
    QString statusToColor(Carlens::StreamStatus status) const;

    // ── UI ─────────────────────────────────────────────────────────────
    QLineEdit*   m_rtspUrlEdit      = nullptr;
    QPushButton* m_connectBtn       = nullptr;
    QPushButton* m_disconnectBtn    = nullptr;
    QLabel*      m_statusDot        = nullptr;
    QLabel*      m_statusLabel      = nullptr;
    QLabel*      m_sessionInfoLabel = nullptr;
    QLabel*      m_timerLabel       = nullptr;
    QTimer*      m_connectionTimer  = nullptr;
    VideoWidget* m_videoWidget      = nullptr;

    // ── Pipeline YOLO + captura (local + remoto) ───────────────────────
    YoloProcessController*          m_yolo = nullptr;
    std::unique_ptr<CaptureManager> m_captureManager;

    // Operaciones pesadas (tunel SSH) corren en hilo de fondo para no congelar.
    QFutureWatcher<CaptureResult>*               m_persistWatcher  = nullptr;
    QFutureWatcher<SnapshotPersistenceResult>*   m_runOpWatcher    = nullptr;
    QFutureWatcher<VehicleAnalysisResult>*       m_analysisWatcher = nullptr;
    GeminiAnalysisService                        m_geminiService;
    bool    m_capturePersistInProgress = false;
    bool    m_analysisInProgress       = false;
    QString m_pendingRunOpLabel; // "begin" | "finish", para el mensaje correcto

    // Estado del ultimo frame recibido (para la captura por clic).
    QImage        m_currentFrame;
    DetectionList m_currentDetections;
    int           m_currentFrameId  = 0;
    double        m_currentTimestamp = 0.0;
    bool          m_runActive = false;

    int m_elapsedSeconds = 0;
    Carlens::StreamStatus m_currentStatus = Carlens::StreamStatus::Disconnected;
};

#endif // RTSPWIDGET_H
