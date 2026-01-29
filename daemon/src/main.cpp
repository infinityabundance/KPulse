#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "kpulse_daemon.hpp"
#include "kpulse_daemon_adaptor.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kpulse-daemon"));

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    const QString dbPath = dataDir + QStringLiteral("/events.db");

    qInfo() << "KPulse daemon starting, DB path:" << dbPath;

    kpulse::KPulseDaemon daemon(dbPath);
    if (!daemon.init()) {
        qCritical() << "Failed to initialize KPulseDaemon";
        return 1;
    }

    new KpulseDaemonAdaptor(&daemon);

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerService(QStringLiteral("org.kde.kpulse"))) {
        qCritical() << "Failed to register D-Bus service org.kde.kpulse";
        return 1;
    }

    if (!bus.registerObject(QStringLiteral("/org/kde/kpulse/Daemon"), &daemon,
                            QDBusConnection::ExportAdaptors)) {
        qCritical() << "Failed to register D-Bus object /org/kde/kpulse/Daemon";
        return 1;
    }

    qInfo() << "KPulse daemon initialized and D-Bus service registered";
    return app.exec();
}
