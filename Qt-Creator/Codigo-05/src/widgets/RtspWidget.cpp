#include "widgets/RtspWidget.h"

#include <QDateTime>
#include <QFileInfo>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>

#include "managers/SessionManager.h"
#include "managers/ConfigManager.h"
#include "yolo/VideoWidget.h"
#include "yolo/YoloProcessController.h"
#include "yolo/CaptureManager.h"

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
    : YoloPipelineWidget(parent)
{
    setupUI();
    connectSignals();
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

    // URL por defecto configurable (persistida en SQLite via ConfigManager): apunta
    // al media server del VPS alimentado por el telefono. El usuario puede editarla.
    const QString defaultRtspUrl = ConfigManager::instance()->rtspDefaultUrl();

    QLabel* urlLabel = new QLabel("URL RTSP o ruta de video:", connCard);
    urlLabel->setStyleSheet("color: #8b949e; font-size: 12px;");
    m_rtspUrlEdit = new QLineEdit(connCard);
    m_rtspUrlEdit->setPlaceholderText(defaultRtspUrl);
    m_rtspUrlEdit->setMinimumHeight(40);
    m_rtspUrlEdit->setText(defaultRtspUrl);

    QLabel* exampleLabel = new QLabel(
        "Cámara del teléfono (Larix → VPS) o video de prueba local: Video/autos.mp4",
        connCard);
    exampleLabel->setStyleSheet("color: #484f58; font-size: 12px;");

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
        "<b>¿Cómo funciona?</b><br>"
        "Transmita la cámara del teléfono con <b>Larix Broadcaster</b> (push RTMP al "
        "servidor) y aquí se consume por RTSP. Al conectar se lanza YOLOv11 (proceso Python "
        "local) sobre la fuente indicada y se dibujan las detecciones en vivo. Haga "
        "<b>clic sobre un vehículo</b> para capturarlo. "
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

    connectPipelineSignals();
}

// ─── Slots específicos de RTSP ────────────────────────────────────────────

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

// ─── Hooks del Template Method ────────────────────────────────────────────

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

void RtspWidget::updateControls(Carlens::StreamStatus status)
{
    const bool connected  = (status == Carlens::StreamStatus::Connected);
    const bool connecting = (status == Carlens::StreamStatus::Connecting);
    m_connectBtn->setEnabled(!connected && !connecting);
    m_disconnectBtn->setEnabled(connected);
    m_rtspUrlEdit->setEnabled(!connected && !connecting);
}

void RtspWidget::onPipelineStarted()
{
    emit streamStarted();
}

void RtspWidget::onPipelineStopped()
{
    emit streamStopped();
}

// ─── Reset ────────────────────────────────────────────────────────────────

void RtspWidget::reset()
{
    onDisconnectClicked();
    m_rtspUrlEdit->setText(ConfigManager::instance()->rtspDefaultUrl());
    setStatus(Carlens::StreamStatus::Disconnected);
    m_timer->stop();
    m_elapsedSeconds = 0;
    m_timerLabel->clear();
    m_sessionInfoLabel->clear();
}
