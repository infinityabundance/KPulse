#pragma once

#include <QAbstractTableModel>
#include <vector>

#include "kpulse/event.hpp"

class EventModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit EventModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void setEvents(std::vector<kpulse::Event> events);
    void appendEvent(const kpulse::Event &event);
    const kpulse::Event &eventAt(int row) const;
    bool exportToJson(const QString &filePath) const;
    bool exportToCsv(const QString &filePath) const;
    const std::vector<kpulse::Event> &allEvents() const;

private:
    std::vector<kpulse::Event> events_;
};
