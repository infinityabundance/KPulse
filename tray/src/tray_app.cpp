#include "tray_app.hpp"

#include <QAction>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QIcon>
#include <QMenu>
#include <QProcess>

using kpulse::Event;
using kpulse::Category;
using kpulse::Severity;

namespace {

int severityRank(Severity s)
{
    switch (s) {
    case Severity::Info:
        return 0;
    case Severity::Warning:
        return 1;
    case Severity::Error:
        return 2;
    case Severity::Critical:
        return 3;
    }
    return 0;
}

} // namespace

TrayApp::TrayApp(QObject *parent)
    : QObject(parent)
{
    trayIcon_ = new KStatusNotifierItem(QStringLiteral("KPulseTray"), this);
    trayIcon_->setTitle(QStringLiteral("KPulse"));
    trayIcon_->setToolTipTitle(QStringLiteral("KPulse"));
    trayIcon_->setCategory(KStatusNotifierItem::ApplicationStatus);
    trayIcon_->setIconByName(QStringLiteral("dialog-information"));

    QMenu *menu = new QMenu();
    QAction *openAction = menu->addAction(QStringLiteral("Open KPulse"));
    QAction *quitAction = menu->addAction(QStringLiteral("Quit"));

    connect(openAction, &QAction::triggered, this, &TrayApp::openMainUi);
    connect(quitAction, &QAction::triggered, QCoreApplication::instance(), &QCoreApplication::quit);

    trayIcon_->setContextMenu(menu);

    connect(&ipc_, &kpulse::IpcClient::eventReceived, this, &TrayApp::onEventReceived);

    refreshTimer_.setInterval(30000);
    refreshTimer_.setSingleShot(false);
    connect(&refreshTimer_, &QTimer::timeout, this, &TrayApp::updateStatus);
    refreshTimer_.start();

    updateStatus();
}

void TrayApp::updateStatus()
{
    refreshFromDaemon();
}

void TrayApp::refreshFromDaemon()
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QDateTime from = now.addSecs(-5 * 60);

    std::vector<Category> categories;
    auto events = ipc_.getEvents(from, now, categories);
    applyEvents(events);

    lastUpdate_ = now;
}

void TrayApp::applyEvents(const std::vector<Event> &events)
{
    if (events.empty()) {
        trayIcon_->setIconByName(QStringLiteral("dialog-information"));
        trayIcon_->setToolTipSubTitle(QStringLiteral("System healthy (no recent events)"));
        return;
    }

    int maxRank = 0;
    Severity worst = Severity::Info;
    QString worstLabel;

    for (const auto &ev : events) {
        const int rank = severityRank(ev.severity);
        if (rank >= maxRank) {
            maxRank = rank;
            worst = ev.severity;
            worstLabel = ev.label;
        }
    }

    QString iconName;
    QString subtitle;

    switch (worst) {
    case Severity::Info:
        iconName = QStringLiteral("dialog-information");
        subtitle = QStringLiteral("System healthy");
        break;
    case Severity::Warning:
        iconName = QStringLiteral("dialog-warning");
        subtitle = QStringLiteral("Warnings in last 5 minutes");
        break;
    case Severity::Error:
        iconName = QStringLiteral("dialog-error");
        subtitle = QStringLiteral("Errors in last 5 minutes");
        break;
    case Severity::Critical:
        iconName = QStringLiteral("dialog-error");
        subtitle = QStringLiteral("Critical issues in last 5 minutes");
        break;
    }

    if (!worstLabel.isEmpty()) {
        subtitle += QStringLiteral(" — latest: %1").arg(worstLabel);
    }

    trayIcon_->setIconByName(iconName);
    trayIcon_->setToolTipSubTitle(subtitle);
}

void TrayApp::onEventReceived(const Event &event)
{
    Q_UNUSED(event);
    updateStatus();
}

void TrayApp::openMainUi()
{
    if (!QProcess::startDetached(QStringLiteral("kpulse-ui"))) {
        qWarning() << "KPulse tray: failed to launch kpulse-ui";
    }
}
