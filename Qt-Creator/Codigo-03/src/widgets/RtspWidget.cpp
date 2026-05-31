#include "widgets/RtspWidget.h"
#include "managers/RtspManager.h"
#include <QPainter>
#include <QFont>

RtspWidget::RtspWidget(QWidget* parent)
    : BaseWidget(parent)
    , m_connectionTimer(new QTimer(this))
{
    setupUI();
    connectSignals();
    m_connectionTimer->setInterval(1000);
}

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
        "Conecte a una cámara IP mediante protocolo RTSP para vigilancia en tiempo real.",
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

    QLabel* urlLabel = new QLabel("URL RTSP:", connCard);
    urlLabel->setStyleSheet("color: #8b949e; font-size: 12px;");
    m_rtspUrlEdit = new QLineEdit(connCard);
    m_rtspUrlEdit->setPlaceholderText("rtsp://127.0.0.1:8554/carlens");
    m_rtspUrlEdit->setMinimumHeight(40);
    m_rtspUrlEdit->setText("rtsp://127.0.0.1:8554/carlens");

    QLabel* exampleLabel = new QLabel(
        "Demo local: rtsp://127.0.0.1:8554/carlens · Cámara real: rtsp://admin:password@192.168.1.100:554/Streaming/Channels/1",
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

    connGrid->addWidget(urlLabel,     0, 0);
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
    m_statusDot->setStyleSheet(
        "background-color: #8b949e; border-radius: 6px;");

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

    // ── Área de "preview" del stream ──────────────────────────────────
    m_previewArea = new QWidget(this);
    m_previewArea->setMinimumHeight(280);
    m_previewArea->setStyleSheet(
        "background: #010409; border: 1px solid #21262d; border-radius: 8px;");

    QVBoxLayout* previewLayout = new QVBoxLayout(m_previewArea);
    m_previewLabel = new QLabel(m_previewArea);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("color: #30363d; font-size: 13px;");
    m_previewLabel->setText(
        "📡  Sin señal\n\nEl stream de video se muestra aquí cuando\nel backend procesa el feed RTSP con YOLOv11.");
    previewLayout->addWidget(m_previewLabel);

    // ── Instrucciones ─────────────────────────────────────────────────
    QWidget* infoCard = new QWidget(this);
    infoCard->setStyleSheet(
        "background: #111d2d; border: 1px solid #1f3a5f; border-radius: 8px;");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoCard);
    infoLayout->setContentsMargins(16, 12, 16, 12);
    QLabel* infoLabel = new QLabel(
        "ℹ️  <b>¿Cómo funciona?</b><br>"
        "Al conectar, el backend recibe la URL RTSP y comienza a procesar frames con YOLOv11. "
        "Los vehículos detectados se guardan automáticamente en el historial. "
        "Puede usar el script scripts/start-local-rtsp-demo.ps1 para levantar un stream RTSP local desde un MP4.", infoCard);
    infoLabel->setWordWrap(true);
    infoLabel->setTextFormat(Qt::RichText);
    infoLabel->setStyleSheet("color: #8b949e; font-size: 12px; line-height: 1.5;");
    infoLayout->addWidget(infoLabel);

    // ── Ensamblar ─────────────────────────────────────────────────────
    rootLayout->addWidget(sectionTitle);
    rootLayout->addWidget(subtitle);
    rootLayout->addWidget(connCard);
    rootLayout->addWidget(statusCard);
    rootLayout->addWidget(m_previewArea, 1);
    rootLayout->addWidget(infoCard);
}

void RtspWidget::connectSignals()
{
    connect(m_connectBtn, &QPushButton::clicked,
            this,         &RtspWidget::onConnectClicked);
    connect(m_disconnectBtn, &QPushButton::clicked,
            this,            &RtspWidget::onDisconnectClicked);

    auto* rm = RtspManager::instance();
    connect(rm, &RtspManager::streamStatusChanged,
            this, &RtspWidget::onStreamStatusChanged);
    connect(rm, &RtspManager::streamStarted,
            this, &RtspWidget::onStreamStarted);
    connect(rm, &RtspManager::streamStopped,
            this, [this]() {
                setStatus(Carlens::StreamStatus::Disconnected);
                m_connectionTimer->stop();
                m_elapsedSeconds = 0;
                emit statusMessage("Stream RTSP detenido.");
            });
    connect(rm, &RtspManager::streamError,
            this, &RtspWidget::onStreamError);

    connect(m_connectionTimer, &QTimer::timeout,
            this,              &RtspWidget::updateConnectionTime);
}

// ─── Slots ────────────────────────────────────────────────────────────────

void RtspWidget::onConnectClicked()
{
    QString url = m_rtspUrlEdit->text().trimmed();
    if (url.isEmpty() || url == "rtsp://") {
        emit errorMessage("Ingrese una URL RTSP válida.");
        return;
    }
    setStatus(Carlens::StreamStatus::Connecting);
    RtspManager::instance()->startStream(url);
    emit statusMessage("Conectando a: " + url);
}

void RtspWidget::onDisconnectClicked()
{
    RtspManager::instance()->stopStream();
    m_connectBtn->setEnabled(true);
    m_disconnectBtn->setEnabled(false);
}

void RtspWidget::onStreamStatusChanged(Carlens::StreamStatus status)
{
    setStatus(status);
}

void RtspWidget::onStreamStarted(const SessionModel& session)
{
    m_elapsedSeconds = 0;
    m_connectionTimer->start();
    m_sessionInfoLabel->setText(QString("Sesión #%1").arg(session.id()));
    m_previewLabel->setText(
        "📡  Stream activo\n\nEl backend está procesando el video.\n"
        "Los vehículos detectados aparecerán en el Historial.");
    m_previewLabel->setStyleSheet("color: #3fb950; font-size: 13px;");
    emit statusMessage("Stream RTSP conectado · Sesión #" +
                       QString::number(session.id()));
}

void RtspWidget::onStreamError(const QString& reason)
{
    setStatus(Carlens::StreamStatus::Error);
    m_connectionTimer->stop();
    emit errorMessage(reason);
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
        QString("background-color: %1; border-radius: 6px;")
        .arg(statusToColor(status)));

    bool connected = (status == Carlens::StreamStatus::Connected);
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
    m_rtspUrlEdit->setText("rtsp://127.0.0.1:8554/carlens");
    setStatus(Carlens::StreamStatus::Disconnected);
    m_connectionTimer->stop();
    m_elapsedSeconds = 0;
    m_timerLabel->clear();
    m_sessionInfoLabel->clear();
    m_previewLabel->setText(
        "📡  Sin señal\n\nEl stream de video se muestra aquí cuando\nel backend procesa el feed RTSP.");
    m_previewLabel->setStyleSheet("color: #30363d; font-size: 13px;");
}
