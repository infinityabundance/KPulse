#pragma once

#include <QObject>
#include <QTimer>

#include "kpulse/event.hpp"

struct sd_journal; // from <systemd/sd-journal.h>

namespace kpulse {

class JournaldReader : public QObject
{
    Q_OBJECT
public:
    explicit JournaldReader(QObject *parent = nullptr);
    ~JournaldReader() override;

    // Start tailing the systemd journal from "now".
    // Returns false if the journal cannot be opened.
    bool start();

    // Stop reading and release resources.
    void stop();

signals:
    // Emitted whenever we detect an interesting event in the journal.
    void eventDetected(const kpulse::Event &event);

private slots:
    // Periodic poll driven by a QTimer.
    void poll();

private:
    bool readNextEvent(Event &outEvent);

    static Severity severityFromPriority(int prio);
    static bool classifyMessage(const QString &message,
                                Category &outCategory,
                                Severity &outSeverity,
                                QString &outLabel);

    sd_journal *journal_ = nullptr;
    QTimer *timer_ = nullptr;
};

} // namespace kpulse
