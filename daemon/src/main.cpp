#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "kpulse_daemon.hpp"

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

    qInfo() << "KPulse daemon initialized";
    return app.exec();
}
