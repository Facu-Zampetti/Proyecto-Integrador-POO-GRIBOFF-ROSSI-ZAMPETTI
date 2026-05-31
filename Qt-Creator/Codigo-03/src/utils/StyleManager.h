#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QObject>
#include <QString>

/**
 * @file StyleManager.h
 * @brief Gestor de estilos QSS. Singleton.
 *
 * Centraliza la carga y provisión de stylesheets.
 * Permite a los widgets consultar colores del tema y
 * facilita un posible cambio de tema en el futuro.
 */
class StyleManager : public QObject {
    Q_OBJECT

public:
    static StyleManager* instance();

    /// Carga un archivo QSS desde resources o disco.
    QString loadStyleSheet(const QString& resourcePath) const;

    /// Retorna el stylesheet del tema oscuro embebido.
    static QString darkTheme();

    // ── Paleta del tema (para uso programático) ───────────────────────
    static constexpr const char* COLOR_BG_DARK      = "#0d1117";
    static constexpr const char* COLOR_BG_CARD      = "#161b22";
    static constexpr const char* COLOR_BG_SIDEBAR   = "#010409";
    static constexpr const char* COLOR_ACCENT       = "#58a6ff";
    static constexpr const char* COLOR_ACCENT_HOVER = "#79c0ff";
    static constexpr const char* COLOR_SUCCESS      = "#3fb950";
    static constexpr const char* COLOR_WARNING      = "#d29922";
    static constexpr const char* COLOR_ERROR        = "#f85149";
    static constexpr const char* COLOR_TEXT_PRIMARY  = "#e6edf3";
    static constexpr const char* COLOR_TEXT_SECONDARY= "#8b949e";
    static constexpr const char* COLOR_BORDER        = "#30363d";
    static constexpr const char* COLOR_INPUT_BG      = "#0d1117";
    static constexpr const char* COLOR_BUTTON_BG     = "#21262d";

private:
    explicit StyleManager(QObject* parent = nullptr);
    static StyleManager* s_instance;
};

#endif // STYLEMANAGER_H
