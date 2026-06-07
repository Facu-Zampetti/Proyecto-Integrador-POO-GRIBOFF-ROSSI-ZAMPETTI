#include "widgets/MainWindow.h"
#include "widgets/LoginWidget.h"
#include "widgets/RegisterWidget.h"
#include "widgets/DashboardWidget.h"
#include "managers/SessionManager.h"
#include "managers/UserManager.h"
#include "managers/VideoManager.h"
#include "managers/HistoryManager.h"
#include "managers/ConfigManager.h"
#include "services/adminDB.h"
#include <QApplication>
#include <QMessageBox>
#include <QScreen>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    connectSignals();
    checkAutoLogin();
}

void MainWindow::setupUI()
{
    setWindowTitle("Carlens — Sistema de Vigilancia Vehicular");
    setMinimumSize(1000, 640);
    setObjectName("mainWindow");

    // Centrar en pantalla
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect geo = screen->availableGeometry();
        resize(qMin(1400, geo.width() - 80),
               qMin(860, geo.height() - 80));
        move((geo.width()  - width())  / 2,
             (geo.height() - height()) / 2);
    }

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_stackedWidget = new QStackedWidget(this);
    m_mainLayout->addWidget(m_stackedWidget);

    // Crear widgets de pantalla
    m_loginWidget     = new LoginWidget(this);
    m_registerWidget  = new RegisterWidget(this);
    m_dashboardWidget = new DashboardWidget(this);

    m_loginIndex     = m_stackedWidget->addWidget(m_loginWidget);
    m_registerIndex  = m_stackedWidget->addWidget(m_registerWidget);
    m_dashboardIndex = m_stackedWidget->addWidget(m_dashboardWidget);

    // Iniciar en Login
    m_stackedWidget->setCurrentIndex(m_loginIndex);
}

void MainWindow::connectSignals()
{
    // Navegación desde LoginWidget
    connect(m_loginWidget, &LoginWidget::navigateTo,
            this,          &MainWindow::navigateTo);

    // Navegación desde RegisterWidget
    connect(m_registerWidget, &RegisterWidget::navigateTo,
            this,             &MainWindow::navigateTo);

    // Navegación desde DashboardWidget
    connect(m_dashboardWidget, &DashboardWidget::navigateTo,
            this,              &MainWindow::navigateTo);

    // Cuando el usuario hace logout desde el dashboard
    connect(m_dashboardWidget, &DashboardWidget::logoutRequested,
            this, [this]() {
                m_dashboardWidget->reset();
                UserManager::instance()->logout();
                navigateTo(Carlens::AppScreen::Login);
                m_loginWidget->reset();
            });
}

void MainWindow::checkAutoLogin()
{
    if (SessionManager::instance()->tryRestoreSession()) {
        const UserModel& user = SessionManager::instance()->currentUser();
        m_dashboardWidget->setCurrentUser(user);
        navigateTo(Carlens::AppScreen::Dashboard);
    }
}

// ── Event overrides ───────────────────────────────────────────────────────

void MainWindow::closeEvent(QCloseEvent* event)
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Cerrar Carlens",
        "¿Desea salir?\nSe cerrarán todos los procesos en ejecución.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        // Shutdown en orden inverso al startup definido en main.cpp.
        HistoryManager::instance()->shutdown();
        VideoManager::instance()->shutdown();
        UserManager::instance()->shutdown();
        SessionManager::instance()->shutdown();
        ConfigManager::instance()->shutdown();
        adminDB::instance()->shutdown();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_F5:
        // Atajo global: ir al historial y refrescarlo.
        if (m_stackedWidget->currentIndex() == m_dashboardIndex)
            m_dashboardWidget->showSection(Carlens::DashboardSection::History);
        break;
    case Qt::Key_Q:
        if (event->modifiers() & Qt::ControlModifier)
            close();    // dispara closeEvent con diálogo de confirmación
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

void MainWindow::navigateTo(Carlens::AppScreen screen)
{
    switch (screen) {
    case Carlens::AppScreen::Login:
        m_stackedWidget->setCurrentIndex(m_loginIndex);
        break;
    case Carlens::AppScreen::Register:
        m_stackedWidget->setCurrentIndex(m_registerIndex);
        break;
    case Carlens::AppScreen::Dashboard:
        if (SessionManager::instance()->isLoggedIn()) {
            m_dashboardWidget->setCurrentUser(
                SessionManager::instance()->currentUser());
            m_stackedWidget->setCurrentIndex(m_dashboardIndex);
        }
        break;
    }
}
