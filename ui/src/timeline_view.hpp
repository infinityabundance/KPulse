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

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<kpulse::Event> events_;
};
