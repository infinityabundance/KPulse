#include "journald_reader.hpp"

#include <QDebug>
#include <QByteArray>
#include <QTimeZone>

#include <systemd/sd-journal.h>

#include "kpulse/common.hpp"

namespace kpulse {

namespace {

QString readField(sd_journal *journal, const char *field)
{
    const void *data = nullptr;
    size_t len = 0;
    int r = sd_journal_get_data(journal, field, &data, &len);
    if (r < 0 || !data || len == 0) {
        return {};
    }

    QByteArray ba(static_cast<const char *>(data), static_cast<int>(len));
    int idx = ba.indexOf('=');
    if (idx < 0) {
        return {};
    }
    return QString::fromUtf8(ba.constData() + idx + 1,
                             ba.size() - idx - 1);
}

} // namespace

JournaldReader::JournaldReader(QObject *parent)
    : QObject(parent)
{
}

JournaldReader::~JournaldReader()
{
    stop();
}

bool JournaldReader::start()
{
    if (journal_) {
        return true;
    }

    int r = sd_journal_open(&journal_, SD_JOURNAL_LOCAL_ONLY);
    if (r < 0) {
        qWarning() << "JournaldReader: failed to open journal:" << r;
        journal_ = nullptr;
        return false;
    }

    // Seek to the end so we only see new events.
    sd_journal_seek_tail(journal_);
    sd_journal_next(journal_);

    if (!timer_) {
        timer_ = new QTimer(this);
        timer_->setInterval(1000); // 1s poll
        connect(timer_, &QTimer::timeout,
                this, &JournaldReader::poll);
    }
    timer_->start();

    qInfo() << "JournaldReader: started tailing systemd journal";
    return true;
}

void JournaldReader::stop()
{
    if (timer_) {
        timer_->stop();
    }
    if (journal_) {
        sd_journal_close(journal_);
        journal_ = nullptr;
    }
}

void JournaldReader::poll()
{
    if (!journal_) {
        return;
    }

    Event ev;
    // Drain all pending entries each tick.
    while (readNextEvent(ev)) {
        emit eventDetected(ev);
    }
}

Severity JournaldReader::severityFromPriority(int prio)
{
    // systemd priorities: 0..7 (emerg..debug)
    if (prio <= 2) {
        return Severity::Critical;
    }
    if (prio <= 3) {
        return Severity::Error;
    }
    if (prio <= 4) {
        return Severity::Warning;
    }
    return Severity::Info;
}

bool JournaldReader::classifyMessage(const QString &message,
                                     Category &outCategory,
                                     Severity &outSeverity,
                                     QString &outLabel)
{
    const QString lower = message.toLower();

    // Very rough heuristics, we can refine later.

    // GPU hangs / resets / timeouts
    if ((lower.contains("gpu") || lower.contains("amdgpu") || lower.contains("nvidia")) &&
        (lower.contains("hang") || lower.contains("reset") ||
         lower.contains("fault") || lower.contains("timeout"))) {
        outCategory = Category::GPU;
        outSeverity = Severity::Error;
        outLabel = QStringLiteral("GPU hang/reset");
        return true;
    }

    // Thermal throttling
    if (lower.contains("thermal") ||
        lower.contains("throttle") ||
        lower.contains("temperature above threshold")) {
        outCategory = Category::Thermal;
        outSeverity = Severity::Warning;
        outLabel = QStringLiteral("Thermal throttling");
        return true;
    }

    // OOM killer / out of memory
    if (lower.contains("oom-killer") ||
        lower.contains("out of memory")) {
        outCategory = Category::System;
        outSeverity = Severity::Critical;
        outLabel = QStringLiteral("Out-of-memory condition");
        return true;
    }

    // Soft lockups / watchdog
    if (lower.contains("soft lockup") ||
        lower.contains("watchdog: bug: soft lockup")) {
        outCategory = Category::System;
        outSeverity = Severity::Error;
        outLabel = QStringLiteral("CPU soft lockup");
        return true;
    }

    return false;
}

bool JournaldReader::readNextEvent(Event &outEvent)
{
    int r = sd_journal_next(journal_);
    if (r < 0) {
        qWarning() << "JournaldReader: sd_journal_next failed:" << r;
        return false;
    }
    if (r == 0) {
        // No more entries right now.
        return false;
    }

    // Timestamp
    uint64_t usec = 0;
    r = sd_journal_get_realtime_usec(journal_, &usec);
    if (r < 0) {
        outEvent.timestamp = nowUtc();
    } else {
        const qint64 msec = static_cast<qint64>(usec / 1000);
        outEvent.timestamp = QDateTime::fromMSecsSinceEpoch(
            msec,
            QTimeZone::utc()
        );
    }

    // Basic fields
    const QString message = readField(journal_, "MESSAGE");
    const QString unit = readField(journal_, "_SYSTEMD_UNIT");
    const QString ident = readField(journal_, "SYSLOG_IDENTIFIER");
    const QString prioStr = readField(journal_, "PRIORITY");

    int prio = 5; // default to "notice/info-ish"
    bool ok = false;
    int tmp = prioStr.toInt(&ok);
    if (ok) {
        prio = tmp;
    }

    Category cat = Category::System;
    Severity sev = severityFromPriority(prio);
    QString label;

    if (!classifyMessage(message, cat, sev, label)) {
        // Fallback: generic mapping
        cat = Category::System;
        label = message.left(120);
    }

    outEvent.category = cat;
    outEvent.severity = sev;
    outEvent.label = label;

    QJsonObject details;
    if (!message.isEmpty())
        details.insert(QStringLiteral("message"), message);
    if (!unit.isEmpty())
        details.insert(QStringLiteral("unit"), unit);
    if (!ident.isEmpty())
        details.insert(QStringLiteral("identifier"), ident);
    details.insert(QStringLiteral("priority"), prio);

    outEvent.details = details;

    return true;
}

} // namespace kpulse
