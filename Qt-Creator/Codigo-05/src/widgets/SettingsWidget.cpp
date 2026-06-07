#include "widgets/SettingsWidget.h"
#include "managers/ConfigManager.h"
#include "services/adminDB.h"
#include "base/Singleton.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QTimer>

SettingsWidget::SettingsWidget(QWidget* parent)
    : BaseWidget(parent)
{
    setupUI();
    connectSignals();
    applyStyle();
    loadSettings();
}

// ─── BaseWidget interface ─────────────────────────────────────────────────

void SettingsWidget::setupUI()
{
    setObjectName("settingsWidget");

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(32, 32, 32, 32);
    root->setSpacing(24);

    // ── Título ────────────────────────────────────────────────────────
    QLabel* title = new QLabel("Configuración", this);
    QFont tf = title->font();
    tf.setPointSize(18);
    tf.setWeight(QFont::DemiBold);
    title->setFont(tf);
    title->setStyleSheet("color: #e6edf3;");
    root->addWidget(title);

    // ── Grupo: Detección ──────────────────────────────────────────────
    // QGroupBox demuestra el agrupamiento visual de controles relacionados.
    QGroupBox* detGroup = new QGroupBox("Detección YOLO", this);
    detGroup->setStyleSheet(
        "QGroupBox { color: #8b949e; font-size: 13px; font-weight: 600; "
        "border: 1px solid #30363d; border-radius: 8px; margin-top: 12px; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }");

    QVBoxLayout* detLayout = new QVBoxLayout(detGroup);
    detLayout->setContentsMargins(16, 16, 16, 16);
    detLayout->setSpacing(12);

    // QSlider: umbral de confianza (0–100 %)
    QLabel* confLabel = new QLabel("Umbral de confianza YOLO:", detGroup);
    confLabel->setStyleSheet("color: #e6edf3; font-size: 13px;");

    QHBoxLayout* sliderRow = new QHBoxLayout();
    m_confidenceSlider = new QSlider(Qt::Horizontal, detGroup);
    m_confidenceSlider->setRange(0, 100);
    m_confidenceSlider->setTickInterval(10);
    m_confidenceSlider->setTickPosition(QSlider::TicksBelow);
    m_confidenceSlider->setStyleSheet(
        "QSlider::groove:horizontal { height: 4px; background: #30363d; border-radius: 2px; }"
        "QSlider::handle:horizontal { width: 14px; height: 14px; margin: -5px 0; "
        "background: #1f6feb; border-radius: 7px; }"
        "QSlider::sub-page:horizontal { background: #1f6feb; border-radius: 2px; }");

    m_confidenceValue = new QLabel("35%", detGroup);
    m_confidenceValue->setFixedWidth(40);
    m_confidenceValue->setAlignment(Qt::AlignCenter);
    m_confidenceValue->setStyleSheet("color: #58a6ff; font-size: 13px; font-weight: 600;");

    sliderRow->addWidget(m_confidenceSlider, 1);
    sliderRow->addWidget(m_confidenceValue);

    // QSpinBox: frame-skip (0 = sin saltar, N = procesar 1 de cada N+1 frames)
    QLabel* skipLabel = new QLabel("Saltar frames (0 = ninguno):", detGroup);
    skipLabel->setStyleSheet("color: #e6edf3; font-size: 13px;");

    m_frameSkipSpin = new QSpinBox(detGroup);
    m_frameSkipSpin->setRange(0, 10);
    m_frameSkipSpin->setSuffix(" frames");
    m_frameSkipSpin->setMinimumHeight(36);
    m_frameSkipSpin->setStyleSheet(
        "QSpinBox { background: #21262d; border: 1px solid #30363d; border-radius: 6px; "
        "color: #e6edf3; font-size: 13px; padding: 4px 8px; }"
        "QSpinBox:focus { border-color: #1f6feb; }");

    detLayout->addWidget(confLabel);
    detLayout->addLayout(sliderRow);
    detLayout->addSpacing(4);
    detLayout->addWidget(skipLabel);
    detLayout->addWidget(m_frameSkipSpin);

    // ── Grupo: Conexión ───────────────────────────────────────────────
    QGroupBox* connGroup = new QGroupBox("Conexión y polling", this);
    connGroup->setStyleSheet(detGroup->styleSheet());

    QVBoxLayout* connLayout = new QVBoxLayout(connGroup);
    connLayout->setContentsMargins(16, 16, 16, 16);
    connLayout->setSpacing(12);

    // URL RTSP por defecto
    QLabel* rtspLabel = new QLabel("URL RTSP por defecto:", connGroup);
    rtspLabel->setStyleSheet("color: #e6edf3; font-size: 13px;");

    m_rtspUrlEdit = new QLineEdit(connGroup);
    m_rtspUrlEdit->setPlaceholderText("rtsp://usuario:pass@host:puerto/stream");
    m_rtspUrlEdit->setMinimumHeight(36);
    m_rtspUrlEdit->setStyleSheet(
        "QLineEdit { background: #21262d; border: 1px solid #30363d; border-radius: 6px; "
        "color: #e6edf3; font-size: 13px; padding: 4px 10px; }"
        "QLineEdit:focus { border-color: #1f6feb; }");

    // QSpinBox: intervalo de polling del historial en segundo plano
    QLabel* pollLabel = new QLabel("Intervalo de actualización del historial:", connGroup);
    pollLabel->setStyleSheet("color: #e6edf3; font-size: 13px;");

    m_pollIntervalSpin = new QSpinBox(connGroup);
    m_pollIntervalSpin->setRange(1, 60);
    m_pollIntervalSpin->setSuffix(" s");
    m_pollIntervalSpin->setMinimumHeight(36);
    m_pollIntervalSpin->setStyleSheet(m_frameSkipSpin->styleSheet());

    connLayout->addWidget(rtspLabel);
    connLayout->addWidget(m_rtspUrlEdit);
    connLayout->addSpacing(4);
    connLayout->addWidget(pollLabel);
    connLayout->addWidget(m_pollIntervalSpin);

    // ── Grupo: ODBC (demo) ────────────────────────────────────────────
    QGroupBox* odbcGroup = new QGroupBox("Conexión ODBC (demo)", this);
    odbcGroup->setStyleSheet(detGroup->styleSheet());

    QVBoxLayout* odbcLayout = new QVBoxLayout(odbcGroup);
    odbcLayout->setContentsMargins(16, 16, 16, 16);
    odbcLayout->setSpacing(12);

    QLabel* dsnLabel = new QLabel("Nombre del DSN ODBC:", odbcGroup);
    dsnLabel->setStyleSheet("color: #e6edf3; font-size: 13px;");

    m_odbcDsnEdit = new QLineEdit(odbcGroup);
    m_odbcDsnEdit->setPlaceholderText("Ej: MiDSN");
    m_odbcDsnEdit->setMinimumHeight(36);
    m_odbcDsnEdit->setStyleSheet(m_rtspUrlEdit->styleSheet());

    m_odbcProbeBtn = new QPushButton("Probar conexión", odbcGroup);
    m_odbcProbeBtn->setMinimumHeight(36);
    m_odbcProbeBtn->setCursor(Qt::PointingHandCursor);
    m_odbcProbeBtn->setStyleSheet(
        "QPushButton { background: #21262d; border: 1px solid #30363d; border-radius: 6px; "
        "color: #e6edf3; font-size: 13px; padding: 4px 16px; }"
        "QPushButton:hover { border-color: #8b949e; background: #30363d; }");

    m_odbcResultLabel = new QLabel("", odbcGroup);
    m_odbcResultLabel->setWordWrap(true);
    m_odbcResultLabel->setStyleSheet("font-size: 13px; color: #8b949e;");

    odbcLayout->addWidget(dsnLabel);
    odbcLayout->addWidget(m_odbcDsnEdit);
    odbcLayout->addWidget(m_odbcProbeBtn);
    odbcLayout->addWidget(m_odbcResultLabel);

    // ── Botón guardar + estado ────────────────────────────────────────
    QHBoxLayout* btnRow = new QHBoxLayout();
    m_saveBtn = new QPushButton("Guardar configuración", this);
    m_saveBtn->setProperty("class", "primary");
    m_saveBtn->setMinimumHeight(42);
    m_saveBtn->setMinimumWidth(200);
    m_saveBtn->setCursor(Qt::PointingHandCursor);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: #3fb950; font-size: 13px;");
    m_statusLabel->setVisible(false);

    btnRow->addStretch();
    btnRow->addWidget(m_saveBtn);
    btnRow->addStretch();

    root->addWidget(detGroup);
    root->addWidget(connGroup);
    root->addWidget(odbcGroup);
    root->addLayout(btnRow);
    root->addWidget(m_statusLabel, 0, Qt::AlignCenter);
    root->addStretch();
}

void SettingsWidget::connectSignals()
{
    connect(m_saveBtn, &QPushButton::clicked,
            this,      &SettingsWidget::onSaveClicked);

    connect(m_confidenceSlider, &QSlider::valueChanged,
            this,               &SettingsWidget::onConfidenceSliderMoved);

    connect(m_odbcProbeBtn, &QPushButton::clicked,
            this,           &SettingsWidget::onOdbcProbeClicked);
}

void SettingsWidget::applyStyle()
{
    setStyleSheet("SettingsWidget { background: #0d1117; }");
}

// ─── Slots ────────────────────────────────────────────────────────────────

void SettingsWidget::onSaveClicked()
{
    const double conf = m_confidenceSlider->value() / 100.0;
    ConfigManager::instance()->setConfidenceThreshold(conf);
    ConfigManager::instance()->setRtspDefaultUrl(m_rtspUrlEdit->text().trimmed());

    adminDB::instance()->saveSetting(
        "poll_interval_sec",
        QString::number(m_pollIntervalSpin->value()));
    adminDB::instance()->saveSetting(
        "frame_skip",
        QString::number(m_frameSkipSpin->value()));

    m_statusLabel->setText("Configuración guardada.");
    m_statusLabel->setVisible(true);

    // Ocultar el mensaje después de 3 segundos.
    QTimer::singleShot(3000, this, [this]() { m_statusLabel->setVisible(false); });

    emit settingsSaved();
}

void SettingsWidget::onConfidenceSliderMoved(int value)
{
    m_confidenceValue->setText(QString("%1%").arg(value));
}

void SettingsWidget::onOdbcProbeClicked()
{
    const QString dsn = m_odbcDsnEdit->text().trimmed();
    if (dsn.isEmpty()) {
        m_odbcResultLabel->setStyleSheet("color: #f85149; font-size: 13px;");
        m_odbcResultLabel->setText("Ingresa un nombre de DSN.");
        return;
    }

    Result<QString> result = adminDB::instance()->probeOdbc(dsn);
    if (result.ok) {
        m_odbcResultLabel->setStyleSheet("color: #3fb950; font-size: 13px;");
        m_odbcResultLabel->setText(result.value);
    } else {
        m_odbcResultLabel->setStyleSheet("color: #f85149; font-size: 13px;");
        m_odbcResultLabel->setText(result.error);
    }
}

// ─── Helpers ──────────────────────────────────────────────────────────────

void SettingsWidget::loadSettings()
{
    const int confPct = static_cast<int>(
        ConfigManager::instance()->confidenceThreshold() * 100);
    m_confidenceSlider->setValue(confPct);
    m_confidenceValue->setText(QString("%1%").arg(confPct));

    m_rtspUrlEdit->setText(ConfigManager::instance()->rtspDefaultUrl());

    const int pollSec = adminDB::instance()->loadSetting("poll_interval_sec", "5").toInt();
    m_pollIntervalSpin->setValue(qBound(1, pollSec, 60));

    const int frameSkip = adminDB::instance()->loadSetting("frame_skip", "0").toInt();
    m_frameSkipSpin->setValue(qBound(0, frameSkip, 10));
}

void SettingsWidget::reset()
{
    loadSettings();
    m_statusLabel->setVisible(false);
}
