#include "widgets/RegisterWidget.h"
#include "managers/UserManager.h"
#include <QFrame>
#include <QRegularExpression>
#include <QGraphicsDropShadowEffect>
#include <QTimer>
#include <QStyle>

RegisterWidget::RegisterWidget(QWidget* parent)
    : BaseWidget(parent)
{
    setupUI();
    connectSignals();
    applyStyle();
}

void RegisterWidget::setupUI()
{
    setObjectName("registerPage");

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    m_card = new QWidget(this);
    m_card->setObjectName("loginCard");
    m_card->setMinimumWidth(340);
    m_card->setMaximumWidth(480);

    auto* shadow = new QGraphicsDropShadowEffect(m_card);
    shadow->setBlurRadius(40);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(0, 0, 0, 120));
    m_card->setGraphicsEffect(shadow);

    QVBoxLayout* cardLayout = new QVBoxLayout(m_card);
    cardLayout->setContentsMargins(40, 36, 40, 36);
    cardLayout->setSpacing(12);

    // ── Título ────────────────────────────────────────────────────────
    m_titleLabel = new QLabel("Crear Cuenta", m_card);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    QFont f = m_titleLabel->font();
    f.setPointSize(22);
    f.setWeight(QFont::Bold);
    m_titleLabel->setFont(f);
    m_titleLabel->setStyleSheet("color: #58a6ff;");

    QLabel* sub = new QLabel("Carlens — Vigilancia Vehicular", m_card);
    sub->setAlignment(Qt::AlignCenter);
    sub->setStyleSheet("color: #8b949e; font-size: 12px;");

    QFrame* sep = new QFrame(m_card);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #21262d;");

    // ── Campos ────────────────────────────────────────────────────────
    auto makeLabel = [&](const QString& text) {
        QLabel* l = new QLabel(text, m_card);
        l->setStyleSheet("color: #8b949e; font-size: 12px; font-weight: 500;");
        return l;
    };

    m_usernameEdit = new QLineEdit(m_card);
    m_usernameEdit->setPlaceholderText("Nombre de usuario (mín. 3 caracteres)");
    m_usernameEdit->setMinimumHeight(38);

    m_emailEdit = new QLineEdit(m_card);
    m_emailEdit->setPlaceholderText("correo@ejemplo.com");
    m_emailEdit->setMinimumHeight(38);

    m_passwordEdit = new QLineEdit(m_card);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("Mínimo 6 caracteres");
    m_passwordEdit->setMinimumHeight(38);

    m_strengthLabel = new QLabel(m_card);
    m_strengthLabel->setStyleSheet("font-size: 12px;");
    m_strengthLabel->setVisible(false);

    m_confirmEdit = new QLineEdit(m_card);
    m_confirmEdit->setEchoMode(QLineEdit::Password);
    m_confirmEdit->setPlaceholderText("Repita la contraseña");
    m_confirmEdit->setMinimumHeight(38);

    // ── Mensajes ──────────────────────────────────────────────────────
    m_messageLabel = new QLabel(m_card);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setVisible(false);

    m_progressBar = new QProgressBar(m_card);
    m_progressBar->setRange(0, 0);
    m_progressBar->setFixedHeight(3);
    m_progressBar->setVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar { border: none; background: #21262d; } "
        "QProgressBar::chunk { background: #1f6feb; }");

    // ── Botón ─────────────────────────────────────────────────────────
    m_registerBtn = new QPushButton("Crear Cuenta", m_card);
    m_registerBtn->setProperty("class", "primary");
    m_registerBtn->setMinimumHeight(42);
    QFont bf = m_registerBtn->font();
    bf.setWeight(QFont::DemiBold);
    m_registerBtn->setFont(bf);

    // ── Link a login ──────────────────────────────────────────────────
    QHBoxLayout* loginRow = new QHBoxLayout();
    QLabel* hasAccount = new QLabel("¿Ya tienes cuenta?", m_card);
    hasAccount->setStyleSheet("color: #8b949e; font-size: 12px;");
    m_loginLink = new QPushButton("Iniciar Sesión", m_card);
    m_loginLink->setFlat(true);
    m_loginLink->setCursor(Qt::PointingHandCursor);
    m_loginLink->setStyleSheet(
        "QPushButton { color: #58a6ff; background: transparent; border: none; "
        "font-size: 12px; padding: 0; } "
        "QPushButton:hover { color: #79c0ff; text-decoration: underline; }");
    loginRow->addStretch();
    loginRow->addWidget(hasAccount);
    loginRow->addSpacing(4);
    loginRow->addWidget(m_loginLink);
    loginRow->addStretch();

    // ── Ensamblar ─────────────────────────────────────────────────────
    cardLayout->addWidget(m_titleLabel);
    cardLayout->addWidget(sub);
    cardLayout->addWidget(sep);
    cardLayout->addWidget(makeLabel("Usuario"));
    cardLayout->addWidget(m_usernameEdit);
    cardLayout->addWidget(makeLabel("Email"));
    cardLayout->addWidget(m_emailEdit);
    cardLayout->addWidget(makeLabel("Contraseña"));
    cardLayout->addWidget(m_passwordEdit);
    cardLayout->addWidget(m_strengthLabel);
    cardLayout->addWidget(makeLabel("Confirmar Contraseña"));
    cardLayout->addWidget(m_confirmEdit);
    cardLayout->addWidget(m_messageLabel);
    cardLayout->addWidget(m_progressBar);
    cardLayout->addSpacing(4);
    cardLayout->addWidget(m_registerBtn);
    cardLayout->addLayout(loginRow);

    // ── Centrar ───────────────────────────────────────────────────────
    rootLayout->addStretch(1);
    QHBoxLayout* hc = new QHBoxLayout();
    hc->addStretch();
    hc->addWidget(m_card);
    hc->addStretch();
    rootLayout->addLayout(hc);
    rootLayout->addStretch(2);
}

void RegisterWidget::connectSignals()
{
    connect(m_registerBtn, &QPushButton::clicked,
            this,          &RegisterWidget::onRegisterClicked);

    connect(m_loginLink, &QPushButton::clicked,
            this,        [this]() { emit navigateTo(Carlens::AppScreen::Login); });

    connect(m_passwordEdit, &QLineEdit::textChanged,
            this,           &RegisterWidget::onPasswordChanged);

    connect(m_confirmEdit, &QLineEdit::returnPressed,
            m_registerBtn, &QPushButton::click);

    auto* um = UserManager::instance();
    connect(um, &UserManager::registerSuccess, this, &RegisterWidget::onRegisterSuccess);
    connect(um, &UserManager::registerFailed,  this, &RegisterWidget::onRegisterFailed);
}

void RegisterWidget::applyStyle()
{
    setStyleSheet(
        "#registerPage { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 #0d1117, stop:1 #161b22); }");
}

// ─── Slots ────────────────────────────────────────────────────────────────

void RegisterWidget::onRegisterClicked()
{
    clearMessages();
    if (!validateInputs()) return;
    setLoading(true);
    UserManager::instance()->registerUser(
        m_usernameEdit->text().trimmed(),
        m_emailEdit->text().trimmed(),
        m_passwordEdit->text()
    );
}

void RegisterWidget::onRegisterSuccess(const QString& message)
{
    setLoading(false);
    showSuccess(message.isEmpty() ? "¡Cuenta creada! Ya puedes iniciar sesión." : message);
    QTimer::singleShot(2000, this, [this]() {
        emit navigateTo(Carlens::AppScreen::Login);
    });
}

void RegisterWidget::onRegisterFailed(const QString& reason)
{
    setLoading(false);
    showError(reason.isEmpty() ? "No se pudo crear la cuenta." : reason);
}

void RegisterWidget::onPasswordChanged(const QString& text)
{
    int len = text.length();
    if (len == 0) {
        m_strengthLabel->setVisible(false);
        return;
    }
    m_strengthLabel->setVisible(true);
    if (len < 6)
        m_strengthLabel->setStyleSheet("color: #f85149; font-size: 12px;"),
        m_strengthLabel->setText("Contraseña débil");
    else if (len < 10)
        m_strengthLabel->setStyleSheet("color: #d29922; font-size: 12px;"),
        m_strengthLabel->setText("Contraseña media");
    else
        m_strengthLabel->setStyleSheet("color: #3fb950; font-size: 12px;"),
        m_strengthLabel->setText("Contraseña fuerte");
}

// ─── Helpers ──────────────────────────────────────────────────────────────

bool RegisterWidget::validateInputs() const
{
    auto* self = const_cast<RegisterWidget*>(this);
    if (m_usernameEdit->text().trimmed().length() < 3) {
        self->showError("El usuario debe tener al menos 3 caracteres.");
        return false;
    }
    if (!isValidEmail(m_emailEdit->text().trimmed())) {
        self->showError("Ingrese un email válido.");
        return false;
    }
    if (m_passwordEdit->text().length() < 6) {
        self->showError("La contraseña debe tener al menos 6 caracteres.");
        return false;
    }
    if (m_passwordEdit->text() != m_confirmEdit->text()) {
        self->showError("Las contraseñas no coinciden.");
        return false;
    }
    return true;
}

bool RegisterWidget::isValidEmail(const QString& email) const
{
    static const QRegularExpression emailRx(
        R"(^[a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,}$)");
    return emailRx.match(email).hasMatch();
}

void RegisterWidget::setLoading(bool loading)
{
    m_registerBtn->setEnabled(!loading);
    m_usernameEdit->setEnabled(!loading);
    m_emailEdit->setEnabled(!loading);
    m_passwordEdit->setEnabled(!loading);
    m_confirmEdit->setEnabled(!loading);
    m_progressBar->setVisible(loading);
}

void RegisterWidget::showError(const QString& msg)
{
    m_messageLabel->setText(msg);
    m_messageLabel->setProperty("class", "error");
    m_messageLabel->style()->unpolish(m_messageLabel);
    m_messageLabel->style()->polish(m_messageLabel);
    m_messageLabel->setVisible(true);
}

void RegisterWidget::showSuccess(const QString& msg)
{
    m_messageLabel->setText(msg);
    m_messageLabel->setProperty("class", "success");
    m_messageLabel->style()->unpolish(m_messageLabel);
    m_messageLabel->style()->polish(m_messageLabel);
    m_messageLabel->setVisible(true);
}

void RegisterWidget::clearMessages()
{
    m_messageLabel->clear();
    m_messageLabel->setVisible(false);
}

void RegisterWidget::reset()
{
    m_usernameEdit->clear();
    m_emailEdit->clear();
    m_passwordEdit->clear();
    m_confirmEdit->clear();
    m_strengthLabel->setVisible(false);
    clearMessages();
    setLoading(false);
}
