#include "widgets/RtspWidget.h"

#include <QDateTime>
#include <QFileInfo>
#include <QFont>
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

namespace {
QString deriveSourceType(const QString& source)
{
    const QString s = source.trimmed();
    if (s.startsWith("rtsp://", Qt::CaseInsensitive)) return "rtsp";
    const QString suffix = QFileInfo(s).suffix().toLower();
    return suffix.isEmpty() ? "file" : suffix;
}
}

RtspWidget::RtspWidget(QWidget* parent)
    : BaseWidget(parent)
    , m_connectionTimer(new QTimer(this))
    , m_yolo(new YoloProcessController(this))
    // LOCAL + REMOTO: el Composite guarda primero en captures/ (siempre) y luego
    // sincroniza al VPS (imagen por SFTP + filas en MySQL via tunel SSH). Si el
    // remoto falla, el guardado local NO se rompe: el fallo se reporta como
    // advertencia. La subida ocurre SOLO al hacer clic en un vehiculo (accion
    // manual con confirmacion), nunca de forma automatica.
    , m_captureManager(std::make_unique<CaptureManager>(
          std::make_unique<CompositeSnapshotPersistenceBackend>(
              std::make_unique<FileSystemSnapshotPersistenceBackend>(),
              std::make_unique<RemoteSnapshotPersistenceBackend>())))
    , m_persistWatcher(new QFutureWatcher<CaptureResult>(this))
    , m_runOpWatcher(new QFutureWatcher<SnapshotPersistenceResult>(this))
    , m_analysisWatcher(new QFutureWatcher<VehicleAnalysisResult>(this))
{
    setupUI();
    connectSignals();
    m_connectionTimer->setInterval(1000);
}

RtspWidget::~RtspWidget() = default;

void RtspWidget::setupUI()
{
    setObjectName("rtspWidget");
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(32, 32, 32, 32);
    rootLayout->setSpacing(20);

    // ── Título ────────────────────────────────────────────────────────
    QLabel* sectionTitle = new QLabel("Conexión RTSP", this);
    QFont tf = sectionTitle->font();
    tf.setPointSize(18);
    tf.setWeight(QFont::DemiBold);
    sectionTitle->setFont(tf);
    sectionTitle->setStyleSheet("color: #e6edf3;");

    QLabel* subtitle = new QLabel(
        "Conecte a una cámara IP o video y observe la detección YOLO en vivo. "
        "Haga clic sobre un vehículo para capturarlo.",
        this);
    subtitle->setStyleSheet("color: #8b949e; font-size: 13px;");
    subtitle->setWordWrap(true);

    // ── Card de conexión ──────────────────────────────────────────────
    QWidget* connCard = new QWidget(this);
    connCard->setStyleSheet(
        "background: #161b22; border: 1px solid #30363d; border-radius: 8px;");
    QGridLayout* connGrid = new QGridLayout(connCard);
    connGrid->setContentsMargins(20, 20, 20, 20);
    connGrid->setSpacing(12);

    QLabel* urlLabel = new QLabel("URL RTSP o ruta de video:", connCard);
    urlLabel->setStyleSheet("color: #8b949e; font-size: 12px;");
    m_rtspUrlEdit = new QLineEdit(connCard);
    m_rtspUrlEdit->setPlaceholderText("rtsp://127.0.0.1:8554/carlens");
    m_rtspUrlEdit->setMinimumHeight(40);
    m_rtspUrlEdit->setText("rtsp://127.0.0.1:8554/carlens");

    QLabel* exampleLabel = new QLabel(
        "Demo local: rtsp://127.0.0.1:8554/carlens · Video de prueba: Video/autos.mp4",
        connCard);
    exampleLabel->setStyleSheet("color: #484f58; font-size: 11px;");

    // ── Botones ───────────────────────────────────────────────────────
    QHBoxLayout* btnRow = new QHBoxLayout();
    m_connectBtn    = new QPushButton("Conectar", connCard);
    m_connectBtn->setProperty("class", "primary");
    m_connectBtn->setMinimumHeight(40);
    m_connectBtn->setMinimumWidth(120);

    m_disconnectBtn = new QPushButton("Desconectar", connCard);
    m_disconnectBtn->setProperty("class", "danger");
    m_disconnectBtn->setMinimumHeight(40);
    m_disconnectBtn->setMinimumWidth(120);
    m_disconnectBtn->setEnabled(false);
    btnRow->addWidget(m_connectBtn);
    btnRow->addWidget(m_disconnectBtn);
    btnRow->addStretch();

    connGrid->addWidget(urlLabel,      0, 0);
    connGrid->addWidget(m_rtspUrlEdit, 0, 1, 1, 3);
    connGrid->addWidget(exampleLabel,  1, 1, 1, 3);
    connGrid->addLayout(btnRow,        2, 0, 1, 4);

    // ── Estado de conexión ────────────────────────────────────────────
    QWidget* statusCard = new QWidget(this);
    statusCard->setStyleSheet(
        "background: #161b22; border: 1px solid #30363d; border-radius: 8px;");
    QHBoxLayout* statusLayout = new QHBoxLayout(statusCard);
    statusLayout->setContentsMargins(20, 12, 20, 12);
    statusLayout->setSpacing(10);

    m_statusDot = new QLabel(this);
    m_statusDot->setFixedSize(12, 12);
    m_statusDot->setStyleSheet("background-color: #8b949e; border-radius: 6px;");

    m_statusLabel = new QLabel("Desconectado", this);
    m_statusLabel->setStyleSheet("color: #8b949e; font-size: 13px; font-weight: 500;");

    m_timerLabel = new QLabel("", this);
    m_timerLabel->setStyleSheet("color: #484f58; font-size: 12px;");

    m_sessionInfoLabel = new QLabel("", this);
    m_sessionInfoLabel->setStyleSheet("color: #58a6ff; font-size: 12px;");

    statusLayout->addWidget(m_statusDot);
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_timerLabel);
    statusLayout->addWidget(m_sessionInfoLabel);

    // ── Video en vivo (VideoWidget real) ──────────────────────────────
    m_videoWidget = new VideoWidget(this);
    m_videoWidget->setMinimumHeight(320);

    // ── Instrucciones ─────────────────────────────────────────────────
    QWidget* infoCard = new QWidget(this);
    infoCard->setStyleSheet(
        "background: #111d2d; border: 1px solid #1f3a5f; border-radius: 8px;");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoCard);
    infoLayout->setContentsMargins(16, 12, 16, 12);
    QLabel* infoLabel = new QLabel(
        "ℹ️  <b>¿Cómo funciona?</b><br>"
        "Al conectar se lanza YOLOv11 (proceso Python local) sobre la fuente indicada y se "
        "dibujan las detecciones en vivo. Haga <b>clic sobre un vehículo</b> para capturarlo. "
        "<br>Cada captura se guarda en <b>captures/</b> y se <b>sincroniza con el servidor</b> "
        "(imagen + registro en la base de datos). La subida ocurre solo al hacer clic, nunca "
        "de forma automática.", infoCard);
    infoLabel->setWordWrap(true);
    infoLabel->setTextFormat(Qt::RichText);
    infoLabel->setStyleSheet("color: #8b949e; font-size: 12px; line-height: 1.5;");
    infoLayout->addWidget(infoLabel);

    // ── Ensamblar ─────────────────────────────────────────────────────
    rootLayout->addWidget(sectionTitle);
    rootLayout->addWidget(subtitle);
    rootLayout->addWidget(connCard);
    rootLayout->addWidget(statusCard);
    rootLayout->addWidget(m_videoWidget, 1);
    rootLayout->addWidget(infoCard);
}

void RtspWidget::connectSignals()
{
    connect(m_connectBtn,    &QPushButton::clicked, this, &RtspWidget::onConnectClicked);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &RtspWidget::onDisconnectClicked);

    connect(m_yolo, &YoloProcessController::frameReady,
            this,   &RtspWidget::onFrameReady);
    connect(m_yolo, &YoloProcessController::runningChanged,
            this,   &RtspWidget::onYoloRunningChanged);
    connect(m_yolo, &YoloProcessController::processFinished,
            this,   &RtspWidget::onYoloFinished);
    connect(m_yolo, &YoloProcessController::logMessage,
            this,   &RtspWidget::onYoloLog);

    connect(m_videoWidget, &VideoWidget::detectionClicked,
            this,          &RtspWidget::onDetectionClicked);

    connect(m_persistWatcher, &QFutureWatcher<CaptureResult>::finished,
            this,             &RtspWidget::onPersistFinished);
    connect(m_runOpWatcher, &QFutureWatcher<SnapshotPersistenceResult>::finished,
            this,           &RtspWidget::onRunOpFinished);
    connect(m_analysisWatcher, &QFutureWatcher<VehicleAnalysisResult>::finished,
            this,              &RtspWidget::onAnalysisFinished);

    connect(m_connectionTimer, &QTimer::timeout,
            this,              &RtspWidget::updateConnectionTime);
}

// ─── Slots ────────────────────────────────────────────────────────────────

void RtspWidget::onConnectClicked()
{
    const QString url = m_rtspUrlEdit->text().trimmed();
    if (url.isEmpty() || url == "rtsp://") {
        emit errorMessage("Ingrese una URL RTSP o ruta de video válida.");
        return;
    }

    setStatus(Carlens::StreamStatus::Connecting);

    QString err;
    if (!m_yolo->start(url, /*showWindow=*/false, &err)) {
        setStatus(Carlens::StreamStatus::Error);
        emit errorMessage(err.isEmpty() ? "No se pudo iniciar YOLO." : err);
        return;
    }

    // Configurar la persistencia local con la raiz del proyecto resuelta por YOLO.
    m_captureManager->setProjectRoot(m_yolo->projectRoot());

    ProcessingRunInfo runInfo;
    runInfo.sourcePath   = url;
    runInfo.sourceType   = deriveSourceType(url);
    runInfo.yoloModel    = "yolo11n.pt";
    runInfo.trackingModel = "manual_capture_no_tracker";
    runInfo.frameSkip    = 0;
    runInfo.confidenceThreshold = 0.35;
    runInfo.iouThreshold = -1.0;
    runInfo.notes        = "Sesión iniciada desde la pestaña RTSP";
    // Asociar la sesion al usuario logueado (created_by_user_id en la BD).
    runInfo.createdByUserId = SessionManager::instance()->currentUser().id();
    runInfo.startedAtUtc = QDateTime::currentDateTimeUtc();

    // start-session abre el tunel SSH: en hilo de fondo para no congelar la UI.
    m_runActive = true;
    m_pendingRunOpLabel = "begin";
    CaptureManager* mgr = m_captureManager.get();
    QFuture<SnapshotPersistenceResult> future = QtConcurrent::run(
        [mgr, runInfo]() { return mgr->beginProcessingRun(runInfo); });
    m_runOpWatcher->setFuture(future);

    emit statusMessage("Conectando a: " + url + " (estableciendo sesión…)");
}

void RtspWidget::onDisconnectClicked()
{
    if (m_runActive && !m_runOpWatcher->isRunning()) {
        ProcessingRunCompletionInfo completion;
        completion.status     = "completed";
        completion.endedAtUtc = QDateTime::currentDateTimeUtc();
        m_runActive = false;
        m_pendingRunOpLabel = "finish";
        // finish-session (UPDATE en MySQL) tambien en hilo de fondo.
        CaptureManager* mgr = m_captureManager.get();
        QFuture<SnapshotPersistenceResult> future = QtConcurrent::run(
            [mgr, completion]() { return mgr->finishProcessingRun(completion); });
        m_runOpWatcher->setFuture(future);
    } else {
        m_runActive = false;
    }

    m_yolo->stop();
    m_videoWidget->clearFrame();
    m_currentFrame = QImage();
    m_currentDetections.clear();
    setStatus(Carlens::StreamStatus::Disconnected);
}

void RtspWidget::onFrameReady(const FramePayload& payload)
{
    m_currentFrame      = payload.frame;
    m_currentDetections = payload.detections;
    m_currentFrameId    = payload.frameId;
    m_currentTimestamp  = payload.timestamp;

    m_videoWidget->setFrame(payload.frame, payload.detections,
                            payload.sourceWidth, payload.sourceHeight);
}

void RtspWidget::onDetectionClicked(int detectionIndex, QPoint imagePoint)
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

void RtspWidget::onPersistFinished()
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
        emit statusMessage("Captura sincronizada con el servidor: "
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

void RtspWidget::onAnalysisFinished()
{
    m_analysisInProgress = false;
    const VehicleAnalysisResult res = m_analysisWatcher->result();

    if (res.success) {
        const QString desc = QString("Análisis IA: %1 %2 · color %3%4 — actualice el Historial para verlo.")
                                 .arg(res.make.isEmpty() ? "?" : res.make)
                                 .arg(res.model)
                                 .arg(res.color.isEmpty() ? "?" : res.color)
                                 .arg(res.bodyType.isEmpty() ? "" : " · " + res.bodyType);
        emit statusMessage(desc);
        if (!res.warningMessage.isEmpty())
            emit errorMessage(res.warningMessage);
    } else if (!res.warningMessage.isEmpty()) {
        emit errorMessage(res.warningMessage);
    } else if (!res.errorMessage.isEmpty()) {
        emit errorMessage(res.errorMessage);
    }
}

void RtspWidget::onRunOpFinished()
{
    const SnapshotPersistenceResult result = m_runOpWatcher->result();
    const bool isBegin = (m_pendingRunOpLabel == "begin");

    if (!result.success) {
        if (isBegin)
            emit errorMessage("No se pudo iniciar la sesión en el servidor: " + result.errorMessage
                              + " — las capturas se guardarán solo localmente.");
        else
            emit errorMessage("No se pudo cerrar la sesión en el servidor: " + result.errorMessage);
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

void RtspWidget::onYoloRunningChanged(bool running, const QString& statusMsg)
{
    if (running) {
        setStatus(Carlens::StreamStatus::Connected);
        m_elapsedSeconds = 0;
        m_connectionTimer->start();
        m_sessionInfoLabel->setText("YOLO en vivo");
    } else {
        m_connectionTimer->stop();
        if (m_currentStatus != Carlens::StreamStatus::Error)
            setStatus(Carlens::StreamStatus::Disconnected);
        m_sessionInfoLabel->clear();
    }
    if (!statusMsg.isEmpty())
        emit statusMessage(statusMsg);
}

void RtspWidget::onYoloFinished(int exitCode)
{
    m_runActive = false;
    if (exitCode != 0) {
        setStatus(Carlens::StreamStatus::Error);
        emit errorMessage(QString("YOLO finalizó con código %1.").arg(exitCode));
    }
}

void RtspWidget::onYoloLog(const QString& message)
{
    // Reenviar logs/errores del proceso Python a la barra de estado.
    if (message.startsWith("[ERR]"))
        emit errorMessage(message);
    else
        emit statusMessage(message);
}

void RtspWidget::updateConnectionTime()
{
    ++m_elapsedSeconds;
    int h = m_elapsedSeconds / 3600;
    int m = (m_elapsedSeconds % 3600) / 60;
    int s = m_elapsedSeconds % 60;
    m_timerLabel->setText(
        QString("Tiempo: %1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0')));
}

// ─── Helpers ──────────────────────────────────────────────────────────────

void RtspWidget::setStatus(Carlens::StreamStatus status)
{
    m_currentStatus = status;
    m_statusLabel->setText(statusToString(status));
    m_statusDot->setStyleSheet(
        QString("background-color: %1; border-radius: 6px;").arg(statusToColor(status)));

    bool connected  = (status == Carlens::StreamStatus::Connected);
    bool connecting = (status == Carlens::StreamStatus::Connecting);
    m_connectBtn->setEnabled(!connected && !connecting);
    m_disconnectBtn->setEnabled(connected);
    m_rtspUrlEdit->setEnabled(!connected && !connecting);
}

QString RtspWidget::statusToString(Carlens::StreamStatus status) const
{
    switch (status) {
    case Carlens::StreamStatus::Disconnected: return "Desconectado";
    case Carlens::StreamStatus::Connecting:   return "Conectando…";
    case Carlens::StreamStatus::Connected:    return "Conectado — En vivo";
    case Carlens::StreamStatus::Error:        return "Error de conexión";
    }
    return "Desconocido";
}

QString RtspWidget::statusToColor(Carlens::StreamStatus status) const
{
    switch (status) {
    case Carlens::StreamStatus::Disconnected: return "#8b949e";
    case Carlens::StreamStatus::Connecting:   return "#d29922";
    case Carlens::StreamStatus::Connected:    return "#3fb950";
    case Carlens::StreamStatus::Error:        return "#f85149";
    }
    return "#8b949e";
}

void RtspWidget::reset()
{
    onDisconnectClicked();
    m_rtspUrlEdit->setText("rtsp://127.0.0.1:8554/carlens");
    setStatus(Carlens::StreamStatus::Disconnected);
    m_connectionTimer->stop();
    m_elapsedSeconds = 0;
    m_timerLabel->clear();
    m_sessionInfoLabel->clear();
}
