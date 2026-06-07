#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

#include "base/BaseWidget.h"

/**
 * @file SettingsWidget.h
 * @brief Pantalla de Configuración del Dashboard. Hereda de BaseWidget.
 *
 * Demuestra el uso de:
 *   - QGroupBox: agrupa controles relacionados visualmente.
 *   - QSlider:   umbral de confianza YOLO (0–100 %).
 *   - QSpinBox:  frame-skip y tiempo de polling del historial.
 *
 * La configuración se persiste via ConfigManager y adminDB (saveSetting).
 *
 * Principios: composición de widgets Qt, persistencia, señales/slots.
 */
class SettingsWidget : public BaseWidget {
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget* parent = nullptr);

    QString widgetName() const override { return "SettingsWidget"; }
    void    reset() override;

signals:
    void settingsSaved();

protected:
    void setupUI()        override;
    void connectSignals() override;
    void applyStyle()     override;

private slots:
    void onSaveClicked();
    void onConfidenceSliderMoved(int value);
    void onOdbcProbeClicked();

private:
    void loadSettings();

    // ── Grupo Detección ───────────────────────────────────────────────
    QSlider*  m_confidenceSlider = nullptr;
    QLabel*   m_confidenceValue  = nullptr;
    QSpinBox* m_frameSkipSpin    = nullptr;

    // ── Grupo Conexión ────────────────────────────────────────────────
    QLineEdit* m_rtspUrlEdit      = nullptr;
    QSpinBox*  m_pollIntervalSpin = nullptr;

    // ── Grupo ODBC (demo) ────────────────────────────────────────────
    QLineEdit*   m_odbcDsnEdit     = nullptr;
    QPushButton* m_odbcProbeBtn    = nullptr;
    QLabel*      m_odbcResultLabel = nullptr;

    // ── Acciones ──────────────────────────────────────────────────────
    QPushButton* m_saveBtn    = nullptr;
    QLabel*      m_statusLabel = nullptr;
};

#endif // SETTINGSWIDGET_H
