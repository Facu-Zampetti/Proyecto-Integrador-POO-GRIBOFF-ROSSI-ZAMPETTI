#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

#include "base/BaseWidget.h"
#include "models/UserModel.h"

// Forward declaration del namespace generado por uic desde LoginWidget.ui.
// El archivo ui_LoginWidget.h es generado automáticamente por qmake/uic.
namespace Ui { class LoginWidget; }

/**
 * @file LoginWidget.h
 * @brief Pantalla de Login. Hereda de BaseWidget.
 *
 * Implementa:
 *   - Formulario con usuario y contraseña (estructura definida en LoginWidget.ui).
 *   - Checkbox "Recordar sesión".
 *   - Validación en cliente antes de enviar.
 *   - Indicador de carga (QProgressBar indeterminado).
 *   - Navegación hacia Register o Dashboard.
 *   - Qt Designer + promote: setupUI() llama ui->setupUi(this); uic genera
 *     ui_LoginWidget.h con Ui::LoginWidget que encapsula todos los widgets.
 *   - eventFilter: limpia el error al enfocar cualquiera de los campos.
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

    /// Intercepta FocusIn en los campos para limpiar el mensaje de error.
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onLoginClicked();
    void onLoginSuccess(const UserModel& user);
    void onLoginFailed(const QString& reason);

private:
    bool validateInputs() const;
    void setLoading(bool loading);
    void showError(const QString& msg);
    void clearError();

    // ── Qt Designer form (generado por uic desde LoginWidget.ui) ─────
    Ui::LoginWidget* m_ui = nullptr;  ///< Puntero al form generado.

    // ── UI — punteros de conveniencia apuntando a m_ui->... ──────────
    QLabel*       m_titleLabel    = nullptr;
    QLabel*       m_subtitleLabel = nullptr;
    QLabel*       m_errorLabel    = nullptr;
    QLineEdit*    m_usernameEdit  = nullptr;
    QLineEdit*    m_passwordEdit  = nullptr;
    QCheckBox*    m_rememberCheck = nullptr;
    QPushButton*  m_loginBtn      = nullptr;
    QPushButton*  m_registerLink  = nullptr;
    QProgressBar* m_progressBar   = nullptr;
    QWidget*      m_card          = nullptr;
};

#endif // LOGINWIDGET_H
