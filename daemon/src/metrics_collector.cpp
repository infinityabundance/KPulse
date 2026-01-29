#include "metrics_collector.hpp"

#include <QFile>
#include <QJsonObject>
#include <QtGlobal>

namespace kpulse {

MetricsCollector::MetricsCollector(QObject *parent)
    : QObject(parent)
{
    timer_.setParent(this);
    connect(&timer_, &QTimer::timeout, this, &MetricsCollector::sample);
}

void MetricsCollector::start(int intervalMs)
{
    timer_.start(intervalMs);
}

void MetricsCollector::sample()
{
    QFile file(QStringLiteral("/proc/loadavg"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    const QByteArray line = file.readLine();
    const QList<QByteArray> parts = line.split(' ');
    if (parts.isEmpty()) {
        return;
    }

    bool ok = false;
    const double load = parts.at(0).toDouble(&ok);
    if (!ok) {
        return;
    }

    if (load <= 4.0) {
        return;
    }

    Event event;
    event.timestamp = QDateTime::currentDateTimeUtc();
    event.category = Category::Process;
    event.severity = Severity::Warning;
    event.label = QStringLiteral("High CPU load");

    QJsonObject details;
    details.insert(QStringLiteral("loadavg_1min"), load);
    event.details = details;

    emit eventDetected(event);
}

} // namespace kpulse
