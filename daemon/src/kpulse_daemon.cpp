#include "kpulse_daemon.hpp"

#include "kpulse/common.hpp"

#include <QtGlobal>

namespace kpulse {

KPulseDaemon::KPulseDaemon(const QString &dbPath, QObject *parent)
    : QObject(parent)
    , dbPath_(dbPath)
    , store_(dbPath)
    , journald_(this)
    , metrics_(this)
{
    connect(&journald_, &JournaldReader::eventDetected, this, &KPulseDaemon::handleEventDetected);
    connect(&metrics_, &MetricsCollector::eventDetected, this, &KPulseDaemon::handleEventDetected);
}

bool KPulseDaemon::init()
{
    if (!store_.open()) {
        qWarning() << "Failed to open KPulse database";
        return false;
    }

    if (!store_.initSchema()) {
        qWarning() << "Failed to initialize KPulse database schema";
        return false;
    }

    if (!journald_.init()) {
        qWarning() << "Failed to initialize journald reader";
        return false;
    }

    journald_.start();
    metrics_.start();

    return true;
}

void KPulseDaemon::handleEventDetected(const kpulse::Event &event)
{
    Event eventCopy = event;
    if (!eventCopy.timestamp.isValid()) {
        eventCopy.timestamp = nowUtc();
    }

    qint64 id = 0;
    if (!store_.insertEvent(eventCopy, &id)) {
        qWarning() << "Failed to store KPulse event";
        return;
    }
}

} // namespace kpulse
