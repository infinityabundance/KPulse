#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <optional>

namespace kpulse {

enum class Category {
    System,
    GPU,
    Thermal,
    Process,
    Update,
    Network
};

enum class Severity {
    Info,
    Warning,
    Error,
    Critical
};

struct Event
{
    qint64 id = 0;

    QDateTime timestamp;
    Category  category = Category::System;
    Severity  severity = Severity::Info;
    QString   label;
    QJsonObject details;

    std::optional<qint64> windowId;
};

// String conversions
QString categoryToString(Category c);
Category categoryFromString(const QString &s);

QString severityToString(Severity s);
Severity severityFromString(const QString &s);

// JSON helpers
QJsonObject eventToJson(const Event &ev);
Event eventFromJson(const QJsonObject &obj);

QString eventToJsonString(const Event &ev);
Event eventFromJsonString(const QString &json);

} // namespace kpulse
