#include "widgets/LoginWidget.h"
#include "ui_LoginWidget.h"   // generado por uic a partir de LoginWidget.ui
#include "managers/UserManager.h"
#include <QEvent>
#include <QFont>
#include <QGraphicsDropShadowEffect>

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
    // Qt Designer + promote: ui->setupUi(this) construye toda la estructura
    // de widgets definida en LoginWidget.ui (generada por uic en ui_LoginWidget.h).
    // Los punteros de conveniencia se asignan desde m_ui para que el resto del
    // código de la clase no necesite cambiar.
    m_ui = new Ui::LoginWidget();
    m_ui->setupUi(this);

    // El .ui asigna objectName "LoginWidget" al root; lo sobreescribimos para
    // que el stylesheet global (#loginPage { ... }) aplique correctamente.
    setObjectName("loginPage");

    // ── Asignar punteros de conveniencia ──────────────────────────────
    m_card          = m_ui->m_card;
    m_titleLabel    = m_ui->m_titleLabel;
    m_subtitleLabel = m_ui->m_subtitleLabel;
    m_usernameEdit  = m_ui->m_usernameEdit;
    m_passwordEdit  = m_ui->m_passwordEdit;
    m_rememberCheck = m_ui->m_rememberCheck;
    m_errorLabel    = m_ui->m_errorLabel;
    m_progressBar   = m_ui->m_progressBar;
    m_loginBtn      = m_ui->m_loginBtn;
    m_registerLink  = m_ui->m_registerLink;

    // ── Propiedades dinámicas no editables en Qt Designer ────────────
    m_card->setObjectName("loginCard");

    auto* shadow = new QGraphicsDropShadowEffect(m_card);
    shadow->setBlurRadius(40);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(0, 0, 0, 120));
    m_card->setGraphicsEffect(shadow);

    m_errorLabel->setProperty("class", "error");
    m_errorLabel->setVisible(false);

    m_progressBar->setVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar { border: none; background: #21262d; } "
        "QProgressBar::chunk { background: #1f6feb; }");

    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(28);
    titleFont.setWeight(QFont::Bold);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setStyleSheet("color: #58a6ff; letter-spacing: 4px;");

    m_subtitleLabel->setStyleSheet("color: #8b949e; font-size: 12px;");

    m_ui->m_usernameLabel->setStyleSheet("color: #8b949e; font-size: 12px; font-weight: 500;");
    m_ui->m_passwordLabel->setStyleSheet("color: #8b949e; font-size: 12px; font-weight: 500;");
    m_ui->m_regHintLabel->setStyleSheet("color: #8b949e; font-size: 12px;");
    m_ui->separator->setStyleSheet("color: #21262d; margin: 8px 0;");

    m_loginBtn->setProperty("class", "primary");
    QFont btnFont = m_loginBtn->font();
    btnFont.setWeight(QFont::DemiBold);
    m_loginBtn->setFont(btnFont);

    m_registerLink->setCursor(Qt::PointingHandCursor);
    m_registerLink->setStyleSheet(
        "QPushButton { color: #58a6ff; background: transparent; border: none; "
        "font-size: 12px; padding: 0; } "
        "QPushButton:hover { color: #79c0ff; text-decoration: underline; }");
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

    // Limpiar error cuando el usuario enfoca cualquiera de los dos campos.
    m_usernameEdit->installEventFilter(this);
    m_passwordEdit->installEventFilter(this);
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

bool LoginWidget::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == m_usernameEdit || watched == m_passwordEdit)
        && event->type() == QEvent::FocusIn)
        clearError();
    return BaseWidget::eventFilter(watched, event);
}

void LoginWidget::reset()
{
    m_usernameEdit->clear();
    m_passwordEdit->clear();
    m_rememberCheck->setChecked(false);
    clearError();
    setLoading(false);
}
