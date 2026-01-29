#include <QApplication>
#include <QMenu>
#include <QProcess>
#include <QSystemTrayIcon>
#include <QTimer>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>

static bool isDaemonRunning()
{
    QDBusInterface iface(
        "org.kde.kpulse.Daemon",
        "/org/kde/kpulse/Daemon",
        "org.kde.kpulse.Daemon",
        QDBusConnection::sessionBus()
    );
    return iface.isValid();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return 0;
    }

    auto *tray = new QSystemTrayIcon();
    tray->setIcon(QIcon::fromTheme("view-pulse", QIcon(":/icons/kpulse.png")));
    tray->setToolTip("KPulse");

    auto *menu = new QMenu();

    QAction *statusAction = menu->addAction("Checking daemon...");
    statusAction->setEnabled(false);

    menu->addSeparator();

    QAction *openAction = menu->addAction("Open KPulse");
    QAction *quitAction = menu->addAction("Quit");

    tray->setContextMenu(menu);
    tray->show();

    // Periodically update daemon status
    QTimer *timer = new QTimer(tray);
    QObject::connect(timer, &QTimer::timeout, [&]() {
        if (isDaemonRunning()) {
            statusAction->setText("Daemon: running");
        } else {
            statusAction->setText("Daemon: not running");
        }
    });
    timer->start(2000);

    QObject::connect(openAction, &QAction::triggered, []() {
        QProcess::startDetached("kpulse-ui");
    });

    QObject::connect(quitAction, &QAction::triggered, [&]() {
        tray->hide();
        qApp->quit();
    });

    return app.exec();
}
