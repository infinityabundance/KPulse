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

static QColor categoryColor(Category c)
{
    switch (c) {
    case Category::GPU:     return QColor(200, 80, 80);
    case Category::Thermal: return QColor(200, 150, 80);
    case Category::Process: return QColor(80, 160, 200);
    case Category::Network: return QColor(120, 120, 220);
    case Category::System:  return QColor(150, 150, 150);
    default:                return QColor(160, 160, 160);
    }
}

static QRect plotRectForTimeline(const QRect &outer, bool *hasLegend = nullptr)
{
    const QRect full = outer.adjusted(8, 8, -8, -8);
    const int legendHeight = 22;
    const int legendGap = 6;

    if (full.height() > legendHeight + legendGap + 40) {
        if (hasLegend) {
            *hasLegend = true;
        }
        QRect plot = full;
        plot.setBottom(full.bottom() - legendHeight - legendGap);
        return plot;
    }

    if (hasLegend) {
        *hasLegend = false;
    }
    return full;
}

void TimelineView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    bool hasLegend = false;
    const QRect plotRect = plotRectForTimeline(rect(), &hasLegend);
    if (events_.isEmpty()) {
        p.drawText(plotRect, Qt::AlignCenter, QStringLiteral("No events in range"));
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
        p.drawText(plotRect, Qt::AlignCenter, QStringLiteral("No events in range"));
        return;
    }

    if (maxMs == minMs) {
        maxMs = minMs + 1000; // avoid div-by-zero
    }

    const int lanes = 6;
    const double laneHeight = plotRect.height() / double(lanes);

    // Draw lane lines
    p.setPen(QPen(Qt::gray, 1, Qt::DashLine));
    for (int i = 0; i < lanes; ++i) {
        int y = int(plotRect.top() + laneHeight * (i + 0.5));
        p.drawLine(plotRect.left(), y, plotRect.right(), y);
    }

    // Plot events as circles
    for (int i = 0; i < events_.size(); ++i) {
        const Event &ev = events_.at(i);
        const qint64 ms = ev.timestamp.toMSecsSinceEpoch();
        const double tNorm = double(ms - minMs) / double(maxMs - minMs);
        const double x = plotRect.left() + tNorm * plotRect.width();

        const int idx = categoryIndex(ev.category);
        const double y = plotRect.top() + laneHeight * (idx + 0.5);

        QColor color = categoryColor(ev.category);

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

    if (hasLegend) {
        const QRect full = rect().adjusted(8, 8, -8, -8);
        const int legendHeight = 22;
        const int legendGap = 6;
        const QRect legendRect(full.left(), plotRect.bottom() + legendGap,
                               full.width(), legendHeight);

        struct LegendItem {
            Category category;
            const char *label;
        };

        const LegendItem items[] = {
            {Category::GPU, "GPU"},
            {Category::Thermal, "Thermal"},
            {Category::Process, "Process"},
            {Category::Network, "Network"},
            {Category::System, "System"},
            {Category::Unknown, "Other"},
        };

        int x = legendRect.left();
        const int centerY = legendRect.center().y();
        const int dotRadius = 4;
        const int gap = 10;
        const int textOffset = 6;

        QFontMetrics fm(font());
        for (const auto &item : items) {
            const QString label = QString::fromUtf8(item.label);
            const int textWidth = fm.horizontalAdvance(label);

            p.setPen(Qt::NoPen);
            p.setBrush(categoryColor(item.category));
            p.drawEllipse(QPointF(x + dotRadius, centerY), dotRadius, dotRadius);

            p.setPen(palette().text().color());
            p.drawText(x + dotRadius + textOffset, centerY + fm.ascent() / 2, label);

            x += dotRadius * 2 + textOffset + textWidth + gap;
            if (x > legendRect.right()) {
                break;
            }
        }
    }
}

int TimelineView::hitTest(const QPoint &pos) const
{
    if (events_.isEmpty())
        return -1;

    const QRect plotRect = plotRectForTimeline(rect());

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
    const double laneHeight = plotRect.height() / double(lanes);

    const double hitRadius = 7.0;
    const double hitRadiusSq = hitRadius * hitRadius;

    int bestIndex = -1;
    double bestDistSq = std::numeric_limits<double>::max();

    for (int i = 0; i < events_.size(); ++i) {
        const Event &ev = events_.at(i);
        const qint64 ms = ev.timestamp.toMSecsSinceEpoch();
        const double tNorm = double(ms - minMs) / double(maxMs - minMs);
        const double x = plotRect.left() + tNorm * plotRect.width();

        const int idx = categoryIndex(ev.category);
        const double y = plotRect.top() + laneHeight * (idx + 0.5);

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
