#include "kpulse/ipc_client.hpp"

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>
#include <QDebug>

namespace {

// Must match the service/object/interface used in KPulseDaemon
constexpr const char *kServiceName  = "org.kde.kpulse.Daemon";
constexpr const char *kObjectPath   = "/org/kde/kpulse/Daemon";
constexpr const char *kInterface    = "org.kde.kpulse.Daemon";
constexpr const char *kSignalName   = "EventAdded";
constexpr const char *kMethodGet    = "GetEvents";

} // namespace

namespace kpulse {

IpcClient::IpcClient(QObject *parent)
    : QObject(parent)
{
}

IpcClient::~IpcClient()
{
    if (iface_) {
        delete iface_;
        iface_ = nullptr;
    }
}

void IpcClient::setConnected(bool c)
{
    if (connected_ == c)
        return;
    connected_ = c;
    emit connectionChanged(connected_);
}

bool IpcClient::connectToDaemon()
{
    if (iface_) {
        delete iface_;
        iface_ = nullptr;
    }

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        lastError_ = QStringLiteral("DBus session bus not connected");
        setConnected(false);
        return false;
    }

    iface_ = new QDBusInterface(
        QString::fromUtf8(kServiceName),
        QString::fromUtf8(kObjectPath),
        QString::fromUtf8(kInterface),
        bus,
        this
    );

    if (!iface_->isValid()) {
        lastError_ = QStringLiteral("Failed to create DBus interface: %1")
                         .arg(iface_->lastError().message());
        qWarning() << "IpcClient:" << lastError_;
        delete iface_;
        iface_ = nullptr;
        setConnected(false);
        return false;
    }

    // Connect to EventAdded signal for push events.
    bool ok = bus.connect(
        QString::fromUtf8(kServiceName),
        QString::fromUtf8(kObjectPath),
        QString::fromUtf8(kInterface),
        QString::fromUtf8(kSignalName),
        this,
        SLOT(handleEventJson(QString))
    );

    if (!ok) {
        lastError_ = QStringLiteral("Failed to connect to EventAdded signal");
        qWarning() << "IpcClient:" << lastError_;
        // Still usable for pull-based GetEvents, so we do not tear down iface_
    }

    lastError_.clear();
    setConnected(true);
    return true;
}

std::vector<Event> IpcClient::getEvents(const QDateTime &from,
                                        const QDateTime &to,
                                        const std::vector<Category> &categories)
{
    std::vector<Event> results;

    if (!iface_) {
        if (!connectToDaemon()) {
            return results;
        }
    }

    const qint64 fromMs = from.toMSecsSinceEpoch();
    const qint64 toMs   = to.toMSecsSinceEpoch();

    QStringList catNames;
    catNames.reserve(static_cast<int>(categories.size()));
    for (Category c : categories) {
        catNames.push_back(categoryToString(c));
    }

    QDBusReply<QString> reply = iface_->call(
        QString::fromUtf8(kMethodGet),
        static_cast<qlonglong>(fromMs),
        static_cast<qlonglong>(toMs),
        catNames
    );

    if (!reply.isValid()) {
        lastError_ = reply.error().message();
        qWarning() << "IpcClient: GetEvents failed:" << lastError_;
        setConnected(false);
        return results;
    }

    const QString json = reply.value();
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isArray()) {
        lastError_ = QStringLiteral("GetEvents returned non-array JSON");
        qWarning() << "IpcClient:" << lastError_;
        return results;
    }

    const QJsonArray arr = doc.array();
    results.reserve(arr.size());
    for (const QJsonValue &v : arr) {
        if (!v.isObject())
            continue;
        const QJsonObject obj = v.toObject();
        results.push_back(eventFromJson(obj));
    }

    lastError_.clear();
    return results;
}

void IpcClient::handleEventJson(const QString &json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject())
        return;

    Event ev = eventFromJson(doc.object());
    emit eventReceived(ev);
}

} // namespace kpulse
