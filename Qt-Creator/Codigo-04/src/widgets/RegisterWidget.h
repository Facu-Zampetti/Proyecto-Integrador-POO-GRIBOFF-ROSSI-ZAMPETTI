#ifndef REGISTERWIDGET_H
#define REGISTERWIDGET_H

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>

#include "base/BaseWidget.h"

/**
 * @file RegisterWidget.h
 * @brief Pantalla de Registro de usuario. Hereda de BaseWidget.
 *
 * Implementa:
 *   - Formulario: username, email, password, confirmación.
 *   - Validaciones en cliente (longitud, coincidencia, formato email).
 *   - Indicadores de fortaleza de contraseña.
 *   - Comunicación con backend via UserManager.
 */
class RegisterWidget : public BaseWidget {
    Q_OBJECT

public:
    explicit RegisterWidget(QWidget* parent = nullptr);

    QString widgetName() const override { return "RegisterWidget"; }
    void    reset() override;

protected:
    void setupUI()        override;
    void connectSignals() override;
    void applyStyle()     override;

private slots:
    void onRegisterClicked();
    void onRegisterSuccess(const QString& message);
    void onRegisterFailed(const QString& reason);
    void onPasswordChanged(const QString& text);

private:
    bool validateInputs() const;
    void setLoading(bool loading);
    void showError(const QString& msg);
    void showSuccess(const QString& msg);
    void clearMessages();
    bool isValidEmail(const QString& email) const;

    // ── UI ─────────────────────────────────────────────────────────────
    QWidget*     m_card            = nullptr;
    QLabel*      m_titleLabel      = nullptr;
    QLabel*      m_messageLabel    = nullptr;
    QLineEdit*   m_usernameEdit    = nullptr;
    QLineEdit*   m_emailEdit       = nullptr;
    QLineEdit*   m_passwordEdit    = nullptr;
    QLineEdit*   m_confirmEdit     = nullptr;
    QLabel*      m_strengthLabel   = nullptr;
    QPushButton* m_registerBtn     = nullptr;
    QPushButton* m_loginLink       = nullptr;
    QProgressBar* m_progressBar    = nullptr;
};

#endif // REGISTERWIDGET_H
