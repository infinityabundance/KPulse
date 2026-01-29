#include "mainwindow.hpp"

#include "event_model.hpp"
#include "timeline_view.hpp"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QSplitter>
#include <QTableView>
#include <QTextEdit>
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
    resize(1200, 800);

    QWidget *central = new QWidget(this);
    QHBoxLayout *hbox = new QHBoxLayout(central);
    hbox->setContentsMargins(0, 0, 0, 0);

    QSplitter *leftSplitter = new QSplitter(Qt::Vertical, central);

    QWidget *topControlsAndTimeline = new QWidget(leftSplitter);
    QVBoxLayout *topLayout = new QVBoxLayout(topControlsAndTimeline);
    topLayout->setContentsMargins(4, 4, 4, 4);

    QHBoxLayout *controls = new QHBoxLayout();
    rangeCombo_ = new QComboBox(topControlsAndTimeline);
    rangeCombo_->addItem(QStringLiteral("Last 10 minutes"));
    rangeCombo_->addItem(QStringLiteral("Last hour"));
    rangeCombo_->addItem(QStringLiteral("Today"));

    refreshButton_ = new QPushButton(QStringLiteral("Refresh"), topControlsAndTimeline);
    controls->addWidget(rangeCombo_);
    controls->addWidget(refreshButton_);
    controls->addStretch(1);

    topLayout->addLayout(controls);

    timelineView_ = new TimelineView(topControlsAndTimeline);
    topLayout->addWidget(timelineView_);

    leftSplitter->addWidget(topControlsAndTimeline);

    model_ = new EventModel(this);
    tableView_ = new QTableView(leftSplitter);
    tableView_->setModel(model_);
    tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView_->setAlternatingRowColors(true);
    tableView_->horizontalHeader()->setStretchLastSection(true);
    tableView_->setSortingEnabled(true);

    leftSplitter->addWidget(tableView_);
    leftSplitter->setStretchFactor(0, 1);
    leftSplitter->setStretchFactor(1, 2);

    detailsEdit_ = new QTextEdit(central);
    detailsEdit_->setReadOnly(true);

    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, central);
    mainSplitter->addWidget(leftSplitter);
    mainSplitter->addWidget(detailsEdit_);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 2);

    hbox->addWidget(mainSplitter);
    setCentralWidget(central);

    connect(refreshButton_, &QPushButton::clicked, this, &MainWindow::refreshEvents);

    QItemSelectionModel *selModel = tableView_->selectionModel();
    connect(selModel, &QItemSelectionModel::selectionChanged, this, &MainWindow::handleTableSelectionChanged);

    connect(timelineView_, &TimelineView::eventClicked, this, &MainWindow::handleTimelineClicked);
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

    model_->setEvents(events);
    timelineView_->setEvents(events);

    if (events.empty()) {
        detailsEdit_->clear();
    }
}

void MainWindow::handleLiveEvent(const kpulse::Event &event)
{
    Q_UNUSED(event);
    refreshEvents();
}

void MainWindow::handleTableSelectionChanged()
{
    QItemSelectionModel *selModel = tableView_->selectionModel();
    if (!selModel) {
        return;
    }

    const QModelIndexList rows = selModel->selectedRows();
    if (rows.isEmpty()) {
        return;
    }

    const int row = rows.first().row();
    if (row < 0 || row >= model_->rowCount()) {
        return;
    }

    showEventDetails(model_->eventAt(row));
}

void MainWindow::handleTimelineClicked(const kpulse::Event &event)
{
    showEventDetails(event);
}

void MainWindow::showEventDetails(const kpulse::Event &event)
{
    QString text;
    text += QStringLiteral("Timestamp: %1\n").arg(event.timestamp.toString(Qt::ISODate));
    text += QStringLiteral("Category: %1\n").arg(kpulse::categoryToString(event.category));
    text += QStringLiteral("Severity: %1\n").arg(kpulse::severityToString(event.severity));
    text += QStringLiteral("Label: %1\n\n").arg(event.label);

    QJsonDocument doc(event.details);
    text += QStringLiteral("Details:\n");
    text += QString::fromUtf8(doc.toJson(QJsonDocument::Indented));

    detailsEdit_->setPlainText(text);
}
