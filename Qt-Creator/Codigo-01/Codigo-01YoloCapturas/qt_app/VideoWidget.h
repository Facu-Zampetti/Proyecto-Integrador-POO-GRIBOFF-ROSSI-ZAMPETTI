#pragma once

#include <QImage>
#include <QRect>
#include <QVector>
#include <QWidget>

class QMouseEvent;
class QPaintEvent;

struct DetectionBox {
    int classId = -1;
    QString label;
    double confidence = 0.0;
    QRect bbox;
    qint64 trackId = -1;
};

using DetectionList = QVector<DetectionBox>;

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