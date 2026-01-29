#pragma once

#include <QObject>
#include <QProcess>

#include "kpulse/event.hpp"

namespace kpulse {

class JournaldReader : public QObject
{
    Q_OBJECT
public:
    explicit JournaldReader(QObject *parent = nullptr);
    ~JournaldReader() override;

    // Start tailing the systemd journal via `journalctl -f -o json`.
    // Returns false if the process cannot be started.
    bool start();

    // Stop reading and release resources.
    void stop();

signals:
    // Emitted whenever we detect an interesting event in the journal.
    void eventDetected(const kpulse::Event &event);

private slots:
    void handleReadyRead();
    void handleFinished(int exitCode, QProcess::ExitStatus status);

private:
    void processLine(const QByteArray &line);

    static Severity severityFromPriority(int prio);
    static bool classifyMessage(const QString &message,
                                Category &outCategory,
                                Severity &outSeverity,
                                QString &outLabel);

    QProcess *process_ = nullptr;
    QByteArray buffer_;
};

} // namespace kpulse
