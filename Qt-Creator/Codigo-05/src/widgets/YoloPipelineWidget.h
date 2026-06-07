#ifndef YOLOPIPELINEWIDGET_H
#define YOLOPIPELINEWIDGET_H

#include <memory>

#include <QImage>
#include <QLabel>
#include <QTimer>
#include <QFutureWatcher>

#include "base/BaseWidget.h"
#include "utils/AppEnums.h"

#include "yolo/DetectionTypes.h"
#include "yolo/FramePayload.h"
#include "yolo/CaptureManager.h"
#include "yolo/GeminiAnalysisService.h"

class VideoWidget;
class YoloProcessController;

/**
 * @file YoloPipelineWidget.h
 * @brief Clase base (Template Method) para widgets con pipeline YOLO+captura.
 *
 * Concentra el pipeline duplicado entre RtspWidget y VideoUploadWidget:
 *   YoloProcessController → frames → VideoWidget → CaptureManager → Gemini.
 *
 * Hooks virtuales que cada subclase implementa (Template Method):
 *   - statusToString(): texto del estado en el idioma de cada pantalla.
 *   - updateControls(): habilita/deshabilita los botones propios del widget.
 *   - onPipelineStarted() / onPipelineStopped(): acciones al arrancar/parar.
 *
 * Principios: Template Method, herencia, polimorfismo, encapsulamiento.
 */
class YoloPipelineWidget : public BaseWidget {
    Q_OBJECT

public:
    explicit YoloPipelineWidget(QWidget* parent = nullptr);
    ~YoloPipelineWidget() override;

signals:
    void statusMessage(const QString& msg);
    void errorMessage(const QString& msg);
    void successMessage(const QString& msg);

protected:
    // ── Hooks virtuales (Template Method) ─────────────────────────────────
    /// Texto del estado para mostrar al usuario (varía por pantalla).
    virtual QString statusToString(Carlens::StreamStatus status) const = 0;
    /// Habilita/deshabilita los botones propios de cada subclase.
    virtual void    updateControls(Carlens::StreamStatus status) = 0;
    /// Hook al arrancar el pipeline YOLO (override opcional).
    virtual void    onPipelineStarted() {}
    /// Hook al detener el pipeline YOLO (override opcional).
    virtual void    onPipelineStopped() {}

    // ── Helpers para subclases ─────────────────────────────────────────────
    /// Actualiza el indicador visual y llama a updateControls().
    void    setStatus(Carlens::StreamStatus status);
    /// Conecta las señales YOLO/watchers/timer comunes. Llamar desde connectSignals().
    void    connectPipelineSignals();
    QString statusToColor(Carlens::StreamStatus status) const;

    // ── Pipeline (construido en el ctor base) ──────────────────────────────
    YoloProcessController*          m_yolo           = nullptr;
    std::unique_ptr<CaptureManager> m_captureManager;

    QFutureWatcher<CaptureResult>*             m_persistWatcher  = nullptr;
    QFutureWatcher<SnapshotPersistenceResult>* m_runOpWatcher    = nullptr;
    QFutureWatcher<VehicleAnalysisResult>*     m_analysisWatcher = nullptr;
    GeminiAnalysisService                      m_geminiService;

    bool    m_capturePersistInProgress = false;
    bool    m_analysisInProgress       = false;
    QString m_pendingRunOpLabel;

    // ── Estado del frame actual ────────────────────────────────────────────
    QImage        m_currentFrame;
    DetectionList m_currentDetections;
    int           m_currentFrameId   = 0;
    double        m_currentTimestamp = 0.0;
    bool          m_runActive        = false;

    int                   m_elapsedSeconds = 0;
    Carlens::StreamStatus m_currentStatus  = Carlens::StreamStatus::Disconnected;

    // ── Widgets de estado (asignados por la subclase en setupUI) ──────────
    QLabel*      m_statusDot        = nullptr;
    QLabel*      m_statusLabel      = nullptr;
    QLabel*      m_timerLabel       = nullptr;
    QLabel*      m_sessionInfoLabel = nullptr;
    VideoWidget* m_videoWidget      = nullptr;
    QTimer*      m_timer            = nullptr;

protected slots:
    void onFrameReady(const FramePayload& payload);
    void onDetectionClicked(int detectionIndex, QPoint imagePoint);
    void onYoloRunningChanged(bool running, const QString& statusMsg);
    void onYoloFinished(int exitCode);
    void onYoloLog(const QString& message);
    void onPersistFinished();
    void onRunOpFinished();
    void onAnalysisFinished();
    void onTimerTick();
};

#endif // YOLOPIPELINEWIDGET_H
