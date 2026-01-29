#include "event_model.hpp"

#include <QDateTime>

using kpulse::Event;

EventModel::EventModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int EventModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return events_.size();
}

int EventModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    // Timestamp, Category, Severity, Label
    return 4;
}

QVariant EventModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= events_.size())
        return {};

    const Event &ev = events_.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0:
            return ev.timestamp.toString(Qt::ISODateWithMs);
        case 1:
            return kpulse::categoryToString(ev.category);
        case 2:
            return kpulse::severityToString(ev.severity);
        case 3:
            return ev.label;
        default:
            break;
        }
    }

    return {};
}

QVariant EventModel::headerData(int section, Qt::Orientation orientation,
                                int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section) {
    case 0: return QStringLiteral("Timestamp");
    case 1: return QStringLiteral("Category");
    case 2: return QStringLiteral("Severity");
    case 3: return QStringLiteral("Label");
    default: break;
    }
    return {};
}

void EventModel::setEvents(const std::vector<Event> &events)
{
    beginResetModel();
    events_.clear();
    events_.reserve(static_cast<int>(events.size()));
    for (const auto &ev : events) {
        events_.push_back(ev);
    }
    endResetModel();
}

void EventModel::appendEvent(const Event &ev)
{
    const int row = events_.size();
    beginInsertRows(QModelIndex(), row, row);
    events_.push_back(ev);
    endInsertRows();
}

kpulse::Event EventModel::eventAt(int row) const
{
    if (row < 0 || row >= events_.size())
        return kpulse::Event{};
    return events_.at(row);
}
