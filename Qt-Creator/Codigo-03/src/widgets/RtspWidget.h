#ifndef RTSPWIDGET_H
#define RTSPWIDGET_H

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QTimer>

#include "base/BaseWidget.h"
#include "models/SessionModel.h"
#include "utils/AppEnums.h"

/**
 * @file RtspWidget.h
 * @brief Módulo de conexión a stream RTSP. Hereda de BaseWidget.
 *
 * Funcionalidades:
 *   - Ingreso y validación de URL RTSP.
 *   - Botones de Conectar/Desconectar.
 *   - Indicador de estado del stream con color.
 *   - Área de "pantalla" simulada para el stream.
 *   - Integración con RtspManager.
 */
class RtspWidget : public BaseWidget {
    Q_OBJECT

public:
    explicit RtspWidget(QWidget* parent = nullptr);
    QString widgetName() const override { return "RtspWidget"; }
    void    reset() override;

signals:
    void statusMessage(const QString& msg);
    void errorMessage(const QString& msg);

protected:
    void setupUI()        override;
    void connectSignals() override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onStreamStatusChanged(Carlens::StreamStatus status);
    void onStreamStarted(const SessionModel& session);
    void onStreamError(const QString& reason);
    void updateConnectionTime();

private:
    void setStatus(Carlens::StreamStatus status);
    QString statusToString(Carlens::StreamStatus status) const;
    QString statusToColor(Carlens::StreamStatus status) const;

    // ── UI ─────────────────────────────────────────────────────────────
    QLineEdit*   m_rtspUrlEdit      = nullptr;
    QPushButton* m_connectBtn       = nullptr;
    QPushButton* m_disconnectBtn    = nullptr;
    QLabel*      m_statusDot        = nullptr;
    QLabel*      m_statusLabel      = nullptr;
    QLabel*      m_sessionInfoLabel = nullptr;
    QWidget*     m_previewArea      = nullptr;
    QLabel*      m_previewLabel     = nullptr;
    QLabel*      m_timerLabel       = nullptr;
    QTimer*      m_connectionTimer  = nullptr;

    int m_elapsedSeconds = 0;
    Carlens::StreamStatus m_currentStatus = Carlens::StreamStatus::Disconnected;
};

#endif // RTSPWIDGET_H
