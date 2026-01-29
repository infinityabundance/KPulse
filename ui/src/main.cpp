#include <QApplication>

#include <KAboutData>
#include <KLocalizedString>

#include "mainwindow.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("KPulse"));

    KAboutData about(
        QStringLiteral("kpulse"),
        i18n("KPulse"),
        QStringLiteral("0.1"),
        i18n("System heartbeat and event timeline for KDE Plasma"),
        KAboutLicense::GPL_V3,
        i18n("© 2026 Riaan de Beer"));
    KAboutData::setApplicationData(about);

    MainWindow window;
    window.show();

    return app.exec();
}
