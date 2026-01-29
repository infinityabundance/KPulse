#pragma once

#include <QObject>
#include <QDateTime>
#include <QString>
#include <QStringList>

#include <vector>

#include "kpulse/event.hpp"

class QDBusInterface;

namespace kpulse {

class IpcClient : public QObject
{
    Q_OBJECT
public:
    explicit IpcClient(QObject *parent = nullptr);
    ~IpcClient() override;

    // Try to connect to the KPulse daemon on the session bus.
    // Returns true if the DBus interface looks valid.
    bool connectToDaemon();

    bool isConnected() const { return connected_; }
    QString lastError() const { return lastError_; }

    // Synchronous fetch of events via DBus.
    // Returns an empty vector on error; lastError() will be set.
    std::vector<Event> getEvents(const QDateTime &from,
                                 const QDateTime &to,
                                 const std::vector<Category> &categories);

signals:
    // Emitted whenever the daemon pushes a new event over DBus.
    void eventReceived(const kpulse::Event &event);

    // Emitted when connection state changes.
    void connectionChanged(bool connected);

private slots:
    void handleEventJson(const QString &json);

private:
    void setConnected(bool c);

    QDBusInterface *iface_ = nullptr;
    bool connected_ = false;
    QString lastError_;
};

} // namespace kpulse
