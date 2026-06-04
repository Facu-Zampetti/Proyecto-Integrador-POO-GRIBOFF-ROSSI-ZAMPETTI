#include "base/BaseWidget.h"

BaseWidget::BaseWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
}

void BaseWidget::reset()
{
    // Implementación por defecto vacía.
    // Las subclases pueden sobrescribir para limpiar su estado visual.
}

void BaseWidget::applyStyle()
{
    // Implementación por defecto vacía.
    // Las subclases pueden sobrescribir para aplicar estilos locales.
}
