#include "widgets/SidebarWidget.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QPainter>
#include <QSpacerItem>
#include <QStyle>

SidebarWidget::SidebarWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("sidebar");
    setMinimumWidth(200);
    setMaximumWidth(240);
    setupUI();
    connectSignals();
}

void SidebarWidget::setupUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ── Logo / Header ─────────────────────────────────────────────────
    QWidget* header = new QWidget(this);
    header->setFixedHeight(64);
    header->setStyleSheet("background-color: #010409;");
    QVBoxLayout* headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(16, 0, 16, 0);

    // Logo: ícono SVG + texto en fila horizontal
    QWidget* logoWidget = new QWidget(header);
    QHBoxLayout* logoRow = new QHBoxLayout(logoWidget);
    logoRow->setContentsMargins(0, 0, 0, 0);
    logoRow->setSpacing(8);

    QLabel* logoIcon = new QLabel(logoWidget);
    logoIcon->setFixedSize(20, 20);
    logoIcon->setPixmap(QIcon(":/icons/target.svg").pixmap(QSize(20, 20)));

    QLabel* logoText = new QLabel("Carlens", logoWidget);
    QFont logoFont = logoText->font();
    logoFont.setPointSize(16);
    logoFont.setWeight(QFont::Bold);
    logoText->setFont(logoFont);
    logoText->setStyleSheet("color: #58a6ff; letter-spacing: 2px;");

    logoRow->addWidget(logoIcon);
    logoRow->addWidget(logoText);
    logoRow->addStretch();
    headerLayout->addWidget(logoWidget);

    QFrame* sep1 = new QFrame(this);
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet("color: #21262d; margin: 0;");

    // ── Perfil del usuario ────────────────────────────────────────────
    QWidget* userSection = new QWidget(this);
    userSection->setStyleSheet("background-color: transparent;");
    QHBoxLayout* userLayout = new QHBoxLayout(userSection);
    userLayout->setContentsMargins(16, 12, 16, 12);
    userLayout->setSpacing(10);

    m_avatarLabel = new QLabel(this);
    m_avatarLabel->setFixedSize(36, 36);
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setStyleSheet(
        "background-color: #1f6feb; border-radius: 18px; "
        "color: white; font-weight: bold; font-size: 14px;");

    QVBoxLayout* userInfoLayout = new QVBoxLayout();
    userInfoLayout->setSpacing(2);
    m_userLabel = new QLabel("Usuario", this);
    m_userLabel->setStyleSheet("color: #e6edf3; font-size: 13px; font-weight: 500;");
    m_emailLabel = new QLabel("email@carlens.com", this);
    m_emailLabel->setStyleSheet("color: #8b949e; font-size: 13px;");  // 11→13
    m_emailLabel->setWordWrap(true);
    userInfoLayout->addWidget(m_userLabel);
    userInfoLayout->addWidget(m_emailLabel);

    userLayout->addWidget(m_avatarLabel);
    userLayout->addLayout(userInfoLayout);

    QFrame* sep2 = new QFrame(this);
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("color: #21262d; margin: 0;");

    // ── Sección de navegación ─────────────────────────────────────────
    QWidget* navSection = new QWidget(this);
    QVBoxLayout* navLayout = new QVBoxLayout(navSection);
    navLayout->setContentsMargins(8, 8, 8, 8);
    navLayout->setSpacing(2);

    QLabel* navTitle = new QLabel("MENÚ", navSection);
    navTitle->setStyleSheet(
        "color: #484f58; font-size: 12px; font-weight: 700; "  // 10→12
        "letter-spacing: 1px; padding: 4px 8px;");
    navLayout->addWidget(navTitle);

    m_navGroup = new QButtonGroup(this);
    m_navGroup->setExclusive(true);

    auto addBtn = [&](const QString& iconPath, const QString& text,
                       Carlens::DashboardSection section) {
        QPushButton* btn = createNavButton(text, iconPath, section);
        m_navGroup->addButton(btn);
        m_navButtons[section] = btn;
        navLayout->addWidget(btn);
    };

    // Ícono SVG real en lugar de emoji
    addBtn(":/icons/folder.svg",    "  Cargar Video",   Carlens::DashboardSection::VideoUpload);
    addBtn(":/icons/satellite.svg", "  Stream RTSP",    Carlens::DashboardSection::Rtsp);
    addBtn(":/icons/list.svg",      "  Historial",      Carlens::DashboardSection::History);
    addBtn(":/icons/search.svg",    "  Análisis IA",    Carlens::DashboardSection::Analysis);
    addBtn(":/icons/settings.svg",  "  Configuración",  Carlens::DashboardSection::Settings);

    // ── Separador inferior ────────────────────────────────────────────
    QFrame* sep3 = new QFrame(this);
    sep3->setFrameShape(QFrame::HLine);
    sep3->setStyleSheet("color: #21262d; margin: 0 8px;");
    navLayout->addWidget(sep3);

    // ── Botón Logout ──────────────────────────────────────────────────
    QPushButton* logoutBtn = new QPushButton("  Cerrar Sesión", navSection);
    logoutBtn->setProperty("class", "sidebar-btn");
    logoutBtn->setCursor(Qt::PointingHandCursor);
    logoutBtn->setStyleSheet(
        "QPushButton { color: #f85149; background: transparent; border: none; "
        "border-radius: 6px; text-align: left; padding: 10px 16px; font-size: 13px; }"
        "QPushButton:hover { background-color: #2d1719; }");
    navLayout->addWidget(logoutBtn);
    connect(logoutBtn, &QPushButton::clicked, this, &SidebarWidget::logoutRequested);

    // ── Ensamblar ─────────────────────────────────────────────────────
    layout->addWidget(header);
    layout->addWidget(sep1);
    layout->addWidget(userSection);
    layout->addWidget(sep2);
    layout->addWidget(navSection);
    layout->addStretch();

    // Activar el primer botón por defecto
    if (!m_navButtons.isEmpty())
        m_navButtons.begin().value()->setProperty("active", true);
}

void SidebarWidget::connectSignals()
{
    for (auto it = m_navButtons.begin(); it != m_navButtons.end(); ++it) {
        const Carlens::DashboardSection section = it.key();
        QPushButton* btn = it.value();
        connect(btn, &QPushButton::clicked, this, [this, section]() {
            emit sectionRequested(section);
        });
    }
}

QPushButton* SidebarWidget::createNavButton(const QString& text,
                                             const QString& iconPath,
                                             Carlens::DashboardSection /*section*/)
{
    QPushButton* btn = new QPushButton(text, this);
    btn->setProperty("class", "sidebar-btn");
    btn->setCursor(Qt::PointingHandCursor);
    btn->setCheckable(true);
    btn->setMinimumHeight(40);
    if (!iconPath.isEmpty()) {
        btn->setIcon(QIcon(iconPath));
        btn->setIconSize(QSize(18, 18));
    }
    return btn;
}

void SidebarWidget::setCurrentUser(const UserModel& user)
{
    m_userLabel->setText(user.username());
    m_emailLabel->setText(user.email());
    if (!user.username().isEmpty())
        m_avatarLabel->setText(user.username().at(0).toUpper());
}

void SidebarWidget::setActiveSection(Carlens::DashboardSection section)
{
    for (auto it = m_navButtons.begin(); it != m_navButtons.end(); ++it) {
        bool active = (it.key() == section);
        it.value()->setChecked(active);
        it.value()->setProperty("active", active ? "true" : "false");
        it.value()->style()->unpolish(it.value());
        it.value()->style()->polish(it.value());
    }
}
