#include "journald_reader.hpp"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTimeZone>

#include "kpulse/common.hpp"

namespace kpulse {

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
    if (process_) {
        return true;
    }

    process_ = new QProcess(this);

    // journalctl -f -o json : follow journal as JSON lines
    QStringList args;
    args << QStringLiteral("-f")
         << QStringLiteral("-o") << QStringLiteral("json");

    process_->setProgram(QStringLiteral("journalctl"));
    process_->setArguments(args);
    process_->setProcessChannelMode(QProcess::MergedChannels);

    connect(process_, &QProcess::readyReadStandardOutput,
            this, &JournaldReader::handleReadyRead);
    connect(process_,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            &JournaldReader::handleFinished);

    process_->start();
    if (!process_->waitForStarted(3000)) {
        qWarning() << "JournaldReader: failed to start journalctl process";
        delete process_;
        process_ = nullptr;
        return false;
    }

    qInfo() << "JournaldReader: started journalctl -f -o json";
    return true;
}

void JournaldReader::stop()
{
    if (!process_) {
        return;
    }
    process_->kill();
    process_->waitForFinished(1000);
    delete process_;
    process_ = nullptr;
    buffer_.clear();
}

void JournaldReader::handleReadyRead()
{
    if (!process_) {
        return;
    }

    buffer_.append(process_->readAllStandardOutput());

    int index = 0;
    while (true) {
        int newline = buffer_.indexOf('\n', index);
        if (newline < 0) {
            // No complete line yet.
            break;
        }

        QByteArray line = buffer_.mid(index, newline - index).trimmed();
        if (!line.isEmpty()) {
            processLine(line);
        }

        index = newline + 1;
    }

    // Keep any partial line for next time.
    if (index > 0) {
        buffer_ = buffer_.mid(index);
    }
}

void JournaldReader::handleFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(status);
    qWarning() << "JournaldReader: journalctl exited; stopping reader";
    stop();
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

// Phase 21: classifier with special-cases + gating
bool JournaldReader::classifyMessage(const QString &message,
                                     Category &outCategory,
                                     Severity &outSeverity,
                                     QString &outLabel)
{
    const QString lower = message.toLower();

    // HTTP 429 / rate limiting (from requests / services)
    if (lower.contains("httperror") &&
        (lower.contains("429 client error") ||
         lower.contains("too many requests"))) {
        outCategory = Category::Network;
        outSeverity = Severity::Warning;
        outLabel = QStringLiteral("HTTP 429 (rate limited)");
        return true;
    }

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

    // systemd resource accounting:
    // "Consumed 1.690s CPU time over 8.269s wall clock time, 274.1M memory peak."
    if (lower.contains("consumed") &&
        lower.contains("cpu time over") &&
        lower.contains("memory peak")) {

        static QRegularExpression re(
            R"(Consumed\s+([0-9\.]+)s CPU time over\s+([0-9\.]+)s wall clock time,\s+([0-9\.]+)M memory peak\.)",
            QRegularExpression::CaseInsensitiveOption
        );

        const QRegularExpressionMatch m = re.match(message);
        if (m.hasMatch()) {
            const double cpuSec = m.captured(1).toDouble();
            const double wallSec = m.captured(2).toDouble();
            const double memMB  = m.captured(3).toDouble();

            // Thresholds: only surface if meaningfully heavy
            const double CPU_THRESHOLD  = 5.0;    // seconds
            const double MEM_THRESHOLD  = 1024.0; // ~1 GiB

            if (cpuSec >= CPU_THRESHOLD || memMB >= MEM_THRESHOLD) {
                outCategory = Category::Process;
                outSeverity = Severity::Warning;
                outLabel = QStringLiteral("High resource usage (systemd)");
                return true;
            } else {
                // Below threshold: treat as noise, do NOT classify
                return false;
            }
        }
    }

    return false;
}

void JournaldReader::processLine(const QByteArray &line)
{
    QJsonParseError err{};
    QJsonDocument doc = QJsonDocument::fromJson(line, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    const QJsonObject obj = doc.object();

    // MESSAGE
    const QString message = obj.value(QStringLiteral("MESSAGE")).toString();

    // PRIORITY (string or int)
    int prio = 5;
    const QJsonValue prioVal = obj.value(QStringLiteral("PRIORITY"));
    if (prioVal.isString()) {
        bool ok = false;
        int tmp = prioVal.toString().toInt(&ok);
        if (ok) prio = tmp;
    } else if (prioVal.isDouble()) {
        prio = prioVal.toInt();
    }

    // UNIT / IDENTIFIER
    const QString unit = obj.value(QStringLiteral("_SYSTEMD_UNIT")).toString();
    const QString ident = obj.value(QStringLiteral("SYSLOG_IDENTIFIER")).toString();

    Category cat = Category::System;
    Severity sev = severityFromPriority(prio);
    QString label;

    const bool matched = classifyMessage(message, cat, sev, label);

    if (!matched) {
        // Fallback: generic mapping, but with gating:
        // we DROP generic system/info noise.
        cat = Category::System;
        label = message.left(120);

        if (sev == Severity::Info && cat == Category::System) {
            // too chatty, skip this event entirely
            return;
        }
    }

    Event ev;
    ev.timestamp = nowUtc();  // journalctl already ordered; approximate is fine
    ev.category = cat;
    ev.severity = sev;
    ev.label = label;

    QJsonObject details;
    if (!message.isEmpty())
        details.insert(QStringLiteral("message"), message);
    if (!unit.isEmpty())
        details.insert(QStringLiteral("unit"), unit);
    if (!ident.isEmpty())
        details.insert(QStringLiteral("identifier"), ident);
    details.insert(QStringLiteral("priority"), prio);

    ev.details = details;

    emit eventDetected(ev);
}

} // namespace kpulse
