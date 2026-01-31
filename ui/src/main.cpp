#include <QApplication>
#include <QIcon>
#include "mainwindow.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("KPulse"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/kpulse.svg")));

    MainWindow window;
    window.show();

    return app.exec();
}
