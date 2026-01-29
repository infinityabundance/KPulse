#pragma once

#include <QObject>
#include <QTimer>

#include "kpulse/event.hpp"

namespace kpulse {

class MetricsCollector : public QObject
{
    Q_OBJECT
public:
    explicit MetricsCollector(QObject *parent = nullptr);

    void start(int intervalMs = 5000);

signals:
    void eventDetected(const kpulse::Event &event);

private slots:
    void sample();

private:
    QTimer timer_;
};

} // namespace kpulse
