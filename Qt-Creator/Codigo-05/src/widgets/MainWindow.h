#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCloseEvent>
#include <QKeyEvent>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "utils/AppEnums.h"
#include "models/UserModel.h"

class LoginWidget;
class RegisterWidget;
class DashboardWidget;

/**
 * @file MainWindow.h
 * @brief Ventana principal de la aplicación. Deriva de QWidget (NO QMainWindow).
 *
 * Actúa como contenedor raíz y controlador de navegación:
 *   - Administra un QStackedWidget con las 3 pantallas principales.
 *   - Escucha señales de navegación de los widgets hijos.
 *   - Orquesta el ciclo de vida de la sesión.
 *
 * Principios: Separación de responsabilidades, composición, señales/slots.
 */
class MainWindow : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

public slots:
    void navigateTo(Carlens::AppScreen screen);

protected:
    /// Diálogo de confirmación + shutdown ordenado de managers al cerrar.
    void closeEvent(QCloseEvent* event) override;

    /// Atajos de teclado globales: F5 (historial), Ctrl+Q (salir).
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupUI();
    void connectSignals();
    void checkAutoLogin();

    QVBoxLayout*     m_mainLayout    = nullptr;
    QStackedWidget*  m_stackedWidget = nullptr;

    LoginWidget*     m_loginWidget    = nullptr;
    RegisterWidget*  m_registerWidget = nullptr;
    DashboardWidget* m_dashboardWidget = nullptr;

    // Índices en el QStackedWidget
    int m_loginIndex    = 0;
    int m_registerIndex = 1;
    int m_dashboardIndex = 2;
};

#endif // MAINWINDOW_H
