#include "kpulse/event.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QTimeZone>

namespace kpulse {

QString categoryToString(Category c)
{
    switch (c) {
    case Category::System:
        return QStringLiteral("system");
    case Category::GPU:
        return QStringLiteral("gpu");
    case Category::Thermal:
        return QStringLiteral("thermal");
    case Category::Process:
        return QStringLiteral("process");
    case Category::Update:
        return QStringLiteral("update");
    case Category::Network:
        return QStringLiteral("network");
    }

    return QStringLiteral("system");
}

Category categoryFromString(const QString &s)
{
    const QString lower = s.trimmed().toLower();

    if (lower == QLatin1String("system"))
        return Category::System;
    if (lower == QLatin1String("gpu"))
        return Category::GPU;
    if (lower == QLatin1String("thermal"))
        return Category::Thermal;
    if (lower == QLatin1String("process"))
        return Category::Process;
    if (lower == QLatin1String("update"))
        return Category::Update;
    if (lower == QLatin1String("network"))
        return Category::Network;

    return Category::System;
}

QString severityToString(Severity s)
{
    switch (s) {
    case Severity::Info:
        return QStringLiteral("info");
    case Severity::Warning:
        return QStringLiteral("warning");
    case Severity::Error:
        return QStringLiteral("error");
    case Severity::Critical:
        return QStringLiteral("critical");
    }

    return QStringLiteral("info");
}

Severity severityFromString(const QString &s)
{
    const QString lower = s.trimmed().toLower();

    if (lower == QLatin1String("info"))
        return Severity::Info;
    if (lower == QLatin1String("warning"))
        return Severity::Warning;
    if (lower == QLatin1String("error"))
        return Severity::Error;
    if (lower == QLatin1String("critical"))
        return Severity::Critical;

    return Severity::Info;
}

QJsonObject eventToJson(const Event &ev)
{
    QJsonObject obj;

    // Basic fields
    if (ev.id != 0) {
        obj.insert(QStringLiteral("id"), static_cast<qint64>(ev.id));
    }

    // Timestamp as both ISO and ms since epoch (UTC)
    if (ev.timestamp.isValid()) {
        obj.insert(QStringLiteral("timestamp"),
                   ev.timestamp.toUTC().toString(Qt::ISODate));
        obj.insert(QStringLiteral("timestamp_ms"),
                   static_cast<qint64>(ev.timestamp.toMSecsSinceEpoch()));
    }

    obj.insert(QStringLiteral("category"), categoryToString(ev.category));
    obj.insert(QStringLiteral("severity"), severityToString(ev.severity));
    obj.insert(QStringLiteral("label"), ev.label);

    if (!ev.details.isEmpty()) {
        obj.insert(QStringLiteral("details"), ev.details);
    }

    if (ev.windowId.has_value()) {
        obj.insert(QStringLiteral("window_id"),
                   static_cast<qint64>(*ev.windowId));
    }

    return obj;
}

Event eventFromJson(const QJsonObject &obj)
{
    Event ev;

    // ID (optional)
    if (obj.contains(QStringLiteral("id"))) {
        ev.id = obj.value(QStringLiteral("id")).toInteger();
    }

    // Timestamp: prefer timestamp_ms, fall back to ISO string
    if (obj.contains(QStringLiteral("timestamp_ms"))) {
        const qint64 ms =
            obj.value(QStringLiteral("timestamp_ms")).toInteger();
        ev.timestamp = QDateTime::fromMSecsSinceEpoch(
            ms,
            QTimeZone::utc()
        );
    } else if (obj.contains(QStringLiteral("timestamp"))) {
        const QString tsStr =
            obj.value(QStringLiteral("timestamp")).toString();
        QDateTime dt = QDateTime::fromString(tsStr, Qt::ISODate);
        if (!dt.isValid()) {
            dt = QDateTime::fromString(tsStr, Qt::ISODateWithMs);
        }
        if (dt.isValid()) {
            dt.setTimeZone(QTimeZone::utc());
            ev.timestamp = dt;
        }
    }

    // Category
    if (obj.contains(QStringLiteral("category"))) {
        ev.category = categoryFromString(
            obj.value(QStringLiteral("category")).toString()
        );
    }

    // Severity
    if (obj.contains(QStringLiteral("severity"))) {
        ev.severity = severityFromString(
            obj.value(QStringLiteral("severity")).toString()
        );
    }

    // Label
    if (obj.contains(QStringLiteral("label"))) {
        ev.label = obj.value(QStringLiteral("label")).toString();
    }

    // Details (embedded JSON object)
    if (obj.contains(QStringLiteral("details")) &&
        obj.value(QStringLiteral("details")).isObject()) {
        ev.details = obj.value(QStringLiteral("details")).toObject();
    }

    // Optional window id
    if (obj.contains(QStringLiteral("window_id"))) {
        const QJsonValue v = obj.value(QStringLiteral("window_id"));
        if (v.isDouble()) {
            ev.windowId = static_cast<qint64>(v.toInteger());
        }
    }

    return ev;
}

QString eventToJsonString(const Event &ev)
{
    QJsonObject obj = eventToJson(ev);
    QJsonDocument doc(obj);
    return QString::fromUtf8(
        doc.toJson(QJsonDocument::Compact)
    );
}

Event eventFromJsonString(const QString &json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) {
        return Event{};
    }
    return eventFromJson(doc.object());
}

} // namespace kpulse
