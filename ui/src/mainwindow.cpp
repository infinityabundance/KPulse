#include "mainwindow.hpp"

#include "event_model.hpp"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();

    connect(&ipc_, &kpulse::IpcClient::eventReceived, this, &MainWindow::handleLiveEvent);

    refreshEvents();
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("KPulse"));
    resize(1000, 700);

    QWidget *central = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(central);
    vbox->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *controls = new QHBoxLayout();
    rangeCombo_ = new QComboBox(central);
    rangeCombo_->addItem(QStringLiteral("Last 10 minutes"));
    rangeCombo_->addItem(QStringLiteral("Last hour"));
    rangeCombo_->addItem(QStringLiteral("Today"));

    refreshButton_ = new QPushButton(QStringLiteral("Refresh"), central);
    controls->addWidget(rangeCombo_);
    controls->addWidget(refreshButton_);
    controls->addStretch(1);

    vbox->addLayout(controls);

    model_ = new EventModel(this);
    tableView_ = new QTableView(central);
    tableView_->setModel(model_);
    tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView_->setAlternatingRowColors(true);
    tableView_->setSortingEnabled(true);

    vbox->addWidget(tableView_);

    setCentralWidget(central);

    connect(refreshButton_, &QPushButton::clicked, this, &MainWindow::refreshEvents);
}

void MainWindow::updateTimeRange(QDateTime &from, QDateTime &to) const
{
    to = QDateTime::currentDateTimeUtc();

    const QString sel = rangeCombo_->currentText();
    if (sel == QStringLiteral("Last 10 minutes")) {
        from = to.addSecs(-10 * 60);
    } else if (sel == QStringLiteral("Last hour")) {
        from = to.addSecs(-60 * 60);
    } else if (sel == QStringLiteral("Today")) {
        const QDate date = to.date();
        from = QDateTime(QDate(date.year(), date.month(), date.day()), QTime(0, 0, 0), Qt::UTC);
    } else {
        from = to.addSecs(-60 * 60);
    }
}

void MainWindow::refreshEvents()
{
    QDateTime fromUtc;
    QDateTime toUtc;
    updateTimeRange(fromUtc, toUtc);

    std::vector<kpulse::Category> categories;

    auto events = ipc_.getEvents(fromUtc, toUtc, categories);
    model_->setEvents(std::move(events));
}

void MainWindow::handleLiveEvent(const kpulse::Event &event)
{
    Q_UNUSED(event);
    refreshEvents();
}
