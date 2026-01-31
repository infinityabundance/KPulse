#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QElapsedTimer>
#include <QProcess>
#include <QThread>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

namespace {

constexpr const char *kDaemonService = "org.kde.kpulse.Daemon";
constexpr const char *kTrayService   = "org.kde.kpulse.Tray";

bool isServiceRegistered(const char *service)
{
    QDBusConnectionInterface *iface = QDBusConnection::sessionBus().interface();
    if (!iface) {
        return false;
    }
    return iface->isServiceRegistered(QString::fromUtf8(service));
}

void waitForService(const char *service, int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        if (isServiceRegistered(service)) {
            return;
        }
        QThread::msleep(50);
    }
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const QString appDir = QCoreApplication::applicationDirPath();
    const QString daemonPath = QDir(appDir).filePath("../daemon/kpulse-daemon");
    const QString trayPath = QDir(appDir).filePath("../tray/kpulse-tray");
    const QString uiPath = QDir(appDir).filePath("../ui/kpulse-ui");

    if (!isServiceRegistered(kDaemonService)) {
        const QString daemonCmd = QFileInfo::exists(daemonPath)
            ? QDir::cleanPath(daemonPath)
            : QStringLiteral("kpulse-daemon");
        if (!QProcess::startDetached(daemonCmd)) {
            qWarning() << "KPulse launcher: failed to start kpulse-daemon";
        }
        waitForService(kDaemonService, 2000);
    }

    if (!isServiceRegistered(kTrayService)) {
        const QString trayCmd = QFileInfo::exists(trayPath)
            ? QDir::cleanPath(trayPath)
            : QStringLiteral("kpulse-tray");
        if (!QProcess::startDetached(trayCmd)) {
            qWarning() << "KPulse launcher: failed to start kpulse-tray";
        }
    }

    const QString uiCmd = QFileInfo::exists(uiPath)
        ? QDir::cleanPath(uiPath)
        : QStringLiteral("kpulse-ui");
    if (!QProcess::startDetached(uiCmd)) {
        qWarning() << "KPulse launcher: failed to start kpulse-ui";
        return 1;
    }

    return 0;
}
