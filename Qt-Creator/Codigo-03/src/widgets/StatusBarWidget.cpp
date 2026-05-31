#include "widgets/StatusBarWidget.h"

StatusBarWidget::StatusBarWidget(QWidget* parent)
    : QWidget(parent)
    , m_clearTimer(new QTimer(this))
{
    setObjectName("statusBar");
    setFixedHeight(28);
    setupUI();

    m_clearTimer->setSingleShot(true);
    connect(m_clearTimer, &QTimer::timeout, this, &StatusBarWidget::clearMessage);
}

void StatusBarWidget::setupUI()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 0, 16, 0);
    layout->setSpacing(8);

    m_statusDot = new QLabel(this);
    m_statusDot->setFixedSize(8, 8);
    m_statusDot->setStyleSheet(
        "background-color: #3fb950; border-radius: 4px;");

    m_messageLabel = new QLabel(this);
    m_messageLabel->setStyleSheet("color: #8b949e; font-size: 11px;");

    m_versionLabel = new QLabel("Carlens v1.0 · YOLOv11 + Gemini AI", this);
    m_versionLabel->setStyleSheet("color: #484f58; font-size: 11px;");

    layout->addWidget(m_statusDot);
    layout->addWidget(m_messageLabel);
    layout->addStretch();
    layout->addWidget(m_versionLabel);
}

void StatusBarWidget::showMessage(const QString& message, int durationMs)
{
    m_messageLabel->setText(message);
    m_messageLabel->setStyleSheet("color: #8b949e; font-size: 11px;");
    if (durationMs > 0) m_clearTimer->start(durationMs);
}

void StatusBarWidget::showError(const QString& message, int durationMs)
{
    m_messageLabel->setText("⚠ " + message);
    m_messageLabel->setStyleSheet("color: #f85149; font-size: 11px;");
    if (durationMs > 0) m_clearTimer->start(durationMs);
}

void StatusBarWidget::showSuccess(const QString& message, int durationMs)
{
    m_messageLabel->setText("✓ " + message);
    m_messageLabel->setStyleSheet("color: #3fb950; font-size: 11px;");
    if (durationMs > 0) m_clearTimer->start(durationMs);
}

void StatusBarWidget::clearMessage()
{
    m_messageLabel->clear();
}

void StatusBarWidget::setApiStatus(Carlens::ApiStatus status)
{
    switch (status) {
    case Carlens::ApiStatus::Loading:
        m_statusDot->setStyleSheet("background-color: #d29922; border-radius: 4px;");
        break;
    case Carlens::ApiStatus::Success:
        m_statusDot->setStyleSheet("background-color: #3fb950; border-radius: 4px;");
        break;
    case Carlens::ApiStatus::Error:
        m_statusDot->setStyleSheet("background-color: #f85149; border-radius: 4px;");
        break;
    default:
        m_statusDot->setStyleSheet("background-color: #8b949e; border-radius: 4px;");
        break;
    }
}
