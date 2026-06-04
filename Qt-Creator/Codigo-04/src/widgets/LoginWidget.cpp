#include "widgets/LoginWidget.h"
#include "managers/UserManager.h"
#include <QFrame>
#include <QSpacerItem>
#include <QKeyEvent>
#include <QGraphicsDropShadowEffect>
#include <QDebug>

LoginWidget::LoginWidget(QWidget* parent)
    : BaseWidget(parent)
{
    setupUI();
    connectSignals();
    applyStyle();
}

// ─── BaseWidget interface ─────────────────────────────────────────────────

void LoginWidget::setupUI()
{
    // ── Layout principal: centrar la card ─────────────────────────────
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Fondo con gradiente (aplicado via stylesheet)
    setObjectName("loginPage");

    // ── Card central ──────────────────────────────────────────────────
    m_card = new QWidget(this);
    m_card->setObjectName("loginCard");
    m_card->setFixedWidth(400);

    // Sombra para la card
    auto* shadow = new QGraphicsDropShadowEffect(m_card);
    shadow->setBlurRadius(40);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(0, 0, 0, 120));
    m_card->setGraphicsEffect(shadow);

    QVBoxLayout* cardLayout = new QVBoxLayout(m_card);
    cardLayout->setContentsMargins(40, 40, 40, 40);
    cardLayout->setSpacing(16);

    // ── Logo / Título ─────────────────────────────────────────────────
    m_titleLabel = new QLabel("Carlens", m_card);
    m_titleLabel->setObjectName("loginTitle");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(28);
    titleFont.setWeight(QFont::Bold);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setStyleSheet("color: #58a6ff; letter-spacing: 4px;");

    m_subtitleLabel = new QLabel("Sistema de Vigilancia Vehicular", m_card);
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_subtitleLabel->setStyleSheet("color: #8b949e; font-size: 12px;");

    // ── Separador ─────────────────────────────────────────────────────
    QFrame* sep = new QFrame(m_card);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #21262d; margin: 8px 0;");

    // ── Campos de entrada ─────────────────────────────────────────────
    QLabel* userLabel = new QLabel("Usuario", m_card);
    userLabel->setStyleSheet("color: #8b949e; font-size: 12px; font-weight: 500;");
    m_usernameEdit = new QLineEdit(m_card);
    m_usernameEdit->setPlaceholderText("Ingrese su nombre de usuario");
    m_usernameEdit->setMinimumHeight(40);

    QLabel* passLabel = new QLabel("Contraseña", m_card);
    passLabel->setStyleSheet("color: #8b949e; font-size: 12px; font-weight: 500;");
    m_passwordEdit = new QLineEdit(m_card);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("Ingrese su contraseña");
    m_passwordEdit->setMinimumHeight(40);

    // ── Checkbox recordar + error ─────────────────────────────────────
    m_rememberCheck = new QCheckBox("Recordar sesión", m_card);

    m_errorLabel = new QLabel(m_card);
    m_errorLabel->setProperty("class", "error");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setVisible(false);
    m_errorLabel->setAlignment(Qt::AlignCenter);

    // ── Barra de progreso ─────────────────────────────────────────────
    m_progressBar = new QProgressBar(m_card);
    m_progressBar->setRange(0, 0);  // Indeterminado
    m_progressBar->setFixedHeight(3);
    m_progressBar->setVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar { border: none; background: #21262d; } "
        "QProgressBar::chunk { background: #1f6feb; }");

    // ── Botón Login ───────────────────────────────────────────────────
    m_loginBtn = new QPushButton("Iniciar Sesión", m_card);
    m_loginBtn->setProperty("class", "primary");
    m_loginBtn->setMinimumHeight(42);
    m_loginBtn->setDefault(true);
    QFont btnFont = m_loginBtn->font();
    btnFont.setWeight(QFont::DemiBold);
    m_loginBtn->setFont(btnFont);

    // ── Link a registro ───────────────────────────────────────────────
    QHBoxLayout* regLayout = new QHBoxLayout();
    regLayout->setContentsMargins(0, 0, 0, 0);
    QLabel* regLabel = new QLabel("¿No tienes cuenta?", m_card);
    regLabel->setStyleSheet("color: #8b949e; font-size: 12px;");
    m_registerLink = new QPushButton("Registrarse", m_card);
    m_registerLink->setFlat(true);
    m_registerLink->setCursor(Qt::PointingHandCursor);
    m_registerLink->setStyleSheet(
        "QPushButton { color: #58a6ff; background: transparent; border: none; "
        "font-size: 12px; padding: 0; } "
        "QPushButton:hover { color: #79c0ff; text-decoration: underline; }");
    regLayout->addStretch();
    regLayout->addWidget(regLabel);
    regLayout->addSpacing(4);
    regLayout->addWidget(m_registerLink);
    regLayout->addStretch();

    // ── Ensamblar card ────────────────────────────────────────────────
    cardLayout->addWidget(m_titleLabel);
    cardLayout->addWidget(m_subtitleLabel);
    cardLayout->addSpacing(4);
    cardLayout->addWidget(sep);
    cardLayout->addSpacing(4);
    cardLayout->addWidget(userLabel);
    cardLayout->addWidget(m_usernameEdit);
    cardLayout->addSpacing(4);
    cardLayout->addWidget(passLabel);
    cardLayout->addWidget(m_passwordEdit);
    cardLayout->addSpacing(4);
    cardLayout->addWidget(m_rememberCheck);
    cardLayout->addWidget(m_errorLabel);
    cardLayout->addWidget(m_progressBar);
    cardLayout->addSpacing(8);
    cardLayout->addWidget(m_loginBtn);
    cardLayout->addSpacing(4);
    cardLayout->addLayout(regLayout);

    // ── Centrar la card en el layout raíz ─────────────────────────────
    rootLayout->addStretch(2);
    QHBoxLayout* hCenter = new QHBoxLayout();
    hCenter->addStretch();
    hCenter->addWidget(m_card);
    hCenter->addStretch();
    rootLayout->addLayout(hCenter);
    rootLayout->addStretch(3);
}

void LoginWidget::connectSignals()
{
    connect(m_loginBtn,    &QPushButton::clicked,
            this,          &LoginWidget::onLoginClicked);

    connect(m_registerLink, &QPushButton::clicked,
            this,           [this]() {
                emit navigateTo(Carlens::AppScreen::Register);
            });

    // Conectar Enter en los campos
    connect(m_usernameEdit, &QLineEdit::returnPressed,
            m_loginBtn,     &QPushButton::click);
    connect(m_passwordEdit, &QLineEdit::returnPressed,
            m_loginBtn,     &QPushButton::click);

    // Conectar al UserManager
    auto* um = UserManager::instance();
    connect(um, &UserManager::loginSuccess, this, &LoginWidget::onLoginSuccess);
    connect(um, &UserManager::loginFailed,  this, &LoginWidget::onLoginFailed);
}

void LoginWidget::applyStyle()
{
    setStyleSheet(
        "#loginPage { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 #0d1117, stop:1 #161b22); }");
}

// ─── Slots ────────────────────────────────────────────────────────────────

void LoginWidget::onLoginClicked()
{
    clearError();
    if (!validateInputs()) return;
    setLoading(true);

    UserManager::instance()->login(
        m_usernameEdit->text().trimmed(),
        m_passwordEdit->text(),
        m_rememberCheck->isChecked()
    );
}

void LoginWidget::onLoginSuccess(const UserModel& user)
{
    Q_UNUSED(user)
    setLoading(false);
    emit navigateTo(Carlens::AppScreen::Dashboard);
}

void LoginWidget::onLoginFailed(const QString& reason)
{
    setLoading(false);
    showError(reason.isEmpty() ? "Credenciales inválidas." : reason);
}

// ─── Helpers ──────────────────────────────────────────────────────────────

bool LoginWidget::validateInputs() const
{
    if (m_usernameEdit->text().trimmed().isEmpty()) {
        const_cast<LoginWidget*>(this)->showError("El usuario no puede estar vacío.");
        return false;
    }
    if (m_passwordEdit->text().length() < 4) {
        const_cast<LoginWidget*>(this)->showError("La contraseña debe tener al menos 4 caracteres.");
        return false;
    }
    return true;
}

void LoginWidget::setLoading(bool loading)
{
    m_loginBtn->setEnabled(!loading);
    m_usernameEdit->setEnabled(!loading);
    m_passwordEdit->setEnabled(!loading);
    m_progressBar->setVisible(loading);
}

void LoginWidget::showError(const QString& msg)
{
    m_errorLabel->setText(msg);
    m_errorLabel->setVisible(true);
}

void LoginWidget::clearError()
{
    m_errorLabel->clear();
    m_errorLabel->setVisible(false);
}

void LoginWidget::reset()
{
    m_usernameEdit->clear();
    m_passwordEdit->clear();
    m_rememberCheck->setChecked(false);
    clearError();
    setLoading(false);
}
