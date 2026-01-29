#include "kpulse/ipc_client.hpp"

#include <QDBusConnection>
#include <QDBusReply>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "kpulse/event.hpp"
#include "kpulse_dbus_interface.h"

namespace kpulse {

IpcClient::IpcClient(QObject *parent)
    : QObject(parent)
    , iface_(nullptr)
{
    iface_ = new OrgKdeKpulseDaemonInterface(
        QStringLiteral("org.kde.kpulse"),
        QStringLiteral("/org/kde/kpulse/Daemon"),
        QDBusConnection::sessionBus(),
        this);

    connect(iface_, &OrgKdeKpulseDaemonInterface::EventAdded, this, [this](const QString &eventJson) {
        Event ev = eventFromJsonString(eventJson);
        emit eventReceived(ev);
    });
}

std::vector<Event> IpcClient::getEvents(const QDateTime &from,
                                        const QDateTime &to,
                                        const std::vector<Category> &categories)
{
    if (!iface_) {
        return {};
    }

    const qint64 fromMs = from.toMSecsSinceEpoch();
    const qint64 toMs = to.toMSecsSinceEpoch();

    QStringList catStrings;
    for (Category c : categories) {
        catStrings << categoryToString(c);
    }

    QDBusReply<QString> reply = iface_->GetEvents(fromMs, toMs, catStrings);
    if (!reply.isValid()) {
        return {};
    }

    const QString json = reply.value();
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isArray()) {
        return {};
    }

    const QJsonArray arr = doc.array();
    std::vector<Event> events;
    events.reserve(arr.size());

    for (const QJsonValue &val : arr) {
        if (!val.isObject()) {
            continue;
        }
        Event ev = eventFromJson(val.toObject());
        events.push_back(std::move(ev));
    }

    return events;
}

} // namespace kpulse
