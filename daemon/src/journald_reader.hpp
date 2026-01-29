#pragma once

#include <QObject>
#include <memory>

#include <systemd/sd-journal.h>

#include "kpulse/event.hpp"

namespace kpulse {

class JournaldReader : public QObject
{
    Q_OBJECT
public:
    explicit JournaldReader(QObject *parent = nullptr);
    ~JournaldReader() override;

    bool init();
    void start();

signals:
    void eventDetected(const kpulse::Event &event);

private slots:
    void poll();

private:
    sd_journal *journal_ = nullptr;
    bool initialized_ = false;

    bool readNextEvent(Event &outEvent);
};

} // namespace kpulse
