#include <QApplication>
#include <QFont>

#include "services/adminDB.h"
#include "managers/ConfigManager.h"
#include "managers/SessionManager.h"
#include "managers/UserManager.h"
#include "managers/VideoManager.h"
#include "managers/HistoryManager.h"
#include "utils/StyleManager.h"
#include "widgets/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // ── Metadatos de la aplicación ────────────────────────────────────
    QApplication::setApplicationName("Carlens");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("CarlensTeam");
    QApplication::setOrganizationDomain("carlens.app");

    // ── Fuente predeterminada ─────────────────────────────────────────
    QFont defaultFont("Segoe UI", 10);
    defaultFont.setHintingPreference(QFont::PreferFullHinting);
    QApplication::setFont(defaultFont);

    // ── Inicialización de servicios (orden de dependencia) ────────────
    // 1. adminDB — no tiene dependencias
    adminDB::instance()->initialize();

    // 2. ConfigManager — depende de adminDB
    ConfigManager::instance()->initialize();

    // 3. SessionManager — depende de ConfigManager y adminDB
    SessionManager::instance()->initialize();

    // 4. Managers de dominio — dependen de los anteriores
    UserManager::instance()->initialize();
    VideoManager::instance()->initialize();
    HistoryManager::instance()->initialize();

    // ── Hoja de estilos global ────────────────────────────────────────
    app.setStyleSheet(StyleManager::instance()->darkTheme());

    // ── Ventana principal ─────────────────────────────────────────────
    MainWindow window;
    window.setWindowTitle("Carlens — Vigilancia Inteligente de Vehículos");
    window.resize(1280, 800);
    window.setMinimumSize(960, 640);
    window.show();

    return app.exec();
}
