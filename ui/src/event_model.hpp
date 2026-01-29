#pragma once

// Event model with clipboard/CSV helpers and append support for live updates.

#include <QAbstractTableModel>
#include <QVector>

#include "kpulse/event.hpp"

class EventModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit EventModel(QObject *parent = nullptr);

    // QAbstractTableModel overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // Replace the data set with new events.
    void setEvents(const std::vector<kpulse::Event> &events);

    // Append a single event (for live updates).
    void appendEvent(const kpulse::Event &ev);

    // Accessors used by MainWindow for clipboard/CSV export.
    kpulse::Event eventAt(int row) const;
    const QVector<kpulse::Event> &events() const { return events_; }

private:
    QVector<kpulse::Event> events_;
};
