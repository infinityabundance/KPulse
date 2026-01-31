#include <QApplication>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QProcess>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QUrl>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>

namespace {

constexpr const char *kTrayService = "org.kde.kpulse.Tray";
constexpr const char *kDaemonService = "org.kde.kpulse.Daemon";

bool isDaemonRunning()
{
    QDBusConnectionInterface *iface = QDBusConnection::sessionBus().interface();
    if (!iface) {
        return false;
    }
    return iface->isServiceRegistered(QString::fromUtf8(kDaemonService));
}

QString resolveLocalBinary(const QString &relativePath, const QString &fallback)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString candidate = QDir(appDir).filePath(relativePath);
    return QFileInfo::exists(candidate) ? QDir::cleanPath(candidate) : fallback;
}

void stopDaemonWithFallback()
{
    QProcess proc;
    proc.start(QStringLiteral("systemctl"),
               {QStringLiteral("--user"), QStringLiteral("stop"), QStringLiteral("kpulse-daemon.service")});
    if (proc.waitForStarted(500)) {
        proc.waitForFinished(1500);
    }

    if (isDaemonRunning()) {
        QProcess::startDetached(QStringLiteral("pkill"), {QStringLiteral("-x"), QStringLiteral("kpulse-daemon")});
    }
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    // Prevent multiple tray instances.
    if (!QDBusConnection::sessionBus().registerService(QString::fromUtf8(kTrayService))) {
        return 0;
    }

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return 0;
    }

    auto *tray = new QSystemTrayIcon();
    tray->setIcon(QIcon(QStringLiteral(":/icons/kpulse.svg")));
    tray->setToolTip("KPulse");

    auto *menu = new QMenu();

    QAction *statusAction = menu->addAction("Checking daemon...");
    statusAction->setEnabled(false);

    QAction *toggleDaemonAction = menu->addAction("Start daemon");

    menu->addSeparator();

    QAction *openAction = menu->addAction("Open KPulse");
    QAction *aboutAction = menu->addAction("About KPulse");
    QAction *quitAction = menu->addAction("Quit");

    tray->setContextMenu(menu);
    tray->show();

    // Periodically update daemon status
    QTimer *timer = new QTimer(tray);
    auto updateStatus = [&]() {
        const bool running = isDaemonRunning();
        statusAction->setText(running ? "Daemon: running" : "Daemon: stopped");
        toggleDaemonAction->setText(running ? "Stop daemon" : "Start daemon");
    };
    QObject::connect(timer, &QTimer::timeout, updateStatus);
    timer->start(2000);
    updateStatus();

    QObject::connect(openAction, &QAction::triggered, []() {
        const QString uiCmd = resolveLocalBinary(
            QStringLiteral("../ui/kpulse-ui"),
            QStringLiteral("kpulse-ui")
        );
        QProcess::startDetached(uiCmd);
    });

    QObject::connect(toggleDaemonAction, &QAction::triggered, []() {
        if (isDaemonRunning()) {
            stopDaemonWithFallback();
            return;
        }
        const QString daemonCmd = resolveLocalBinary(
            QStringLiteral("../daemon/kpulse-daemon"),
            QStringLiteral("kpulse-daemon")
        );
        QProcess::startDetached(daemonCmd);
    });

    QObject::connect(aboutAction, &QAction::triggered, []() {
        QDesktopServices::openUrl(
            QUrl(QStringLiteral("https://github.com/infinityabundance/KPulse"))
        );
    });

    QObject::connect(quitAction, &QAction::triggered, [&]() {
        tray->hide();
        qApp->quit();
    });

    return app.exec();
}
