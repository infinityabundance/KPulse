#include "event_model.hpp"

#include <QDateTime>
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

    if (role != Qt::DisplayRole) {
        return {};
    }

    const Event &ev = events_.at(index.row());

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

const Event &EventModel::eventAt(int row) const
{
    return events_.at(row);
}
