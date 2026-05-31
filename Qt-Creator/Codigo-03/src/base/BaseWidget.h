#ifndef BASEWIDGET_H
#define BASEWIDGET_H

#include <QWidget>
#include "utils/AppEnums.h"

/**
 * @file BaseWidget.h
 * @brief Clase base abstracta para todos los widgets de Carlens.
 *
 * Establece el contrato de los widgets de la aplicación:
 *   - setupUI():       construir la interfaz (layouts, widgets).
 *   - connectSignals(): conectar señales y slots internos.
 *   - applyStyle():     aplicar estilos QSS específicos del widget.
 *
 * Principios aplicados:
 *   - Clase abstracta con funciones virtuales puras.
 *   - Herencia: todos los widgets del proyecto la extienden.
 *   - Polimorfismo: se puede tratar cualquier widget como BaseWidget*.
 *   - Encapsulamiento: métodos de ciclo de vida protegidos.
 */
class BaseWidget : public QWidget {
    Q_OBJECT

public:
    explicit BaseWidget(QWidget* parent = nullptr);
    virtual ~BaseWidget() = default;

    /// Limpia el estado visual del widget (inputs, mensajes de error, etc.)
    virtual void reset();

    /// Nombre identificador del widget (usado en logs y navegación).
    virtual QString widgetName() const = 0;

protected:
    /// Construye los layouts, labels, botones, etc.
    virtual void setupUI() = 0;

    /// Conecta las señales y slots internos del widget.
    virtual void connectSignals() = 0;

    /// Aplica estilos QSS locales (complementa el tema global).
    virtual void applyStyle();

signals:
    /// Solicita a MainWindow que navegue a la pantalla indicada.
    void navigateTo(Carlens::AppScreen screen);
};

#endif // BASEWIDGET_H
