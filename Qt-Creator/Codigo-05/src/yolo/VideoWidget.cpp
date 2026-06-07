#include "VideoWidget.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>

VideoWidget::VideoWidget(QWidget *parent)
    : QWidget(parent) {
    setMinimumSize(640, 360);
}

void VideoWidget::clearFrame() {
    m_frame = QImage();
    m_detections.clear();
    m_sourceWidth = 0;
    m_sourceHeight = 0;
    update();
}

void VideoWidget::setFrame(const QImage &frame, const DetectionList &detections, int sourceWidth, int sourceHeight) {
    m_frame = frame;
    m_detections = detections;
    m_sourceWidth = sourceWidth;
    m_sourceHeight = sourceHeight;
    update();
}

QSize VideoWidget::minimumSizeHint() const {
    return {640, 360};
}

void VideoWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor(18, 18, 18));

    if (m_frame.isNull()) {
        painter.setPen(QColor(210, 210, 210));
        painter.drawText(rect(), Qt::AlignCenter, "Esperando frames desde Python...");
        return;
    }

    const QRect imageTarget = targetRect();
    painter.drawImage(imageTarget, m_frame);

    const int imageWidth = m_sourceWidth > 0 ? m_sourceWidth : m_frame.width();
    const int imageHeight = m_sourceHeight > 0 ? m_sourceHeight : m_frame.height();
    if (imageWidth <= 0 || imageHeight <= 0) {
        return;
    }

    const double scaleX = static_cast<double>(imageTarget.width()) / static_cast<double>(imageWidth);
    const double scaleY = static_cast<double>(imageTarget.height()) / static_cast<double>(imageHeight);

    QPen pen(QColor(40, 220, 40));
    pen.setWidth(2);
    painter.setPen(pen);

    for (const DetectionBox &detection : m_detections) {
        const QRect box = detection.bbox;
        QRect scaledBox(
            imageTarget.left() + qRound(box.left() * scaleX),
            imageTarget.top() + qRound(box.top() * scaleY),
            qRound(box.width() * scaleX),
            qRound(box.height() * scaleY)
        );

        painter.drawRect(scaledBox);

        const QString caption = QString("%1 %2")
                                    .arg(detection.label)
                                    .arg(detection.confidence, 0, 'f', 2);
        const QRect textRect = painter.boundingRect(QRect(), Qt::AlignLeft, caption).adjusted(-6, -4, 6, 4);
        const QPoint textOrigin(scaledBox.left(), qMax(imageTarget.top(), scaledBox.top() - textRect.height()));
        const QRect captionRect(textOrigin, textRect.size());
        painter.fillRect(captionRect, QColor(40, 220, 40, 220));
        painter.setPen(Qt::black);
        painter.drawText(captionRect, Qt::AlignCenter, caption);
        painter.setPen(pen);
    }
}

void VideoWidget::mousePressEvent(QMouseEvent *event) {
    bool insideImage = false;
    const QPoint imagePoint = mapWidgetPointToImage(event->position().toPoint(), &insideImage);
    if (!insideImage) {
        QWidget::mousePressEvent(event);
        return;
    }

    for (int index = m_detections.size() - 1; index >= 0; --index) {
        if (m_detections.at(index).bbox.contains(imagePoint)) {
            emit detectionClicked(index, imagePoint);
            return;
        }
    }

    QWidget::mousePressEvent(event);
}

QRect VideoWidget::targetRect() const {
    if (m_frame.isNull()) {
        return rect();
    }

    const int imageWidth = m_sourceWidth > 0 ? m_sourceWidth : m_frame.width();
    const int imageHeight = m_sourceHeight > 0 ? m_sourceHeight : m_frame.height();
    const QSize scaledSize(imageWidth, imageHeight);
    const QSize fitted = scaledSize.scaled(size(), Qt::KeepAspectRatio);
    const int left = (width() - fitted.width()) / 2;
    const int top = (height() - fitted.height()) / 2;
    return QRect(QPoint(left, top), fitted);
}

QPoint VideoWidget::mapWidgetPointToImage(const QPoint &widgetPoint, bool *insideImage) const {
    if (insideImage != nullptr) {
        *insideImage = false;
    }

    if (m_frame.isNull()) {
        return {};
    }

    const QRect imageTarget = targetRect();
    if (!imageTarget.contains(widgetPoint)) {
        return {};
    }

    const int imageWidth = m_sourceWidth > 0 ? m_sourceWidth : m_frame.width();
    const int imageHeight = m_sourceHeight > 0 ? m_sourceHeight : m_frame.height();
    if (imageWidth <= 0 || imageHeight <= 0) {
        return {};
    }

    const double normalizedX = static_cast<double>(widgetPoint.x() - imageTarget.left()) /
                               static_cast<double>(imageTarget.width());
    const double normalizedY = static_cast<double>(widgetPoint.y() - imageTarget.top()) /
                               static_cast<double>(imageTarget.height());

    if (insideImage != nullptr) {
        *insideImage = true;
    }

    return {
        qBound(0, qRound(normalizedX * imageWidth), imageWidth - 1),
        qBound(0, qRound(normalizedY * imageHeight), imageHeight - 1),
    };
}