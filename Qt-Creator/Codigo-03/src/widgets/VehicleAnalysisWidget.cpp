#include "widgets/VehicleAnalysisWidget.h"
#include "managers/HistoryManager.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

VehicleAnalysisWidget::VehicleAnalysisWidget(QWidget* parent)
    : BaseWidget(parent)
{
    setupUI();
    connectSignals();
}

void VehicleAnalysisWidget::setupUI()
{
    setObjectName("vehicleAnalysisWidget");
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(32, 32, 32, 32);
    rootLayout->setSpacing(20);

    // ── Título ────────────────────────────────────────────────────────
    QLabel* sectionTitle = new QLabel("Análisis de Vehículo", this);
    QFont tf = sectionTitle->font();
    tf.setPointSize(18);
    tf.setWeight(QFont::DemiBold);
    sectionTitle->setFont(tf);
    sectionTitle->setStyleSheet("color: #e6edf3;");

    rootLayout->addWidget(sectionTitle);

    // ── QStackedWidget con 3 páginas ──────────────────────────────────
    m_stack = new QStackedWidget(this);
    rootLayout->addWidget(m_stack, 1);

    // Página 0 – vacío
    m_emptyLabel = new QLabel(
        "🔍  Seleccione un vehículo del Historial\n"
        "para ver sus detalles y el análisis disponible.",
        m_stack);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("color: #484f58; font-size: 14px;");
    m_emptyLabel->setWordWrap(true);
    m_stack->addWidget(m_emptyLabel);

    // Página 1 – contenido del vehículo
    QWidget* contentPage = new QWidget(m_stack);
    QHBoxLayout* contentLayout = new QHBoxLayout(contentPage);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(24);

    // ── Columna izquierda: snapshot + atributos ────────────────────────
    QWidget* leftCol = new QWidget(contentPage);
    leftCol->setFixedWidth(280);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftCol);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(12);

    m_snapshotLabel = new QLabel(leftCol);
    m_snapshotLabel->setFixedSize(280, 200);
    m_snapshotLabel->setAlignment(Qt::AlignCenter);
    m_snapshotLabel->setStyleSheet(
        "background: #21262d; border: 1px solid #30363d; "
        "border-radius: 8px; color: #484f58; font-size: 36px;");
    m_snapshotLabel->setText("🚗");
    m_snapshotLabel->setScaledContents(false);

    // Card de atributos
    QWidget* attrsCard = new QWidget(leftCol);
    attrsCard->setStyleSheet(
        "background: #161b22; border: 1px solid #30363d; border-radius: 8px;");
    QGridLayout* attrsGrid = new QGridLayout(attrsCard);
    attrsGrid->setContentsMargins(16, 14, 16, 14);
    attrsGrid->setSpacing(8);
    attrsGrid->setColumnMinimumWidth(0, 80);

    auto addAttr = [&](int row, const QString& key, QLabel*& label) {
        QLabel* keyLabel = new QLabel(key, attrsCard);
        keyLabel->setStyleSheet("color: #484f58; font-size: 11px;");
        label = new QLabel("—", attrsCard);
        label->setStyleSheet("color: #e6edf3; font-size: 12px; font-weight: 500;");
        label->setWordWrap(true);
        attrsGrid->addWidget(keyLabel, row, 0, Qt::AlignTop);
        attrsGrid->addWidget(label,    row, 1);
    };

    addAttr(0, "Tipo",       m_typeLabel);
    addAttr(1, "Marca",      m_brandLabel);
    addAttr(2, "Modelo",     m_modelLabel);
    addAttr(3, "Año aprox.", m_yearLabel);
    addAttr(4, "Color",      m_colorLabel);
    addAttr(5, "Patente",    m_plateLabel);
    addAttr(6, "Confianza",  m_confLabel);
    addAttr(7, "Detectado",  m_dateLabel);

    leftLayout->addWidget(m_snapshotLabel);
    leftLayout->addWidget(attrsCard);
    leftLayout->addStretch();

    // ── Columna derecha: análisis disponible ─────────────────────────
    QWidget* rightCol = new QWidget(contentPage);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightCol);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(12);

    QLabel* analysisTitle = new QLabel("Resumen del análisis", rightCol);
    QFont at = analysisTitle->font();
    at.setWeight(QFont::DemiBold);
    at.setPointSize(13);
    analysisTitle->setFont(at);
    analysisTitle->setStyleSheet("color: #e6edf3;");

    m_analysisText = new QTextEdit(rightCol);
    m_analysisText->setReadOnly(true);
    m_analysisText->setPlaceholderText(
        "El análisis local del vehículo aparecerá aquí cuando el backend tenga "
        "información disponible para esa detección.");
    m_analysisText->setStyleSheet(
        "QTextEdit { background: #161b22; border: 1px solid #30363d; "
        "border-radius: 8px; color: #e6edf3; font-size: 13px; padding: 12px; "
        "line-height: 1.6; }");

    rightLayout->addWidget(analysisTitle);
    rightLayout->addWidget(m_analysisText, 1);

    contentLayout->addWidget(leftCol);
    contentLayout->addWidget(rightCol, 1);
    m_stack->addWidget(contentPage);

    // Página 2 – cargando
    m_loadingLabel = new QLabel("Cargando detalles…", m_stack);
    m_loadingLabel->setAlignment(Qt::AlignCenter);
    m_loadingLabel->setStyleSheet("color: #8b949e; font-size: 14px;");
    m_stack->addWidget(m_loadingLabel);

    m_stack->setCurrentIndex(0);
}

void VehicleAnalysisWidget::connectSignals()
{
    auto* hm = HistoryManager::instance();
    connect(hm, &HistoryManager::vehicleDetailsLoaded,
            this, &VehicleAnalysisWidget::onVehicleDetailsLoaded);
    connect(hm, &HistoryManager::vehicleDetailsFailed,
            this, &VehicleAnalysisWidget::onVehicleDetailsFailed);
}

// ─── Public API ───────────────────────────────────────────────────────────

void VehicleAnalysisWidget::loadVehicle(int vehicleId)
{
    m_currentVehicleId = vehicleId;
    setLoading(true);
    HistoryManager::instance()->loadVehicleDetails(vehicleId);
}

// ─── Slots ────────────────────────────────────────────────────────────────

void VehicleAnalysisWidget::onVehicleDetailsLoaded(const VehicleModel& vehicle,
                                                    const AnalysisModel& analysis)
{
    setLoading(false);
    populateVehicle(vehicle, analysis);
}

void VehicleAnalysisWidget::onVehicleDetailsFailed(const QString& reason)
{
    setLoading(false);
    m_analysisText->setPlainText("Error al cargar los detalles: " + reason);
    m_stack->setCurrentIndex(1);
}

// ─── Helpers ──────────────────────────────────────────────────────────────

void VehicleAnalysisWidget::populateVehicle(const VehicleModel& v,
                                              const AnalysisModel& a)
{
    m_typeLabel->setText(VehicleModel::vehicleTypeToString(v.vehicleType()));
    m_brandLabel->setText(v.brand().isEmpty()  ? "—" : v.brand());
    m_modelLabel->setText(v.model().isEmpty()  ? "—" : v.model());
    m_yearLabel->setText(v.yearRange().isEmpty() ? "—" : v.yearRange());
    m_colorLabel->setText(v.color().isEmpty()  ? "—" : v.color());
    m_plateLabel->setText(v.licensePlate().isEmpty() ? "—" : v.licensePlate());
    m_confLabel->setText(formatConfidence(v.confidence()));
    m_dateLabel->setText(
        v.detectedAt().isValid()
        ? v.detectedAt().toString("dd/MM/yyyy  HH:mm:ss")
        : "—");

    if (a.isValid())
        m_analysisText->setPlainText(
            a.fullText().isEmpty() ? a.observations() : a.fullText());
    else
        m_analysisText->clear();

    loadSnapshot(v.snapshotUrl());
    m_stack->setCurrentIndex(1);
}

void VehicleAnalysisWidget::loadSnapshot(const QString& url)
{
    if (url.isEmpty()) { m_snapshotLabel->setText("🚗"); return; }

    auto* nam   = new QNetworkAccessManager(this);
    auto* reply = nam->get(QNetworkRequest(QUrl(url)));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;
        QPixmap px;
        if (px.loadFromData(reply->readAll()))
            m_snapshotLabel->setPixmap(
                px.scaled(280, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    });
}

void VehicleAnalysisWidget::setLoading(bool loading)
{
    m_stack->setCurrentIndex(loading ? 2 : m_stack->currentIndex());
}

QString VehicleAnalysisWidget::formatConfidence(double conf) const
{
    return QString("%1%").arg(conf * 100, 0, 'f', 1);
}

void VehicleAnalysisWidget::reset()
{
    m_currentVehicleId = -1;
    m_snapshotLabel->clear();
    m_snapshotLabel->setText("🚗");
    m_typeLabel->setText("—");
    m_brandLabel->setText("—");
    m_modelLabel->setText("—");
    m_yearLabel->setText("—");
    m_colorLabel->setText("—");
    m_plateLabel->setText("—");
    m_confLabel->setText("—");
    m_dateLabel->setText("—");
    m_analysisText->clear();
    m_stack->setCurrentIndex(0);
}
