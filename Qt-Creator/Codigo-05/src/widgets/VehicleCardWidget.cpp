#include "widgets/VehicleCardWidget.h"
#include <QIcon>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QPixmap>

VehicleCardWidget::VehicleCardWidget(const VehicleModel& vehicle, QWidget* parent)
    : QWidget(parent)
    , m_vehicle(vehicle)
{
    setObjectName("vehicleCard");
    setCursor(Qt::PointingHandCursor);
    setupUI();
    loadThumbnail();
}

void VehicleCardWidget::setupUI()
{
    setFixedHeight(110);
    setStyleSheet(
        "#vehicleCard { "
        "  background: #161b22; border: 1px solid #30363d; border-radius: 8px; "
        "}"
        "#vehicleCard:hover { border-color: #58a6ff; background: #1c2128; }");

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(14);

    // ── Thumbnail ─────────────────────────────────────────────────────
    m_thumbnail = new QLabel(this);
    m_thumbnail->setFixedSize(80, 80);
    m_thumbnail->setAlignment(Qt::AlignCenter);
    m_thumbnail->setStyleSheet(
        "background: #21262d; border-radius: 6px;");
    m_thumbnail->setPixmap(QIcon(":/icons/car.svg").pixmap(QSize(48, 48)));
    m_thumbnail->setScaledContents(false);

    // ── Info ──────────────────────────────────────────────────────────
    QVBoxLayout* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(3);

    m_typeLabel = new QLabel(
        VehicleModel::vehicleTypeToString(m_vehicle.vehicleType()), this);
    QFont tf = m_typeLabel->font();
    tf.setWeight(QFont::DemiBold);
    tf.setPointSize(13);
    m_typeLabel->setFont(tf);
    m_typeLabel->setStyleSheet("color: #e6edf3;");

    // Atributos detectados
    QStringList details;
    if (!m_vehicle.brand().isEmpty())
        details << m_vehicle.brand();
    if (!m_vehicle.model().isEmpty())
        details << m_vehicle.model();
    if (!m_vehicle.yearRange().isEmpty())
        details << m_vehicle.yearRange();
    if (!m_vehicle.color().isEmpty())
        details << m_vehicle.color();
    if (!m_vehicle.licensePlate().isEmpty())
        details << "Patente: " + m_vehicle.licensePlate();

    m_detailsLabel = new QLabel(
        details.isEmpty() ? "Sin atributos detectados" : details.join(" · "),
        this);
    m_detailsLabel->setStyleSheet("color: #8b949e; font-size: 12px;");

    m_dateLabel = new QLabel(
        m_vehicle.detectedAt().isValid()
        ? m_vehicle.detectedAt().toString("dd/MM/yyyy  HH:mm:ss")
        : "Fecha desconocida",
        this);
    m_dateLabel->setStyleSheet("color: #484f58; font-size: 12px;");

    // Badge de confianza
    m_confidenceLabel = new QLabel(
        QString("Conf: %1%").arg(m_vehicle.confidence() * 100, 0, 'f', 1), this);
    m_confidenceLabel->setStyleSheet(
        "background: #21262d; border: 1px solid #30363d; border-radius: 10px; "
        "padding: 2px 8px; color: #8b949e; font-size: 12px;");

    // Indicador de análisis IA disponible
    QHBoxLayout* tagRow = new QHBoxLayout();
    tagRow->setSpacing(6);
    tagRow->addWidget(m_confidenceLabel);
    if (m_vehicle.hasAnalysis()) {
        QLabel* aiBadge = new QLabel("✦ IA", this);
        aiBadge->setStyleSheet(
            "background: #1f3a5f; border: 1px solid #1f6feb; border-radius: 10px; "
            "padding: 2px 8px; color: #58a6ff; font-size: 12px;");
        tagRow->addWidget(aiBadge);
    }
    tagRow->addStretch();

    infoLayout->addWidget(m_typeLabel);
    infoLayout->addWidget(m_detailsLabel);
    infoLayout->addLayout(tagRow);
    infoLayout->addWidget(m_dateLabel);

    // ── Botón ─────────────────────────────────────────────────────────
    m_analysisBtn = new QPushButton("Ver\nAnálisis", this);
    m_analysisBtn->setFixedSize(72, 56);
    m_analysisBtn->setStyleSheet(
        "QPushButton { background: #1f6feb; color: white; border: none; "
        "border-radius: 6px; font-size: 12px; font-weight: 600; }"
        "QPushButton:hover { background: #388bfd; }");
    m_analysisBtn->setCursor(Qt::PointingHandCursor);
    connect(m_analysisBtn, &QPushButton::clicked, this, [this]() {
        emit analysisRequested(m_vehicle.id());
    });

    layout->addWidget(m_thumbnail);
    layout->addLayout(infoLayout, 1);
    layout->addWidget(m_analysisBtn);
}

void VehicleCardWidget::loadThumbnail()
{
    if (m_vehicle.snapshotUrl().isEmpty()) return;

    // Descarga asíncrona del thumbnail
    auto* nam = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl(m_vehicle.snapshotUrl()));
    QNetworkReply*  reply = nam->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;

        QPixmap px;
        if (px.loadFromData(reply->readAll())) {
            m_thumbnail->setPixmap(
                px.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            m_thumbnail->setText("");
        }
    });
}
