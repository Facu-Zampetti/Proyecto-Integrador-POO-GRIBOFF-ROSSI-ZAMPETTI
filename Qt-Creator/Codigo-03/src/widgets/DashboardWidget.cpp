#include "widgets/DashboardWidget.h"
#include "managers/ConfigManager.h"
#include "managers/HistoryManager.h"
#include "managers/RtspManager.h"
#include <QFrame>
#include <QUrl>

DashboardWidget::DashboardWidget(QWidget* parent)
    : BaseWidget(parent)
{
    m_backgroundHistoryRefreshTimer = new QTimer(this);
    m_backgroundHistoryRefreshTimer->setInterval(5000);
    setupUI();
    connectSignals();
}

void DashboardWidget::setupUI()
{
    setObjectName("dashboardWidget");

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ── Top Bar ───────────────────────────────────────────────────────
    m_topBar = new QWidget(this);
    m_topBar->setObjectName("topbar");
    m_topBar->setFixedHeight(48);
    QHBoxLayout* topLayout = new QHBoxLayout(m_topBar);
    topLayout->setContentsMargins(20, 0, 20, 0);

    m_topBarTitle = new QLabel("Dashboard", m_topBar);
    QFont f = m_topBarTitle->font();
    f.setPointSize(14);
    f.setWeight(QFont::DemiBold);
    m_topBarTitle->setFont(f);
    m_topBarTitle->setStyleSheet("color: #e6edf3;");
    topLayout->addWidget(m_topBarTitle);
    topLayout->addStretch();

    // Indicador de conexión backend
    const QString backendUrl = ConfigManager::instance()->backendUrl();
    const QUrl parsedBackendUrl(backendUrl);
    const QString backendDisplay = parsedBackendUrl.host().isEmpty()
        ? backendUrl
        : parsedBackendUrl.host();
    QLabel* backendLabel = new QLabel(QString("Backend: %1").arg(backendDisplay), m_topBar);
    backendLabel->setStyleSheet("color: #484f58; font-size: 11px;");
    topLayout->addWidget(backendLabel);

    // ── Separador ──────────────────────────────────────────────────────
    QFrame* topSep = new QFrame(this);
    topSep->setFrameShape(QFrame::HLine);
    topSep->setStyleSheet("color: #21262d;");

    // ── Contenido principal ───────────────────────────────────────────
    QWidget* mainContent = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(mainContent);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Sidebar
    m_sidebar = new SidebarWidget(this);

    // Separador vertical
    QFrame* sideSep = new QFrame(this);
    sideSep->setFrameShape(QFrame::VLine);
    sideSep->setStyleSheet("color: #21262d;");

    // Stack de contenido
    m_contentStack = new QStackedWidget(this);
    m_contentStack->setObjectName("contentStack");

    // Crear los widgets de sección
    m_videoWidget    = new VideoUploadWidget(this);
    m_rtspWidget     = new RtspWidget(this);
    m_historyWidget  = new HistoryWidget(this);
    m_analysisWidget = new VehicleAnalysisWidget(this);

    m_videoIndex    = m_contentStack->addWidget(m_videoWidget);
    m_rtspIndex     = m_contentStack->addWidget(m_rtspWidget);
    m_historyIndex  = m_contentStack->addWidget(m_historyWidget);
    m_analysisIndex = m_contentStack->addWidget(m_analysisWidget);

    mainLayout->addWidget(m_sidebar);
    mainLayout->addWidget(sideSep);
    mainLayout->addWidget(m_contentStack, 1);

    // ── Status Bar ────────────────────────────────────────────────────
    m_statusBar = new StatusBarWidget(this);

    // ── Ensamblar todo ────────────────────────────────────────────────
    rootLayout->addWidget(m_topBar);
    rootLayout->addWidget(topSep);
    rootLayout->addWidget(mainContent, 1);
    rootLayout->addWidget(m_statusBar);

    // Sección inicial
    m_contentStack->setCurrentIndex(m_videoIndex);
    m_sidebar->setActiveSection(Carlens::DashboardSection::VideoUpload);
}

void DashboardWidget::connectSignals()
{
    // Navegación desde la sidebar
    connect(m_sidebar, &SidebarWidget::sectionRequested,
            this,      &DashboardWidget::showSection);

    connect(m_sidebar, &SidebarWidget::logoutRequested,
            this,      &DashboardWidget::logoutRequested);

    // Mensajes en StatusBar desde los sub-widgets
    connect(m_videoWidget, &VideoUploadWidget::statusMessage,
            m_statusBar, [this](const QString& msg){ m_statusBar->showMessage(msg); });
    connect(m_videoWidget, &VideoUploadWidget::errorMessage,
            m_statusBar, [this](const QString& msg){ m_statusBar->showError(msg); });
    connect(m_videoWidget, &VideoUploadWidget::successMessage,
            m_statusBar, [this](const QString& msg){ m_statusBar->showSuccess(msg); });

    connect(m_rtspWidget, &RtspWidget::statusMessage,
            m_statusBar, [this](const QString& msg){ m_statusBar->showMessage(msg); });
    connect(m_rtspWidget, &RtspWidget::errorMessage,
            m_statusBar, [this](const QString& msg){ m_statusBar->showError(msg); });

    connect(m_historyWidget, &HistoryWidget::vehicleSelectedForAnalysis,
            this, [this](int vehicleId) {
                m_analysisWidget->loadVehicle(vehicleId);
                showSection(Carlens::DashboardSection::Analysis);
            });

    connect(m_historyWidget, &HistoryWidget::statusMessage,
            m_statusBar, [this](const QString& msg){ m_statusBar->showMessage(msg); });

    connect(m_videoWidget, &VideoUploadWidget::successMessage,
            this, [this](const QString&) {
                HistoryManager::instance()->loadHistory();
            });

    auto* rtspManager = RtspManager::instance();
    connect(rtspManager, &RtspManager::streamStarted,
            this, [this](const SessionModel&) {
                HistoryManager::instance()->loadHistory();
                updateHistoryRefreshPolling();
            });
    connect(rtspManager, &RtspManager::streamStopped,
            this, [this]() {
                updateHistoryRefreshPolling();
                HistoryManager::instance()->loadHistory();
            });
    connect(rtspManager, &RtspManager::streamError,
            this, [this](const QString&) {
                updateHistoryRefreshPolling();
            });

    connect(m_backgroundHistoryRefreshTimer, &QTimer::timeout,
            this, []() {
                HistoryManager::instance()->loadHistory();
            });
}

void DashboardWidget::setCurrentUser(const UserModel& user)
{
    reset();
    m_sidebar->setCurrentUser(user);
    // Cargar historial al entrar al dashboard
    HistoryManager::instance()->loadHistory();
}

void DashboardWidget::reset()
{
    m_backgroundHistoryRefreshTimer->stop();
    m_videoWidget->reset();
    m_rtspWidget->reset();
    m_historyWidget->reset();
    m_analysisWidget->reset();
    HistoryManager::instance()->reset();
    m_contentStack->setCurrentIndex(m_videoIndex);
    m_sidebar->setActiveSection(Carlens::DashboardSection::VideoUpload);
    m_topBarTitle->setText("Cargar Video");
}

void DashboardWidget::updateHistoryRefreshPolling()
{
    const bool shouldPollHistory =
        m_currentSection == Carlens::DashboardSection::History &&
        RtspManager::instance()->streamStatus() == Carlens::StreamStatus::Connected;

    if (shouldPollHistory) {
        if (!m_backgroundHistoryRefreshTimer->isActive())
            m_backgroundHistoryRefreshTimer->start();
    } else {
        m_backgroundHistoryRefreshTimer->stop();
    }
}

void DashboardWidget::showSection(Carlens::DashboardSection section)
{
    m_currentSection = section;
    m_sidebar->setActiveSection(section);

    switch (section) {
    case Carlens::DashboardSection::VideoUpload:
        m_contentStack->setCurrentIndex(m_videoIndex);
        m_topBarTitle->setText("Cargar Video");
        break;
    case Carlens::DashboardSection::Rtsp:
        m_contentStack->setCurrentIndex(m_rtspIndex);
        m_topBarTitle->setText("Stream RTSP");
        break;
    case Carlens::DashboardSection::History:
        m_contentStack->setCurrentIndex(m_historyIndex);
        m_topBarTitle->setText("Historial de Vehículos");
        m_historyWidget->refresh();
        break;
    case Carlens::DashboardSection::Analysis:
        m_contentStack->setCurrentIndex(m_analysisIndex);
        m_topBarTitle->setText("Análisis IA — Gemini");
        break;
    default:
        break;
    }

    updateHistoryRefreshPolling();
}
