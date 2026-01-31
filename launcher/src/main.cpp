#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QElapsedTimer>
#include <QProcess>
#include <QThread>
#include <QDebug>

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

    if (!isServiceRegistered(kDaemonService)) {
        if (!QProcess::startDetached(QStringLiteral("kpulse-daemon"))) {
            qWarning() << "KPulse launcher: failed to start kpulse-daemon";
        }
        waitForService(kDaemonService, 2000);
    }

    if (!isServiceRegistered(kTrayService)) {
        if (!QProcess::startDetached(QStringLiteral("kpulse-tray"))) {
            qWarning() << "KPulse launcher: failed to start kpulse-tray";
        }
    }

    if (!QProcess::startDetached(QStringLiteral("kpulse-ui"))) {
        qWarning() << "KPulse launcher: failed to start kpulse-ui";
        return 1;
    }

    return 0;
}
