#pragma once

#include <QWidget>
#include <QVector>

#include "kpulse/event.hpp"

class TimelineView : public QWidget
{
    Q_OBJECT
public:
    explicit TimelineView(QWidget *parent = nullptr);

    // Replace full event set.
    void setEvents(const QVector<kpulse::Event> &events);

    // Append a single event.
    void appendEvent(const kpulse::Event &ev);

signals:
    // Index in the current event list, or -1 when nothing is hovered.
    void eventHovered(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QVector<kpulse::Event> events_;
    int hoveredIndex_ = -1;

    int hitTest(const QPoint &pos) const;
};
