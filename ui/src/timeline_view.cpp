#include "timeline_view.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <algorithm>

TimelineView::TimelineView(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(160);
}

void TimelineView::setEvents(const std::vector<kpulse::Event> &events)
{
    events_ = events;
    updateTimeBounds();
    update();
}

void TimelineView::appendEvent(const kpulse::Event &event)
{
    events_.push_back(event);
    updateTimeBounds();
    update();
}

void TimelineView::updateTimeBounds()
{
    if (events_.empty()) {
        minTimestampMs_ = 0;
        maxTimestampMs_ = 0;
        return;
    }

    qint64 minTs = events_.front().timestamp.toMSecsSinceEpoch();
    qint64 maxTs = minTs;

    for (const auto &ev : events_) {
        const qint64 ts = ev.timestamp.toMSecsSinceEpoch();
        minTs = std::min(minTs, ts);
        maxTs = std::max(maxTs, ts);
    }

    if (minTs == maxTs) {
        minTs -= 5000;
        maxTs += 5000;
    }

    minTimestampMs_ = minTs;
    maxTimestampMs_ = maxTs;
}

QSize TimelineView::minimumSizeHint() const
{
    return QSize(200, 160);
}

QRectF TimelineView::laneRectForCategory(kpulse::Category cat, const QRectF &fullRect) const
{
    const int laneCount = 6;
    const qreal laneHeight = fullRect.height() / laneCount;

    int index = 0;
    switch (cat) {
    case kpulse::Category::System:
        index = 0;
        break;
    case kpulse::Category::GPU:
        index = 1;
        break;
    case kpulse::Category::Thermal:
        index = 2;
        break;
    case kpulse::Category::Process:
        index = 3;
        break;
    case kpulse::Category::Update:
        index = 4;
        break;
    case kpulse::Category::Network:
        index = 5;
        break;
    }

    QRectF lane(fullRect.left(), fullRect.top() + index * laneHeight, fullRect.width(), laneHeight);
    return lane.adjusted(0, 4, 0, -4);
}

QColor TimelineView::colorForEvent(const kpulse::Event &ev) const
{
    switch (ev.severity) {
    case kpulse::Severity::Info:
        return QColor(100, 180, 255);
    case kpulse::Severity::Warning:
        return QColor(255, 200, 100);
    case kpulse::Severity::Error:
        return QColor(255, 120, 120);
    case kpulse::Severity::Critical:
        return QColor(200, 40, 40);
    }
    return Qt::gray;
}

void TimelineView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = rect();
    p.fillRect(r, palette().base());

    if (events_.empty() || minTimestampMs_ == maxTimestampMs_) {
        p.setPen(palette().text().color());
        p.drawText(r, Qt::AlignCenter, tr("No events in selected range"));
        return;
    }

    p.setPen(QPen(palette().mid().color(), 1, Qt::DashLine));
    for (int i = 0; i < 6; ++i) {
        QRectF lane = laneRectForCategory(static_cast<kpulse::Category>(i), r);
        p.drawLine(QPointF(r.left(), lane.center().y()), QPointF(r.right(), lane.center().y()));
    }

    const qreal margin = 8.0;
    QRectF inner = r.adjusted(margin, margin, -margin, -margin);
    const qreal radius = 4.0;
    const qreal timeSpan = static_cast<qreal>(maxTimestampMs_ - minTimestampMs_);

    for (const auto &ev : events_) {
        const qint64 ts = ev.timestamp.toMSecsSinceEpoch();
        const qreal tNorm = (ts - minTimestampMs_) / timeSpan;

        QRectF lane = laneRectForCategory(ev.category, inner);
        const qreal x = inner.left() + tNorm * inner.width();
        const qreal y = lane.center().y();

        p.setBrush(colorForEvent(ev));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(x, y), radius, radius);
    }
}

const kpulse::Event *TimelineView::eventAtPosition(const QPoint &pos) const
{
    if (events_.empty() || minTimestampMs_ == maxTimestampMs_) {
        return nullptr;
    }

    const QRectF r = rect();
    const qreal margin = 8.0;
    QRectF inner = r.adjusted(margin, margin, -margin, -margin);
    const qreal timeSpan = static_cast<qreal>(maxTimestampMs_ - minTimestampMs_);
    const qreal radius = 6.0;

    for (const auto &ev : events_) {
        const qint64 ts = ev.timestamp.toMSecsSinceEpoch();
        const qreal tNorm = (ts - minTimestampMs_) / timeSpan;

        QRectF lane = laneRectForCategory(ev.category, inner);
        const qreal x = inner.left() + tNorm * inner.width();
        const qreal y = lane.center().y();

        if (QLineF(QPointF(x, y), pos).length() <= radius) {
            return &ev;
        }
    }

    return nullptr;
}

void TimelineView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    const kpulse::Event *ev = eventAtPosition(event->pos());
    if (ev) {
        emit eventClicked(*ev);
        return;
    }

    QWidget::mousePressEvent(event);
}
