#ifndef SIDEBARWIDGET_H
#define SIDEBARWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QButtonGroup>
#include <QMap>

#include "utils/AppEnums.h"
#include "models/UserModel.h"

/**
 * @file SidebarWidget.h
 * @brief Menú lateral del Dashboard. Widget de navegación.
 *
 * Muestra:
 *   - Logo y título de la app.
 *   - Avatar e información del usuario.
 *   - Botones de navegación entre secciones del Dashboard.
 *   - Botón de Logout en la parte inferior.
 *
 * Principio: Separación de responsabilidades, señales/slots.
 */
class SidebarWidget : public QWidget {
    Q_OBJECT

public:
    explicit SidebarWidget(QWidget* parent = nullptr);

    void setCurrentUser(const UserModel& user);
    void setActiveSection(Carlens::DashboardSection section);

signals:
    void sectionRequested(Carlens::DashboardSection section);
    void logoutRequested();

private:
    void setupUI();
    void connectSignals();
    QPushButton* createNavButton(const QString& text,
                                 const QString& icon,
                                 Carlens::DashboardSection section);

    QLabel*      m_userLabel      = nullptr;
    QLabel*      m_emailLabel     = nullptr;
    QLabel*      m_avatarLabel    = nullptr;
    QButtonGroup* m_navGroup      = nullptr;

    QMap<Carlens::DashboardSection, QPushButton*> m_navButtons;
};

#endif // SIDEBARWIDGET_H
