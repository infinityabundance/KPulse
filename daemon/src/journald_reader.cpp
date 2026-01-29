#include "journald_reader.hpp"

#include <QDateTime>
#include <QJsonObject>
#include <QTimer>
#include <QtGlobal>

namespace kpulse {

namespace {
QString readField(sd_journal *journal, const char *field)
{
    const void *data = nullptr;
    size_t length = 0;
    const int result = sd_journal_get_data(journal, field, &data, &length);
    if (result < 0 || data == nullptr || length == 0) {
        return {};
    }

    const QByteArray raw(static_cast<const char *>(data), static_cast<int>(length));
    const QByteArray fieldBytes(field);
    const QByteArray prefix = fieldBytes + '=';
    if (raw.startsWith(prefix)) {
        return QString::fromUtf8(raw.mid(prefix.size()));
    }

    return QString::fromUtf8(raw);
}

Severity priorityToSeverity(int priority)
{
    if (priority <= 3) {
        return Severity::Error;
    }
    if (priority == 4) {
        return Severity::Warning;
    }
    return Severity::Info;
}
}

JournaldReader::JournaldReader(QObject *parent)
    : QObject(parent)
{
}

JournaldReader::~JournaldReader()
{
    if (journal_ != nullptr) {
        sd_journal_close(journal_);
        journal_ = nullptr;
    }
}

bool JournaldReader::init()
{
    if (initialized_) {
        return true;
    }

    const int result = sd_journal_open(&journal_, SD_JOURNAL_LOCAL_ONLY);
    if (result < 0) {
        qWarning() << "Failed to open systemd journal" << result;
        return false;
    }

    sd_journal_seek_tail(journal_);
    sd_journal_next(journal_);

    initialized_ = true;
    return true;
}

void JournaldReader::start()
{
    if (!initialized_) {
        return;
    }

    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &JournaldReader::poll);
    timer->start(1000);
}

void JournaldReader::poll()
{
    if (!initialized_ || journal_ == nullptr) {
        return;
    }

    while (sd_journal_next(journal_) > 0) {
        Event event;
        if (readNextEvent(event)) {
            emit eventDetected(event);
        }
    }
}

bool JournaldReader::readNextEvent(Event &outEvent)
{
    if (journal_ == nullptr) {
        return false;
    }

    uint64_t usec = 0;
    if (sd_journal_get_realtime_usec(journal_, &usec) < 0) {
        return false;
    }

    const QString message = readField(journal_, "MESSAGE");
    const QString unit = readField(journal_, "_SYSTEMD_UNIT");
    QString label = unit;
    if (label.isEmpty()) {
        label = readField(journal_, "SYSLOG_IDENTIFIER");
    }
    if (label.isEmpty()) {
        label = QStringLiteral("system");
    }

    const QString priorityStr = readField(journal_, "PRIORITY");
    bool ok = false;
    const int priority = priorityStr.toInt(&ok);

    const qint64 msec = static_cast<qint64>(usec / 1000);
    outEvent.timestamp = QDateTime::fromMSecsSinceEpoch(msec, Qt::UTC);
    outEvent.category = Category::System;
    outEvent.severity = priorityToSeverity(ok ? priority : 5);
    outEvent.label = label;

    QJsonObject details;
    details.insert(QStringLiteral("message"), message);
    details.insert(QStringLiteral("unit"), unit);
    if (ok) {
        details.insert(QStringLiteral("priority"), priority);
    }
    outEvent.details = details;

    return true;
}

} // namespace kpulse
