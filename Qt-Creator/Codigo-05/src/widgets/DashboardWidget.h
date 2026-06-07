#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QTimer>

#include "base/BaseWidget.h"
#include "widgets/SidebarWidget.h"
#include "widgets/StatusBarWidget.h"
#include "widgets/VideoUploadWidget.h"
#include "widgets/RtspWidget.h"
#include "widgets/HistoryWidget.h"
#include "widgets/VehicleAnalysisWidget.h"
#include "widgets/SettingsWidget.h"
#include "models/UserModel.h"
#include "utils/AppEnums.h"

/**
 * @file DashboardWidget.h
 * @brief Pantalla principal del sistema tras el login. Hereda de BaseWidget.
 *
 * Arquitectura interna:
 *   ┌───────────────────────────────────────────────────────┐
 *   │ SidebarWidget (izq) │ QStackedWidget (contenido)      │
 *   │                     │  ┌──────────────────────────┐  │
 *   │  NavButton: Video   │  │ VideoUploadWidget         │  │
 *   │  NavButton: RTSP    │  │ RtspWidget                │  │
 *   │  NavButton: Histor. │  │ HistoryWidget             │  │
 *   │  NavButton: Anális. │  │ VehicleAnalysisWidget     │  │
 *   │                     │  └──────────────────────────┘  │
 *   └─────────────────────────────────────────────────────── ┘
 *   │ StatusBarWidget (abajo)                               │
 *   └───────────────────────────────────────────────────────┘
 */
class DashboardWidget : public BaseWidget {
    Q_OBJECT

public:
    explicit DashboardWidget(QWidget* parent = nullptr);

    QString widgetName() const override { return "DashboardWidget"; }
    void    reset() override;
    void    setCurrentUser(const UserModel& user);
    void    showSection(Carlens::DashboardSection section);

signals:
    void logoutRequested();

protected:
    void setupUI()        override;
    void connectSignals() override;

private:
    void updateHistoryRefreshPolling();

    QWidget*            m_topBar          = nullptr;
    QLabel*             m_topBarTitle     = nullptr;
    SidebarWidget*      m_sidebar         = nullptr;
    QStackedWidget*     m_contentStack    = nullptr;
    StatusBarWidget*    m_statusBar       = nullptr;
    QTimer*             m_backgroundHistoryRefreshTimer = nullptr;

    VideoUploadWidget*      m_videoWidget     = nullptr;
    RtspWidget*             m_rtspWidget      = nullptr;
    HistoryWidget*          m_historyWidget   = nullptr;
    VehicleAnalysisWidget*  m_analysisWidget  = nullptr;
    SettingsWidget*         m_settingsWidget  = nullptr;

    int m_videoIndex    = 0;
    int m_rtspIndex     = 1;
    int m_historyIndex  = 2;
    int m_analysisIndex = 3;
    int m_settingsIndex = 4;

    Carlens::DashboardSection m_currentSection = Carlens::DashboardSection::VideoUpload;
};

#endif // DASHBOARDWIDGET_H
