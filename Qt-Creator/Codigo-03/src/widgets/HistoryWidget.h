#ifndef HISTORYWIDGET_H
#define HISTORYWIDGET_H

#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QList>

#include "base/BaseWidget.h"
#include "models/VehicleModel.h"

/**
 * @file HistoryWidget.h
 * @brief Módulo de historial de vehículos detectados. Hereda de BaseWidget.
 *
 * Funcionalidades:
 *   - Carga del historial desde HistoryManager.
 *   - Búsqueda/filtro de vehículos.
 *   - Renderizado de VehicleCardWidget por cada vehículo.
 *   - Paginación básica (Cargar más).
 *   - Refresco manual y automático.
 */
class HistoryWidget : public BaseWidget {
    Q_OBJECT

public:
    explicit HistoryWidget(QWidget* parent = nullptr);

    QString widgetName() const override { return "HistoryWidget"; }
    void    refresh();
    void    reset() override;

signals:
    void vehicleSelectedForAnalysis(int vehicleId);
    void statusMessage(const QString& msg);

protected:
    void setupUI()        override;
    void connectSignals() override;

private slots:
    void onHistoryLoaded(const QList<VehicleModel>& vehicles, int total);
    void onHistoryFailed(const QString& reason);
    void onFilterChanged(const QString& text);
    void onLoadMoreClicked();

private:
    void renderVehicles(const QList<VehicleModel>& vehicles);
    void clearVehicleCards();
    void setLoading(bool loading);

    QLineEdit*   m_searchEdit    = nullptr;
    QPushButton* m_refreshBtn    = nullptr;
    QLabel*      m_countLabel    = nullptr;
    QScrollArea* m_scrollArea    = nullptr;
    QWidget*     m_cardsContainer = nullptr;
    QVBoxLayout* m_cardsLayout   = nullptr;
    QPushButton* m_loadMoreBtn   = nullptr;
    QLabel*      m_emptyLabel    = nullptr;
    QLabel*      m_loadingLabel  = nullptr;

    QList<VehicleModel> m_allVehicles;
    int                 m_currentPage  = 1;
    int                 m_totalVehicles = 0;
    bool                m_isLoading    = false;
};

#endif // HISTORYWIDGET_H
