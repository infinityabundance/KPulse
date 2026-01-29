#include "mainwindow.hpp"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTableView>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>

#include "kpulse/event.hpp"
#include "kpulse/ipc_client.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , model_(new EventModel(this))
    , ipcClient_(new kpulse::IpcClient(this))
{
    auto *central = new QWidget(this);
    auto *vbox = new QVBoxLayout(central);
    vbox->setContentsMargins(4, 4, 4, 4);
    vbox->setSpacing(4);

    // Top bar: time range + Refresh + Export CSV
    auto *hbox = new QHBoxLayout();
    hbox->setSpacing(4);

    rangeCombo_ = new QComboBox(central);
    rangeCombo_->addItem(QStringLiteral("Last 10 minutes"));
    rangeCombo_->addItem(QStringLiteral("Last hour"));
    rangeCombo_->addItem(QStringLiteral("Last 24 hours"));
    rangeCombo_->addItem(QStringLiteral("Today"));
    hbox->addWidget(rangeCombo_);

    refreshButton_ = new QPushButton(QStringLiteral("Refresh"), central);
    hbox->addWidget(refreshButton_);

    exportButton_ = new QPushButton(QStringLiteral("Export CSV…"), central);
    hbox->addWidget(exportButton_);

    hbox->addStretch(1);
    vbox->addLayout(hbox);

    // Timeline view
    timelineView_ = new TimelineView(central);
    vbox->addWidget(timelineView_, 0);

    // Table view
    tableView_ = new QTableView(central);
    tableView_->setModel(model_);
    tableView_->horizontalHeader()->setStretchLastSection(true);
    tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView_->setContextMenuPolicy(Qt::CustomContextMenu);
    vbox->addWidget(tableView_, 1);

    setCentralWidget(central);
    setWindowTitle(QStringLiteral("KPulse"));

    // Signals/slots
    connect(refreshButton_, &QPushButton::clicked,
            this, &MainWindow::onRefreshClicked);
    connect(rangeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onTimeRangeChanged);

    connect(tableView_, &QTableView::customContextMenuRequested,
            this, &MainWindow::onTableContextMenuRequested);

    connect(exportButton_, &QPushButton::clicked,
            this, &MainWindow::exportCsv);

    // Timeline hover → table highlight
    connect(timelineView_, &TimelineView::eventHovered,
            this, &MainWindow::onTimelineEventHovered);

    // Initial connect to daemon
    ipcClient_->connectToDaemon();

    // Live updates: when daemon broadcasts EventAdded → append to model/timeline
    connect(ipcClient_, &kpulse::IpcClient::eventReceived,
            this, &MainWindow::onEventReceived);

    // Initial load
    onRefreshClicked();
}

void MainWindow::updateTimeRange(QDateTime &from, QDateTime &to) const
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    to = now;

    switch (rangeCombo_->currentIndex()) {
    case 0: // Last 10 minutes
        from = now.addSecs(-10 * 60);
        break;
    case 1: // Last hour
        from = now.addSecs(-60 * 60);
        break;
    case 2: // Last 24 hours
        from = now.addSecs(-24 * 60 * 60);
        break;
    case 3: // Today (UTC)
    default: {
        QDate d = now.date();
        from = QDateTime(QDate(d.year(), d.month(), d.day()),
                         QTime(0, 0, 0),
                         Qt::UTC);
        break;
    }
    }
}

void MainWindow::loadEvents()
{
    if (!ipcClient_->isConnected()) {
        ipcClient_->connectToDaemon();
    }

    QDateTime from, to;
    updateTimeRange(from, to);

    std::vector<kpulse::Category> cats; // empty = all categories
    const auto events = ipcClient_->getEvents(from, to, cats);
    model_->setEvents(events);

    // Feed timeline with the same events
    timelineView_->setEvents(model_->events());
}

void MainWindow::onRefreshClicked()
{
    loadEvents();
}

void MainWindow::onTimeRangeChanged(int)
{
    loadEvents();
}

// ---------- Live updates ----------

void MainWindow::onEventReceived(const kpulse::Event &ev)
{
    // Only show events that fall inside the current time range.
    QDateTime from, to;
    updateTimeRange(from, to);

    if (ev.timestamp < from || ev.timestamp > to)
        return;

    model_->appendEvent(ev);
    timelineView_->appendEvent(ev);
}

// ---------- Timeline hover → table selection ----------

void MainWindow::onTimelineEventHovered(int index)
{
    QItemSelectionModel *sel = tableView_->selectionModel();
    if (!sel)
        return;

    if (index < 0 || index >= model_->rowCount()) {
        sel->clearSelection();
        return;
    }

    const QModelIndex rowIndex = model_->index(index, 0);
    sel->setCurrentIndex(rowIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    tableView_->scrollTo(rowIndex, QAbstractItemView::PositionAtCenter);
    contextRow_ = index;
}

// ---------- Context menu + copy/export ----------

void MainWindow::onTableContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = tableView_->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    contextRow_ = index.row();

    QMenu menu(this);
    QAction *copyTextAct = menu.addAction(QStringLiteral("Copy event (text)"));
    QAction *copyJsonAct = menu.addAction(QStringLiteral("Copy event (JSON)"));

    QAction *chosen = menu.exec(tableView_->viewport()->mapToGlobal(pos));
    if (!chosen)
        return;

    if (chosen == copyTextAct) {
        copyEventText();
    } else if (chosen == copyJsonAct) {
        copyEventJson();
    }
}

QString MainWindow::eventToText(const kpulse::Event &ev) const
{
    QString line;
    line += ev.timestamp.toString(Qt::ISODateWithMs);
    line += QStringLiteral(" | ");
    line += kpulse::categoryToString(ev.category);
    line += QStringLiteral(" | ");
    line += kpulse::severityToString(ev.severity);
    line += QStringLiteral(" | ");
    line += ev.label;
    return line;
}

QString MainWindow::eventToJsonString(const kpulse::Event &ev) const
{
    return kpulse::eventToJsonString(ev);
}

void MainWindow::copyEventText()
{
    if (contextRow_ < 0)
        return;
    const kpulse::Event ev = model_->eventAt(contextRow_);
    const QString text = eventToText(ev);
    QClipboard *cb = QGuiApplication::clipboard();
    cb->setText(text, QClipboard::Clipboard);
}

void MainWindow::copyEventJson()
{
    if (contextRow_ < 0)
        return;
    const kpulse::Event ev = model_->eventAt(contextRow_);
    const QString text = eventToJsonString(ev);
    QClipboard *cb = QGuiApplication::clipboard();
    cb->setText(text, QClipboard::Clipboard);
}

void MainWindow::exportCsv()
{
    const QString fileName = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Export events to CSV"),
        QStringLiteral("kpulse_events.csv"),
        QStringLiteral("CSV files (*.csv);;All files (*)")
    );
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::warning(this,
                             QStringLiteral("Export failed"),
                             QStringLiteral("Could not open file for writing."));
        return;
    }

    QTextStream out(&file);
    out << "timestamp,category,severity,label\n";

    const auto &events = model_->events();
    for (const kpulse::Event &ev : events) {
        const QString ts = ev.timestamp.toString(Qt::ISODateWithMs);
        const QString cat = kpulse::categoryToString(ev.category);
        const QString sev = kpulse::severityToString(ev.severity);
        QString label = ev.label;
        QString safeLabel = label;
        safeLabel.replace('\"', "\"\"");
        // Very simple CSV escaping: wrap all fields in quotes
        out << "\"" << ts << "\","
            << "\"" << cat << "\","
            << "\"" << sev << "\","
            << "\"" << safeLabel << "\"\n";
    }

    file.close();
}
