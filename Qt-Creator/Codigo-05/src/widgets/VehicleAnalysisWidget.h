#ifndef VEHICLEANALYSISWIDGET_H
#define VEHICLEANALYSISWIDGET_H

#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QStackedWidget>

#include "base/BaseWidget.h"
#include "models/VehicleModel.h"
#include "models/AnalysisModel.h"

/**
 * @file VehicleAnalysisWidget.h
 * @brief Módulo de análisis IA por vehículo. Hereda de BaseWidget.
 *
 * Muestra:
 *   - Snapshot del vehículo en grande.
 *   - Atributos detectados (tipo, marca, modelo, año, color, patente, confianza).
 *   - Análisis local disponible en QTextEdit.
 *
 * Integración con HistoryManager para carga de detalles y análisis.
 */
class VehicleAnalysisWidget : public BaseWidget {
    Q_OBJECT

public:
    explicit VehicleAnalysisWidget(QWidget* parent = nullptr);

    QString widgetName() const override { return "VehicleAnalysisWidget"; }
    void    loadVehicle(int vehicleId);
    void    reset() override;

protected:
    void setupUI()        override;
    void connectSignals() override;

private slots:
    void onVehicleDetailsLoaded(const VehicleModel& vehicle,
                                const AnalysisModel& analysis);
    void onVehicleDetailsFailed(const QString& reason);

private:
    void populateVehicle(const VehicleModel& v, const AnalysisModel& a);
    void loadSnapshot(const QString& url);
    void setLoading(bool loading);
    QString formatConfidence(double conf) const;

    // ── UI ─────────────────────────────────────────────────────────────
    QStackedWidget* m_stack           = nullptr;  // 0: empty, 1: content, 2: loading
    QLabel*         m_emptyLabel      = nullptr;
    QLabel*         m_loadingLabel    = nullptr;

    // Página de contenido
    QLabel*         m_snapshotLabel   = nullptr;
    QLabel*         m_typeLabel       = nullptr;
    QLabel*         m_brandLabel      = nullptr;
    QLabel*         m_modelLabel      = nullptr;
    QLabel*         m_yearLabel       = nullptr;
    QLabel*         m_colorLabel      = nullptr;
    QLabel*         m_plateLabel      = nullptr;
    QLabel*         m_confLabel       = nullptr;
    QLabel*         m_dateLabel       = nullptr;
    QTextEdit*      m_analysisText    = nullptr;

    int m_currentVehicleId = -1;
};

#endif // VEHICLEANALYSISWIDGET_H
