#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include "kpulse/db.hpp"
#include "kpulse/event.hpp"

#include "journald_reader.hpp"
#include "metrics_collector.hpp"
#include "baseline_tracker.hpp"

namespace kpulse {

class KPulseDaemon : public QObject
{
    Q_OBJECT
public:
    explicit KPulseDaemon(const QString &dbPath, QObject *parent = nullptr);
    ~KPulseDaemon() override = default;

    bool init();

public slots:
    QString GetEvents(qint64 fromMs, qint64 toMs, const QStringList &categories);

signals:
    void EventAdded(const QString &eventJson);

private slots:
    void handleEventDetected(const kpulse::Event &event);

private:
    QString dbPath_;
    EventStore store_;
    JournaldReader journald_;
    MetricsCollector metrics_;
    BaselineTracker baseline_;
};

} // namespace kpulse
