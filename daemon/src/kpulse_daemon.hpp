#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>

#include "kpulse/db.hpp"
#include "kpulse/event.hpp"

#include "journald_reader.hpp"
#include "metrics_collector.hpp"

namespace kpulse {

class KPulseDaemon : public QObject
{
    Q_OBJECT
public:
    explicit KPulseDaemon(const QString &dbPath, QObject *parent = nullptr);

    // Initialise the event store and any other resources.
    bool init();

    // DBus-exposed method used by the generated DaemonAdaptor.
    // Returns a JSON array (as a compact string) of events that fall within
    // the given time range and category filter.
    QString GetEvents(qlonglong fromMs,
                      qlonglong toMs,
                      const QStringList &categories);

signals:
    // Emitted whenever a new event is stored. The DBus adaptor maps this
    // to the "EventAdded" signal defined in dbus_interface.xml.
    void EventAdded(const QString &eventJson);

private slots:
    void handleEventDetected(const kpulse::Event &event);

private:
    QString         dbPath_;
    EventStore      store_;
    JournaldReader  journald_;
    MetricsCollector metrics_;
};

} // namespace kpulse
