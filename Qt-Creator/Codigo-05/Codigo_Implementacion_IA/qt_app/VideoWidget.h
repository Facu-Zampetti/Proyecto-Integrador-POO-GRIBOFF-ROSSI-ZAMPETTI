#pragma once

#include <QImage>
#include <QWidget>

#include "DetectionTypes.h"

class QMouseEvent;
class QPaintEvent;

class VideoWidget : public QWidget {
    Q_OBJECT

public:
    explicit VideoWidget(QWidget *parent = nullptr);

    void clearFrame();
    void setFrame(const QImage &frame, const DetectionList &detections, int sourceWidth, int sourceHeight);
    QSize minimumSizeHint() const override;

signals:
    void detectionClicked(int detectionIndex, QPoint imagePoint);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QRect targetRect() const;
    QPoint mapWidgetPointToImage(const QPoint &widgetPoint, bool *insideImage) const;

    QImage frame_;
    DetectionList detections_;
    int sourceWidth_ = 0;
    int sourceHeight_ = 0;
};