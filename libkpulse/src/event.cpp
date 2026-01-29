#include "kpulse/event.hpp"

#include <QJsonDocument>
#include <QJsonValue>

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
    const QString value = s.toLower();
    if (value == QLatin1String("system")) {
        return Category::System;
    }
    if (value == QLatin1String("gpu")) {
        return Category::GPU;
    }
    if (value == QLatin1String("thermal")) {
        return Category::Thermal;
    }
    if (value == QLatin1String("process")) {
        return Category::Process;
    }
    if (value == QLatin1String("update")) {
        return Category::Update;
    }
    if (value == QLatin1String("network")) {
        return Category::Network;
    }
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
    const QString value = s.toLower();
    if (value == QLatin1String("info")) {
        return Severity::Info;
    }
    if (value == QLatin1String("warning")) {
        return Severity::Warning;
    }
    if (value == QLatin1String("error")) {
        return Severity::Error;
    }
    if (value == QLatin1String("critical")) {
        return Severity::Critical;
    }
    return Severity::Info;
}

QJsonObject eventToJson(const Event &ev)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("id"), static_cast<qint64>(ev.id));
    obj.insert(QStringLiteral("timestamp_ms"), static_cast<qint64>(ev.timestamp.toMSecsSinceEpoch()));
    obj.insert(QStringLiteral("category"), categoryToString(ev.category));
    obj.insert(QStringLiteral("severity"), severityToString(ev.severity));
    obj.insert(QStringLiteral("label"), ev.label);
    obj.insert(QStringLiteral("details"), ev.details);
    obj.insert(QStringLiteral("anomalous"), ev.isAnomalous);
    obj.insert(QStringLiteral("anomalyScore"), ev.anomalyScore);
    if (ev.windowId.has_value()) {
        obj.insert(QStringLiteral("window_id"), static_cast<qint64>(ev.windowId.value()));
    }
    return obj;
}

Event eventFromJson(const QJsonObject &obj)
{
    Event ev;
    ev.id = obj.value(QStringLiteral("id")).toVariant().toLongLong();

    const qint64 timestampMs = obj.value(QStringLiteral("timestamp_ms")).toVariant().toLongLong();
    if (timestampMs > 0) {
        ev.timestamp = QDateTime::fromMSecsSinceEpoch(timestampMs, Qt::UTC);
    }

    ev.category = categoryFromString(obj.value(QStringLiteral("category")).toString());
    ev.severity = severityFromString(obj.value(QStringLiteral("severity")).toString());
    ev.label = obj.value(QStringLiteral("label")).toString();

    const QJsonValue detailsValue = obj.value(QStringLiteral("details"));
    if (detailsValue.isObject()) {
        ev.details = detailsValue.toObject();
    }

    const QJsonValue anomalousValue = obj.value(QStringLiteral("anomalous"));
    if (anomalousValue.isBool()) {
        ev.isAnomalous = anomalousValue.toBool();
    }
    const QJsonValue scoreValue = obj.value(QStringLiteral("anomalyScore"));
    if (scoreValue.isDouble()) {
        ev.anomalyScore = scoreValue.toDouble();
    }

    const QJsonValue windowValue = obj.value(QStringLiteral("window_id"));
    if (!windowValue.isUndefined() && !windowValue.isNull()) {
        ev.windowId = windowValue.toVariant().toLongLong();
    }

    return ev;
}

QString eventToJsonString(const Event &ev)
{
    QJsonObject obj = eventToJson(ev);
    QJsonDocument doc(obj);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
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
