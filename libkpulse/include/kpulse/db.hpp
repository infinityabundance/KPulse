#pragma once

#include "event.hpp"

#include <QSqlDatabase>
#include <vector>

namespace kpulse {

class EventStore {
public:
    explicit EventStore(const QString &dbPath);

    bool open();
    bool initSchema();
    bool insertEvent(const Event &event, qint64 *outId = nullptr);

    std::vector<Event> queryEvents(const QDateTime &from,
                                   const QDateTime &to,
                                   const std::vector<Category> &categories = {});

private:
    QString dbPath_;
    QSqlDatabase db_;

    bool ensureConnection();
};

} // namespace kpulse
