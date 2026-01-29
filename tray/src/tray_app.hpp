#pragma once

#include <QObject>
#include <QDateTime>
#include <QTimer>

#include <KStatusNotifierItem>

#include "kpulse/ipc_client.hpp"
#include "kpulse/event.hpp"

class TrayApp : public QObject
{
    Q_OBJECT
public:
    explicit TrayApp(QObject *parent = nullptr);

private slots:
    void updateStatus();
    void onEventReceived(const kpulse::Event &event);
    void openMainUi();

private:
    void refreshFromDaemon();
    void applyEvents(const std::vector<kpulse::Event> &events);

    kpulse::IpcClient ipc_;
    KStatusNotifierItem *trayIcon_ = nullptr;
    QTimer refreshTimer_;
    QDateTime lastUpdate_;
};
