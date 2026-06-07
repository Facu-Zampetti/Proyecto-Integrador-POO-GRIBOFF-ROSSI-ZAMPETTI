#include "widgets/HistoryWidget.h"
#include "widgets/VehicleCardWidget.h"
#include "managers/HistoryManager.h"
#include <QTimer>
#include <algorithm>

HistoryWidget::HistoryWidget(QWidget* parent)
    : BaseWidget(parent)
{
    setupUI();
    connectSignals();
}

void HistoryWidget::setupUI()
{
    setObjectName("historyWidget");
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(32, 32, 32, 32);
    rootLayout->setSpacing(16);

    // ── Encabezado ────────────────────────────────────────────────────
    QLabel* sectionTitle = new QLabel("Historial de Vehículos", this);
    QFont tf = sectionTitle->font();
    tf.setPointSize(18);
    tf.setWeight(QFont::DemiBold);
    sectionTitle->setFont(tf);
    sectionTitle->setStyleSheet("color: #e6edf3;");

    // ── Barra de búsqueda y controles ─────────────────────────────────
    QHBoxLayout* toolRow = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Buscar por tipo, marca, color, patente…");
    m_searchEdit->setMinimumHeight(36);
    m_searchEdit->setMinimumWidth(300);

    m_refreshBtn = new QPushButton("↻  Actualizar", this);
    m_refreshBtn->setMinimumHeight(36);
    m_refreshBtn->setMinimumWidth(120);
    m_refreshBtn->setCursor(Qt::PointingHandCursor);

    m_countLabel = new QLabel("0 vehículos", this);
    m_countLabel->setStyleSheet("color: #8b949e; font-size: 12px;");

    toolRow->addWidget(m_searchEdit, 1);
    toolRow->addWidget(m_refreshBtn);
    toolRow->addStretch();
    toolRow->addWidget(m_countLabel);

    // ── Estado de carga ───────────────────────────────────────────────
    m_loadingLabel = new QLabel("Cargando historial…", this);
    m_loadingLabel->setAlignment(Qt::AlignCenter);
    m_loadingLabel->setStyleSheet("color: #8b949e; font-size: 13px;");
    m_loadingLabel->setVisible(false);

    // ── Área de scroll con tarjetas ───────────────────────────────────
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");

    m_cardsContainer = new QWidget();
    m_cardsContainer->setStyleSheet("background: transparent;");
    m_cardsLayout = new QVBoxLayout(m_cardsContainer);
    m_cardsLayout->setContentsMargins(0, 0, 8, 0);
    m_cardsLayout->setSpacing(8);

    m_emptyLabel = new QLabel(
        "Sin resultados\n\nNo hay vehículos en el historial.\n"
        "Cargue un video o conecte una cámara RTSP para comenzar.",
        m_cardsContainer);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("color: #484f58; font-size: 13px;");
    m_emptyLabel->setWordWrap(true);
    m_emptyLabel->setVisible(false);
    m_cardsLayout->addWidget(m_emptyLabel);
    m_cardsLayout->addStretch();

    m_scrollArea->setWidget(m_cardsContainer);

    // ── Botón cargar más ──────────────────────────────────────────────
    m_loadMoreBtn = new QPushButton("Cargar más…", this);
    m_loadMoreBtn->setMinimumHeight(36);
    m_loadMoreBtn->setVisible(false);

    // ── Ensamblar ─────────────────────────────────────────────────────
    rootLayout->addWidget(sectionTitle);
    rootLayout->addLayout(toolRow);
    rootLayout->addWidget(m_loadingLabel);
    rootLayout->addWidget(m_scrollArea, 1);
    rootLayout->addWidget(m_loadMoreBtn, 0, Qt::AlignCenter);
}

void HistoryWidget::connectSignals()
{
    connect(m_refreshBtn, &QPushButton::clicked,
            this,         &HistoryWidget::refresh);

    connect(m_loadMoreBtn, &QPushButton::clicked,
            this,          &HistoryWidget::onLoadMoreClicked);

    connect(m_searchEdit, &QLineEdit::textChanged,
            this,         &HistoryWidget::onFilterChanged);

    auto* hm = HistoryManager::instance();
    connect(hm, &HistoryManager::historyLoaded,
            this, &HistoryWidget::onHistoryLoaded);
    connect(hm, &HistoryManager::historyLoadFailed,
            this, &HistoryWidget::onHistoryFailed);
}

// ─── Slots ────────────────────────────────────────────────────────────────

void HistoryWidget::refresh()
{
    m_currentPage = 1;
    setLoading(true);
    HistoryManager::instance()->loadHistory(1);
    emit statusMessage("Actualizando historial…");
}

void HistoryWidget::onHistoryLoaded(const QList<VehicleModel>& vehicles, int total)
{
    setLoading(false);
    m_totalVehicles = total;

    // Acumular páginas: la primera reemplaza, las siguientes se suman.
    // Esto asegura que "Cargar más" muestre todo lo acumulado, no solo el último lote.
    if (m_currentPage == 1)
        m_allVehicles = vehicles;
    else
        m_allVehicles.append(vehicles);

    // Deduplicar por id (operator==) y ordenar por fecha descendente (operator<).
    auto last = std::unique(m_allVehicles.begin(), m_allVehicles.end());
    m_allVehicles.erase(last, m_allVehicles.end());
    std::sort(m_allVehicles.begin(), m_allVehicles.end(),
              [](const VehicleModel& a, const VehicleModel& b) { return b < a; });

    m_countLabel->setText(QString("%1 vehículo%2")
                          .arg(total)
                          .arg(total != 1 ? "s" : ""));

    renderVehicles(m_allVehicles);

    // hasMore y "restantes" se calculan sobre lo acumulado, no sobre el lote recibido.
    const bool hasMore = (m_allVehicles.size() < total);
    m_loadMoreBtn->setVisible(hasMore);
    if (hasMore)
        m_loadMoreBtn->setText(
            QString("Cargar más (%1 restantes)")
            .arg(total - m_allVehicles.size()));

    emit statusMessage(QString("Historial actualizado: %1 vehículos.").arg(total));
}

void HistoryWidget::onHistoryFailed(const QString& reason)
{
    setLoading(false);
    m_emptyLabel->setText("No se pudo cargar el historial\n" + reason);
    m_emptyLabel->setVisible(true);
}

void HistoryWidget::onFilterChanged(const QString& text)
{
    if (text.isEmpty()) {
        renderVehicles(m_allVehicles);
        return;
    }

    const QString filter = text.toLower();
    QList<VehicleModel> filtered;
    for (const VehicleModel& v : m_allVehicles) {
        if (VehicleModel::vehicleTypeToString(v.vehicleType()).toLower().contains(filter) ||
            v.brand().toLower().contains(filter)   ||
            v.model().toLower().contains(filter)   ||
            v.yearRange().toLower().contains(filter) ||
            v.color().toLower().contains(filter)   ||
            v.licensePlate().toLower().contains(filter)) {
            filtered.append(v);
        }
    }
    renderVehicles(filtered);
}

void HistoryWidget::onLoadMoreClicked()
{
    ++m_currentPage;
    setLoading(true);
    HistoryManager::instance()->loadHistory(m_currentPage);
}

// ─── Helpers ──────────────────────────────────────────────────────────────

void HistoryWidget::renderVehicles(const QList<VehicleModel>& vehicles)
{
    clearVehicleCards();

    if (vehicles.isEmpty()) {
        m_emptyLabel->setText(
            "Sin resultados\n\nNo hay vehículos que coincidan con la búsqueda.");
        m_emptyLabel->setVisible(true);
        return;
    }

    m_emptyLabel->setVisible(false);

    for (const VehicleModel& v : vehicles) {
        VehicleCardWidget* card = new VehicleCardWidget(v, m_cardsContainer);
        connect(card, &VehicleCardWidget::analysisRequested,
                this, &HistoryWidget::vehicleSelectedForAnalysis);
        m_cardsLayout->insertWidget(m_cardsLayout->count() - 1, card);
    }
}

void HistoryWidget::clearVehicleCards()
{
    // Eliminar todos los VehicleCardWidget del layout
    QLayoutItem* item;
    while ((item = m_cardsLayout->takeAt(0)) != nullptr) {
        if (item->widget() && item->widget() != m_emptyLabel)
            item->widget()->deleteLater();
        delete item;
    }
    m_cardsLayout->addWidget(m_emptyLabel);
    m_cardsLayout->addStretch();
}

void HistoryWidget::setLoading(bool loading)
{
    m_isLoading     = loading;
    m_loadingLabel->setVisible(loading);
    m_scrollArea->setVisible(!loading);
    m_refreshBtn->setEnabled(!loading);
}

void HistoryWidget::reset()
{
    clearVehicleCards();
    m_searchEdit->clear();
    m_countLabel->setText("0 vehículos");
    m_allVehicles.clear();
    m_currentPage   = 1;
    m_totalVehicles = 0;
    m_loadMoreBtn->setVisible(false);
}
