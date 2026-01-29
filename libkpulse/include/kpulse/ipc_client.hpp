#pragma once

#include <QObject>
#include <QDateTime>
#include <vector>

#include "event.hpp"

class OrgKdeKpulseDaemonInterface;

namespace kpulse {

class IpcClient : public QObject
{
    Q_OBJECT
public:
    explicit IpcClient(QObject *parent = nullptr);

    std::vector<Event> getEvents(const QDateTime &from,
                                 const QDateTime &to,
                                 const std::vector<Category> &categories);

signals:
    void eventReceived(const kpulse::Event &event);

private:
    OrgKdeKpulseDaemonInterface *iface_;
};

} // namespace kpulse
