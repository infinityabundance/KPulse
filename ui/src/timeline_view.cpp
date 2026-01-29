#include "timeline_view.hpp"

#include <QPainter>
#include <QDateTime>
#include <QTimeZone>
#include <algorithm>

using kpulse::Event;
using kpulse::Category;
using kpulse::Severity;

TimelineView::TimelineView(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(120);
}

void TimelineView::setEvents(const QVector<Event> &events)
{
    events_ = events;
    update();
}

void TimelineView::appendEvent(const Event &ev)
{
    events_.push_back(ev);
    update();
}

static int categoryIndex(Category c)
{
    switch (c) {
    case Category::GPU:     return 0;
    case Category::Thermal: return 1;
    case Category::Process: return 2;
    case Category::Network: return 3;
    case Category::System:  return 4;
    default:                return 5;
    }
}

void TimelineView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRect r = rect().adjusted(8, 8, -8, -8);
    if (events_.isEmpty()) {
        p.drawText(r, Qt::AlignCenter, QStringLiteral("No events in range"));
        return;
    }

    // Find time bounds
    qint64 minMs = std::numeric_limits<qint64>::max();
    qint64 maxMs = std::numeric_limits<qint64>::min();

    for (const Event &ev : events_) {
        const qint64 ms = ev.timestamp.toMSecsSinceEpoch();
        if (ms < minMs) minMs = ms;
        if (ms > maxMs) maxMs = ms;
    }

    if (minMs == std::numeric_limits<qint64>::max() ||
        maxMs == std::numeric_limits<qint64>::min()) {
        p.drawText(r, Qt::AlignCenter, QStringLiteral("No events in range"));
        return;
    }

    if (maxMs == minMs) {
        maxMs = minMs + 1000; // avoid div-by-zero
    }

    const int lanes = 6;
    const double laneHeight = r.height() / double(lanes);

    // Draw lane lines
    p.setPen(QPen(Qt::gray, 1, Qt::DashLine));
    for (int i = 0; i < lanes; ++i) {
        int y = int(r.top() + laneHeight * (i + 0.5));
        p.drawLine(r.left(), y, r.right(), y);
    }

    // Plot events as small circles
    for (const Event &ev : events_) {
        const qint64 ms = ev.timestamp.toMSecsSinceEpoch();
        const double tNorm = double(ms - minMs) / double(maxMs - minMs);
        const double x = r.left() + tNorm * r.width();

        const int idx = categoryIndex(ev.category);
        const double y = r.top() + laneHeight * (idx + 0.5);

        QColor color = Qt::gray;
        switch (ev.category) {
        case Category::GPU:     color = QColor(200, 80, 80);  break;
        case Category::Thermal: color = QColor(200, 150, 80); break;
        case Category::Process: color = QColor(80, 160, 200); break;
        case Category::Network: color = QColor(120, 120, 220); break;
        case Category::System:  color = QColor(150, 150, 150); break;
        default:                color = QColor(160, 160, 160); break;
        }

        // Darker for more severe
        if (ev.severity == Severity::Warning) {
            color = color.darker(110);
        } else if (ev.severity == Severity::Error ||
                   ev.severity == Severity::Critical) {
            color = color.darker(140);
        }

        p.setPen(Qt::NoPen);
        p.setBrush(color);
        const double radius = 4.0;
        p.drawEllipse(QPointF(x, y), radius, radius);
    }
}
