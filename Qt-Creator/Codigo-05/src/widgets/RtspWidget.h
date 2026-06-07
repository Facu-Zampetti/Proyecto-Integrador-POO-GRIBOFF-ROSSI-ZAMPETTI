#ifndef RTSPWIDGET_H
#define RTSPWIDGET_H

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include "widgets/YoloPipelineWidget.h"

/**
 * @file RtspWidget.h
 * @brief Pestaña RTSP con YOLO en vivo. Hereda de YoloPipelineWidget.
 *
 * Añade la UI de conexión RTSP (campo URL, botones Conectar/Desconectar) y
 * las señales streamStarted/streamStopped. El pipeline YOLO completo
 * (captura → sincronización → Gemini) vive en la clase base.
 *
 * Implementa los hooks del Template Method:
 *   - statusToString(): "Desconectado / Conectando… / Conectado — En vivo / Error"
 *   - updateControls(): habilita URL + conectar cuando desconectado.
 *   - onPipelineStarted/Stopped(): emite streamStarted/streamStopped.
 */
class RtspWidget : public YoloPipelineWidget {
    Q_OBJECT

public:
    explicit RtspWidget(QWidget* parent = nullptr);
    ~RtspWidget() override;

    QString widgetName() const override { return "RtspWidget"; }
    void    reset() override;

    bool isStreamActive() const {
        return m_currentStatus == Carlens::StreamStatus::Connected;
    }

signals:
    void streamStarted();
    void streamStopped();

protected:
    void setupUI()        override;
    void connectSignals() override;

    QString statusToString(Carlens::StreamStatus status) const override;
    void    updateControls(Carlens::StreamStatus status) override;
    void    onPipelineStarted() override;
    void    onPipelineStopped() override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();

private:
    QLineEdit*   m_rtspUrlEdit   = nullptr;
    QPushButton* m_connectBtn    = nullptr;
    QPushButton* m_disconnectBtn = nullptr;
};

#endif // RTSPWIDGET_H
