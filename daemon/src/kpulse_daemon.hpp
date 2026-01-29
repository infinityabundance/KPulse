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
    ~KPulseDaemon() override = default;

    bool init();

private slots:
    void handleEventDetected(const kpulse::Event &event);

private:
    QString dbPath_;
    EventStore store_;
    JournaldReader journald_;
    MetricsCollector metrics_;
};

} // namespace kpulse
