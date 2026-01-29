#include <QApplication>

#include "tray_app.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kpulse-tray"));

    TrayApp tray;
    return app.exec();
}
