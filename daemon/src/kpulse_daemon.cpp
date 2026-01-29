#include "kpulse_daemon.hpp"

#include "kpulse/common.hpp"

#include <QDBusConnection>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>

#include "kpulse_daemon_adaptor.h"

namespace kpulse {

KPulseDaemon::KPulseDaemon(const QString &dbPath, QObject *parent)
    : QObject(parent)
    , dbPath_(dbPath)
    , store_(dbPath)
    , journald_(this)
    , metrics_(this)
{
    // Wire daemon to sources of events
    connect(&journald_, &JournaldReader::eventDetected,
            this, &KPulseDaemon::handleEventDetected);
    connect(&metrics_, &MetricsCollector::eventDetected,
            this, &KPulseDaemon::handleEventDetected);

    // Set up DBus adaptor and object registration.
    auto *adaptor = new DaemonAdaptor(this);
    Q_UNUSED(adaptor);

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerObject(QStringLiteral("/org/kde/kpulse/Daemon"), this)) {
        qWarning() << "KPulseDaemon: failed to register DBus object";
    }
    if (!bus.registerService(QStringLiteral("org.kde.kpulse.Daemon"))) {
        qWarning() << "KPulseDaemon: failed to register DBus service";
    }
}

bool KPulseDaemon::init()
{
    if (!store_.open()) {
        qWarning() << "KPulseDaemon: failed to open event store at" << dbPath_;
        return false;
    }
    if (!store_.initSchema()) {
        qWarning() << "KPulseDaemon: failed to initialise schema";
        return false;
    }

    // Phase 19: start journald tailing so real system events feed into KPulse.
    if (!journald_.start()) {
        qWarning() << "KPulseDaemon: journald reader failed to start";
        // Not fatal: KPulse still works via InjectTestEvent/other sources.
    }

    // MetricsCollector can be wired up later.
    return true;
}

QString KPulseDaemon::GetEvents(qlonglong fromMs,
                                qlonglong toMs,
                                const QStringList &categories)
{
    // Convert from/to to UTC QDateTime
    QDateTime from = QDateTime::fromMSecsSinceEpoch(fromMs, QTimeZone::utc());
    QDateTime to   = QDateTime::fromMSecsSinceEpoch(toMs,   QTimeZone::utc());

    // Map category names (strings) to enum Category
    std::vector<Category> cats;
    cats.reserve(categories.size());
    for (const QString &name : categories) {
        cats.push_back(categoryFromString(name));
    }

    // Query the event store
    const auto events = store_.queryEvents(from, to, cats);

    // Serialize events as a JSON array
    QJsonArray arr;
    for (const auto &ev : events) {
        arr.push_back(eventToJson(ev));
    }

    QJsonDocument doc(arr);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

void KPulseDaemon::InjectTestEvent(const QString &category,
                                   const QString &severity,
                                   const QString &label,
                                   const QString &detailsJson)
{
    Event ev;
    ev.timestamp = nowUtc();
    ev.category  = categoryFromString(category);
    ev.severity  = severityFromString(severity);
    ev.label     = label;

    if (!detailsJson.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(detailsJson.toUtf8());
        if (doc.isObject()) {
            ev.details = doc.object();
        }
    }

    handleEventDetected(ev);
}

void KPulseDaemon::handleEventDetected(const kpulse::Event &event)
{
    Event eventCopy = event;
    if (!eventCopy.timestamp.isValid()) {
        eventCopy.timestamp = nowUtc();
    }

    qint64 id = 0;
    if (!store_.insertEvent(eventCopy, &id)) {
        qWarning() << "KPulseDaemon: failed to store event";
        return;
    }

    // Emit DBus-visible signal with the stored event JSON.
    QJsonObject obj = eventToJson(eventCopy);
    QJsonDocument doc(obj);
    const QString json = QString::fromUtf8(
        doc.toJson(QJsonDocument::Compact)
    );
    emit EventAdded(json);
}

} // namespace kpulse
