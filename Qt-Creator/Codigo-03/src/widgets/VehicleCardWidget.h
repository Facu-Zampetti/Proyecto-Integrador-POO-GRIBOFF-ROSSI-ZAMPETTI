#ifndef VEHICLECARDWIDGET_H
#define VEHICLECARDWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QNetworkReply>

#include "models/VehicleModel.h"

/**
 * @file VehicleCardWidget.h
 * @brief Tarjeta de presentación de un vehículo en el historial.
 *
 * Muestra:
 *   - Thumbnail del snapshot (cargado via HTTP).
 *   - Tipo de vehículo, fecha, confianza.
 *   - Atributos básicos (marca, modelo, color si disponibles).
 *   - Botón "Ver Análisis".
 *
 * Clase derivada de QWidget. Principio: Reutilización, composición.
 */
class VehicleCardWidget : public QWidget {
    Q_OBJECT

public:
    explicit VehicleCardWidget(const VehicleModel& vehicle,
                               QWidget* parent = nullptr);

    int vehicleId() const { return m_vehicle.id(); }

signals:
    void analysisRequested(int vehicleId);

private:
    void setupUI();
    void loadThumbnail();

    QLabel*      m_thumbnail     = nullptr;
    QLabel*      m_typeLabel     = nullptr;
    QLabel*      m_dateLabel     = nullptr;
    QLabel*      m_detailsLabel  = nullptr;
    QLabel*      m_confidenceLabel = nullptr;
    QPushButton* m_analysisBtn   = nullptr;

    VehicleModel m_vehicle;
};

#endif // VEHICLECARDWIDGET_H
