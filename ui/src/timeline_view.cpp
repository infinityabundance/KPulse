#include "timeline_view.hpp"

#include <QDateTime>
#include <QMouseEvent>
#include <QPainter>
#include <QTimeZone>
#include <QToolTip>
#include <QCursor>
#include <algorithm>

using kpulse::Event;
using kpulse::Category;
using kpulse::Severity;

TimelineView::TimelineView(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(120);
    setMouseTracking(true);
}

void TimelineView::setEvents(const QVector<Event> &events)
{
    events_ = events;
    hoveredIndex_ = -1;
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

    // Plot events as circles
    for (int i = 0; i < events_.size(); ++i) {
        const Event &ev = events_.at(i);
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

        const bool hovered = (i == hoveredIndex_);

        p.setPen(Qt::NoPen);
        p.setBrush(color);

        double radius = hovered ? 6.0 : 4.0;
        p.drawEllipse(QPointF(x, y), radius, radius);

        if (hovered) {
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(Qt::white, 1.0));
            p.drawEllipse(QPointF(x, y), radius + 2.0, radius + 2.0);
        }
    }
}

int TimelineView::hitTest(const QPoint &pos) const
{
    if (events_.isEmpty())
        return -1;

    const QRect r = rect().adjusted(8, 8, -8, -8);

    // Find time bounds
    qint64 minMs = std::numeric_limits<qint64>::max();
    qint64 maxMs = std::numeric_limits<qint64>::min();

    for (const Event &ev : events_) {
        const qint64 ms = ev.timestamp.toMSecsSinceEpoch();
        if (ms < minMs) minMs = ms;
        if (ms > maxMs) maxMs = ms;
    }

    if (maxMs == minMs) {
        maxMs = minMs + 1000;
    }

    const int lanes = 6;
    const double laneHeight = r.height() / double(lanes);

    const double hitRadius = 7.0;
    const double hitRadiusSq = hitRadius * hitRadius;

    int bestIndex = -1;
    double bestDistSq = std::numeric_limits<double>::max();

    for (int i = 0; i < events_.size(); ++i) {
        const Event &ev = events_.at(i);
        const qint64 ms = ev.timestamp.toMSecsSinceEpoch();
        const double tNorm = double(ms - minMs) / double(maxMs - minMs);
        const double x = r.left() + tNorm * r.width();

        const int idx = categoryIndex(ev.category);
        const double y = r.top() + laneHeight * (idx + 0.5);

        const double dx = pos.x() - x;
        const double dy = pos.y() - y;
        const double distSq = dx * dx + dy * dy;

        if (distSq <= hitRadiusSq && distSq < bestDistSq) {
            bestDistSq = distSq;
            bestIndex = i;
        }
    }

    return bestIndex;
}

void TimelineView::mouseMoveEvent(QMouseEvent *event)
{
    const int idx = hitTest(event->pos());
    if (idx != hoveredIndex_) {
        hoveredIndex_ = idx;
        emit eventHovered(hoveredIndex_);
        update();

        if (hoveredIndex_ >= 0 && hoveredIndex_ < events_.size()) {
            const Event &ev = events_.at(hoveredIndex_);
            const QString text = QStringLiteral("%1 | %2 | %3 | %4")
                                     .arg(ev.timestamp.toString(Qt::ISODateWithMs))
                                     .arg(kpulse::categoryToString(ev.category))
                                     .arg(kpulse::severityToString(ev.severity))
                                     .arg(ev.label);
            QToolTip::showText(QCursor::pos(), text, this);
        } else {
            QToolTip::hideText();
        }
    }

    QWidget::mouseMoveEvent(event);
}

void TimelineView::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    if (hoveredIndex_ != -1) {
        hoveredIndex_ = -1;
        emit eventHovered(-1);
        update();
        QToolTip::hideText();
    }
}
