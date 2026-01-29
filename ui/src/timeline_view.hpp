#pragma once

#include <QWidget>
#include <vector>

#include "kpulse/event.hpp"

class TimelineView : public QWidget
{
    Q_OBJECT
public:
    explicit TimelineView(QWidget *parent = nullptr);

    void setEvents(const std::vector<kpulse::Event> &events);

signals:
    void eventClicked(const kpulse::Event &event);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    QSize minimumSizeHint() const override;

private:
    std::vector<kpulse::Event> events_;

    qint64 minTimestampMs_ = 0;
    qint64 maxTimestampMs_ = 0;

    QRectF laneRectForCategory(kpulse::Category cat, const QRectF &fullRect) const;
    QColor colorForEvent(const kpulse::Event &ev) const;
    void updateTimeBounds();
    const kpulse::Event *eventAtPosition(const QPoint &pos) const;
};
