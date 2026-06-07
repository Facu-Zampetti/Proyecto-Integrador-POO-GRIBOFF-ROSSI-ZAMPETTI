#include "widgets/YoloPipelineWidget.h"

#include <QDateTime>
#include <QFileInfo>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrentRun>

#include "managers/SessionManager.h"
#include "yolo/VideoWidget.h"
#include "yolo/YoloProcessController.h"
#include "yolo/CaptureManager.h"
#include "yolo/FileSystemSnapshotPersistenceBackend.h"
#include "yolo/RemoteSnapshotPersistenceBackend.h"
#include "yolo/CompositeSnapshotPersistenceBackend.h"
#include "yolo/GeminiAnalysisService.h"

YoloPipelineWidget::YoloPipelineWidget(QWidget* parent)
    : BaseWidget(parent)
    , m_yolo(new YoloProcessController(this))
    // LOCAL + REMOTO: el Composite guarda primero en captures/ (siempre) y luego
    // sincroniza al VPS (imagen por SFTP + filas en MySQL via tunel SSH). Si el
    // remoto falla, el guardado local NO se rompe. La subida ocurre SOLO al hacer
    // clic en un vehiculo (accion manual), nunca de forma automatica.
    , m_captureManager(std::make_unique<CaptureManager>(
          std::make_unique<CompositeSnapshotPersistenceBackend>(
              std::make_unique<FileSystemSnapshotPersistenceBackend>(),
              std::make_unique<RemoteSnapshotPersistenceBackend>())))
    , m_persistWatcher(new QFutureWatcher<CaptureResult>(this))
    , m_runOpWatcher(new QFutureWatcher<SnapshotPersistenceResult>(this))
    , m_analysisWatcher(new QFutureWatcher<VehicleAnalysisResult>(this))
    , m_timer(new QTimer(this))
{
    m_timer->setInterval(1000);
}

YoloPipelineWidget::~YoloPipelineWidget() = default;

// ─── Helpers ──────────────────────────────────────────────────────────────

void YoloPipelineWidget::connectPipelineSignals()
{
    connect(m_yolo, &YoloProcessController::frameReady,
            this,   &YoloPipelineWidget::onFrameReady);
    connect(m_yolo, &YoloProcessController::runningChanged,
            this,   &YoloPipelineWidget::onYoloRunningChanged);
    connect(m_yolo, &YoloProcessController::processFinished,
            this,   &YoloPipelineWidget::onYoloFinished);
    connect(m_yolo, &YoloProcessController::logMessage,
            this,   &YoloPipelineWidget::onYoloLog);

    connect(m_videoWidget, &VideoWidget::detectionClicked,
            this,          &YoloPipelineWidget::onDetectionClicked);

    connect(m_persistWatcher, &QFutureWatcher<CaptureResult>::finished,
            this,             &YoloPipelineWidget::onPersistFinished);
    connect(m_runOpWatcher,   &QFutureWatcher<SnapshotPersistenceResult>::finished,
            this,             &YoloPipelineWidget::onRunOpFinished);
    connect(m_analysisWatcher, &QFutureWatcher<VehicleAnalysisResult>::finished,
            this,              &YoloPipelineWidget::onAnalysisFinished);

    connect(m_timer, &QTimer::timeout,
            this,    &YoloPipelineWidget::onTimerTick);
}

void YoloPipelineWidget::setStatus(Carlens::StreamStatus status)
{
    m_currentStatus = status;
    m_statusLabel->setText(statusToString(status));
    m_statusDot->setStyleSheet(
        QString("background-color: %1; border-radius: 6px;").arg(statusToColor(status)));
    updateControls(status);
}

QString YoloPipelineWidget::statusToColor(Carlens::StreamStatus status) const
{
    switch (status) {
    case Carlens::StreamStatus::Disconnected: return "#8b949e";
    case Carlens::StreamStatus::Connecting:   return "#d29922";
    case Carlens::StreamStatus::Connected:    return "#3fb950";
    case Carlens::StreamStatus::Error:        return "#f85149";
    }
    return "#8b949e";
}

// ─── Slots comunes ────────────────────────────────────────────────────────

void YoloPipelineWidget::onTimerTick()
{
    ++m_elapsedSeconds;
    const int h = m_elapsedSeconds / 3600;
    const int m = (m_elapsedSeconds % 3600) / 60;
    const int s = m_elapsedSeconds % 60;
    m_timerLabel->setText(
        QString("Tiempo: %1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0')));
}

void YoloPipelineWidget::onFrameReady(const FramePayload& payload)
{
    m_currentFrame      = payload.frame;
    m_currentDetections = payload.detections;
    m_currentFrameId    = payload.frameId;
    m_currentTimestamp  = payload.timestamp;
    m_videoWidget->setFrame(payload.frame, payload.detections,
                            payload.sourceWidth, payload.sourceHeight);
}

void YoloPipelineWidget::onDetectionClicked(int detectionIndex, QPoint imagePoint)
{
    if (detectionIndex < 0 || detectionIndex >= m_currentDetections.size())
        return;

    if (m_capturePersistInProgress) {
        emit statusMessage("Ya hay una captura procesándose. Espere a que termine.");
        return;
    }

    const DetectionBox detection = m_currentDetections.at(detectionIndex);

    // prepareDetection (recorte) corre en el hilo de UI ANTES del dialogo, asi el
    // snapshot queda congelado aunque sigan llegando frames nuevos.
    const CaptureResult prepared = m_captureManager->prepareDetection(
        m_currentFrame, m_currentFrameId, m_currentTimestamp, detection, imagePoint);
    if (!prepared.success) {
        emit errorMessage(prepared.errorMessage);
        return;
    }

    const QString caption = QString("%1 (%2)")
                                .arg(detection.label)
                                .arg(detection.confidence, 0, 'f', 2);
    const auto answer = QMessageBox::question(
        this,
        "Capturar vehículo",
        QString("¿Capturar este vehículo: %1?\n\nSe guardará localmente y se "
                "sincronizará con el servidor.").arg(caption),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);
    if (answer != QMessageBox::Yes)
        return;

    // La sincronizacion remota (tunel SSH + SFTP + MySQL) corre en hilo de fondo
    // para no congelar la ventana; el resultado llega en onPersistFinished().
    m_capturePersistInProgress = true;
    emit statusMessage("Guardando captura y sincronizando con el servidor…");

    CaptureManager* mgr = m_captureManager.get();
    QFuture<CaptureResult> future = QtConcurrent::run(
        [mgr, snapshot = prepared.snapshot]() mutable {
            return mgr->persistSnapshot(std::move(snapshot));
        });
    m_persistWatcher->setFuture(future);
}

void YoloPipelineWidget::onPersistFinished()
{
    m_capturePersistInProgress = false;
    const CaptureResult saved = m_persistWatcher->result();

    if (!saved.success) {
        emit errorMessage(saved.errorMessage.isEmpty()
                              ? "No se pudo guardar la captura."
                              : saved.errorMessage);
        return;
    }

    // El local siempre se guarda; si el remoto fallo, llega como advertencia.
    if (!saved.warningMessage.isEmpty()) {
        emit errorMessage("Captura local OK, pero la sincronización con el VPS falló: "
                          + saved.warningMessage);
    } else {
        emit successMessage("Captura sincronizada con el servidor: "
                            + saved.snapshot.localImagePath);
    }

    // Encadenar el analisis de IA (Gemini) en segundo plano sobre la captura.
    if (m_analysisInProgress)
        return;
    m_analysisInProgress = true;
    emit statusMessage("Analizando el vehículo con IA (Gemini)…");

    GeminiAnalysisService* svc = &m_geminiService;
    const QString projectRoot = m_yolo->projectRoot();
    const int userId = SessionManager::instance()->currentUser().id();
    QFuture<VehicleAnalysisResult> future = QtConcurrent::run(
        [svc, snapshot = saved.snapshot, projectRoot, userId]() {
            return svc->analyzeSnapshot(snapshot, projectRoot, userId);
        });
    m_analysisWatcher->setFuture(future);
}

void YoloPipelineWidget::onAnalysisFinished()
{
    m_analysisInProgress = false;
    const VehicleAnalysisResult res = m_analysisWatcher->result();

    if (res.success) {
        const QString desc =
            QString("Análisis IA: %1 %2 · color %3%4 — actualice el Historial para verlo.")
                .arg(res.make.isEmpty() ? "?" : res.make)
                .arg(res.model)
                .arg(res.color.isEmpty() ? "?" : res.color)
                .arg(res.bodyType.isEmpty() ? "" : " · " + res.bodyType);
        emit successMessage(desc);
        if (!res.warningMessage.isEmpty())
            emit errorMessage(res.warningMessage);
    } else if (!res.warningMessage.isEmpty()) {
        emit errorMessage(res.warningMessage);
    } else if (!res.errorMessage.isEmpty()) {
        emit errorMessage(res.errorMessage);
    }
}

void YoloPipelineWidget::onRunOpFinished()
{
    const SnapshotPersistenceResult result = m_runOpWatcher->result();
    const bool isBegin = (m_pendingRunOpLabel == "begin");

    if (!result.success) {
        if (isBegin)
            emit errorMessage("No se pudo iniciar la sesión en el servidor: "
                              + result.errorMessage
                              + " — las capturas se guardarán solo localmente.");
        else
            emit errorMessage("No se pudo cerrar la sesión en el servidor: "
                              + result.errorMessage);
        return;
    }

    if (!result.warningMessage.isEmpty()) {
        emit errorMessage(result.warningMessage);
    } else if (!result.infoMessage.isEmpty()) {
        emit statusMessage(result.infoMessage);
    } else if (isBegin) {
        emit statusMessage("Sesión iniciada en el servidor.");
    }
}

void YoloPipelineWidget::onYoloRunningChanged(bool running, const QString& statusMsg)
{
    if (running) {
        setStatus(Carlens::StreamStatus::Connected);
        m_elapsedSeconds = 0;
        m_timer->start();
        m_sessionInfoLabel->setText("YOLO en vivo");
        onPipelineStarted();
    } else {
        m_timer->stop();
        if (m_currentStatus != Carlens::StreamStatus::Error)
            setStatus(Carlens::StreamStatus::Disconnected);
        m_sessionInfoLabel->clear();
        onPipelineStopped();
    }
    if (!statusMsg.isEmpty())
        emit statusMessage(statusMsg);
}

void YoloPipelineWidget::onYoloFinished(int exitCode)
{
    m_runActive = false;
    if (exitCode != 0) {
        setStatus(Carlens::StreamStatus::Error);
        emit errorMessage(QString("YOLO finalizó con código %1.").arg(exitCode));
    }
}

void YoloPipelineWidget::onYoloLog(const QString& message)
{
    if (message.startsWith("[ERR]"))
        emit errorMessage(message);
    else
        emit statusMessage(message);
}
