#include "event_model.hpp"

#include <QColor>
#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>
#include <QVariant>

using kpulse::Event;
using kpulse::Category;
using kpulse::Severity;

EventModel::EventModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int EventModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(events_.size());
}

int EventModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return 4;
}

QVariant EventModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }

    const Event &ev = events_.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0:
            return ev.timestamp.toString(Qt::ISODate);
        case 1:
            return kpulse::categoryToString(ev.category);
        case 2:
            return kpulse::severityToString(ev.severity);
        case 3:
            return ev.label;
        default:
            return {};
        }
    }

    if (role == Qt::BackgroundRole) {
        switch (ev.severity) {
        case Severity::Info:
            return QVariant();
        case Severity::Warning:
            return QColor(255, 245, 200);
        case Severity::Error:
            return QColor(255, 220, 220);
        case Severity::Critical:
            return QColor(255, 200, 200);
        }
    }

    return {};
}

QVariant EventModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }

    switch (section) {
    case 0:
        return QStringLiteral("Timestamp");
    case 1:
        return QStringLiteral("Category");
    case 2:
        return QStringLiteral("Severity");
    case 3:
        return QStringLiteral("Label");
    default:
        return {};
    }
}

void EventModel::setEvents(std::vector<Event> events)
{
    beginResetModel();
    events_ = std::move(events);
    endResetModel();
}

void EventModel::appendEvent(const Event &event)
{
    const int row = static_cast<int>(events_.size());
    beginInsertRows(QModelIndex(), row, row);
    events_.push_back(event);
    endInsertRows();
}

const Event &EventModel::eventAt(int row) const
{
    return events_.at(row);
}

bool EventModel::exportToJson(const QString &filePath) const
{
    QJsonArray arr;
    for (const auto &ev : events_) {
        arr.append(kpulse::eventToJson(ev));
    }

    QJsonDocument doc(arr);
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    f.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

bool EventModel::exportToCsv(const QString &filePath) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QTextStream out(&f);
    out << "timestamp,category,severity,label\n";

    for (const auto &ev : events_) {
        out << ev.timestamp.toString(Qt::ISODate) << ",";
        out << kpulse::categoryToString(ev.category) << ",";
        out << kpulse::severityToString(ev.severity) << ",";
        out << "\"" << QString(ev.label).replace('"', "\"\"") << "\"\n";
    }

    return true;
}

const std::vector<kpulse::Event> &EventModel::allEvents() const
{
    return events_;
}
