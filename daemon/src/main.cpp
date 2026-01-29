#include <QCoreApplication>
#include <QCommandLineParser>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

#include "kpulse_daemon.hpp"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kpulse-daemon"));
    app.setOrganizationName(QStringLiteral("KPulse"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("KPulse system events daemon"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption dbOpt(
        QStringList() << QStringLiteral("d") << QStringLiteral("database"),
        QStringLiteral("Path to the KPulse events database (SQLite file)."),
        QStringLiteral("path")
    );
    parser.addOption(dbOpt);

    parser.process(app);

    QString dbPath = parser.value(dbOpt);
    if (dbPath.isEmpty()) {
        QString dataDir =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (dataDir.isEmpty()) {
            dataDir = QDir::homePath() + QStringLiteral("/.local/share/kpulse");
        }
        QDir().mkpath(dataDir);
        dbPath = dataDir + QStringLiteral("/events.sqlite");
    }

    kpulse::KPulseDaemon daemon(dbPath);
    if (!daemon.init()) {
        qCritical() << "KPulse daemon: failed to initialise, exiting";
        return 1;
    }

    qInfo() << "KPulse daemon running, DB at" << dbPath;
    return app.exec();
}
