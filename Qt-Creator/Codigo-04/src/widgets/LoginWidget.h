#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QProgressBar>

#include "base/BaseWidget.h"
#include "models/UserModel.h"

/**
 * @file LoginWidget.h
 * @brief Pantalla de Login. Hereda de BaseWidget.
 *
 * Implementa:
 *   - Formulario con usuario y contraseña.
 *   - Checkbox "Recordar sesión".
 *   - Validación en cliente antes de enviar.
 *   - Indicador de carga durante la petición HTTP.
 *   - Navegación hacia Register o Dashboard.
 */
class LoginWidget : public BaseWidget {
    Q_OBJECT

public:
    explicit LoginWidget(QWidget* parent = nullptr);

    QString widgetName() const override { return "LoginWidget"; }
    void    reset() override;

protected:
    void setupUI()       override;
    void connectSignals() override;
    void applyStyle()     override;

private slots:
    void onLoginClicked();
    void onLoginSuccess(const UserModel& user);
    void onLoginFailed(const QString& reason);

private:
    bool validateInputs() const;
    void setLoading(bool loading);
    void showError(const QString& msg);
    void clearError();

    // ── UI ─────────────────────────────────────────────────────────────
    QLabel*      m_titleLabel    = nullptr;
    QLabel*      m_subtitleLabel = nullptr;
    QLabel*      m_errorLabel    = nullptr;
    QLineEdit*   m_usernameEdit  = nullptr;
    QLineEdit*   m_passwordEdit  = nullptr;
    QCheckBox*   m_rememberCheck = nullptr;
    QPushButton* m_loginBtn      = nullptr;
    QPushButton* m_registerLink  = nullptr;
    QProgressBar* m_progressBar  = nullptr;
    QWidget*     m_card          = nullptr;
};

#endif // LOGINWIDGET_H
