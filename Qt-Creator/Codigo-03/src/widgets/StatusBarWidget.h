#ifndef STATUSBARWIDGET_H
#define STATUSBARWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QTimer>

#include "utils/AppEnums.h"

/**
 * @file StatusBarWidget.h
 * @brief Barra de estado inferior del Dashboard.
 *
 * Muestra mensajes temporales de estado (éxito, error, info)
 * con auto-limpieza vía QTimer.
 */
class StatusBarWidget : public QWidget {
    Q_OBJECT

public:
    explicit StatusBarWidget(QWidget* parent = nullptr);

    void showMessage(const QString& message, int durationMs = 4000);
    void showError(const QString& message,   int durationMs = 5000);
    void showSuccess(const QString& message, int durationMs = 4000);
    void clearMessage();

    void setApiStatus(Carlens::ApiStatus status);

private:
    void setupUI();

    QLabel* m_messageLabel  = nullptr;
    QLabel* m_statusDot     = nullptr;
    QLabel* m_versionLabel  = nullptr;
    QTimer* m_clearTimer    = nullptr;
};

#endif // STATUSBARWIDGET_H
