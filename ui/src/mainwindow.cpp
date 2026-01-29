#include "mainwindow.hpp"

#include "event_model.hpp"
#include "timeline_view.hpp"

#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QCloseEvent>
#include <QDate>
#include <QDateTime>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileDialog>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QTableView>
#include <QSysInfo>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();

    connect(&ipc_, &kpulse::IpcClient::eventReceived, this, &MainWindow::handleLiveEvent);

    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup uiGroup(config, "UI");

    restoreGeometry(uiGroup.readEntry("Geometry", QByteArray()));
    rangeCombo_->setCurrentIndex(uiGroup.readEntry("TimeRange", 1));
    searchEdit_->setText(uiGroup.readEntry("SearchText", QString()));

    checkboxSystem_->setChecked(uiGroup.readEntry("FilterSystem", true));
    checkboxGPU_->setChecked(uiGroup.readEntry("FilterGPU", true));
    checkboxThermal_->setChecked(uiGroup.readEntry("FilterThermal", true));
    checkboxProcess_->setChecked(uiGroup.readEntry("FilterProcess", true));
    checkboxUpdate_->setChecked(uiGroup.readEntry("FilterUpdate", true));
    checkboxNetwork_->setChecked(uiGroup.readEntry("FilterNetwork", true));

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
    searchEdit_ = new QLineEdit(topControlsAndTimeline);
    searchEdit_->setPlaceholderText(QStringLiteral("Filter by text (label/message)..."));
    controls->addWidget(rangeCombo_);
    controls->addWidget(refreshButton_);
    controls->addWidget(searchEdit_);
    controls->addStretch(1);

    topLayout->addLayout(controls);

    QHBoxLayout *filters = new QHBoxLayout();
    checkboxSystem_ = new QCheckBox(QStringLiteral("System"), topControlsAndTimeline);
    checkboxGPU_ = new QCheckBox(QStringLiteral("GPU"), topControlsAndTimeline);
    checkboxThermal_ = new QCheckBox(QStringLiteral("Thermal"), topControlsAndTimeline);
    checkboxProcess_ = new QCheckBox(QStringLiteral("Process"), topControlsAndTimeline);
    checkboxUpdate_ = new QCheckBox(QStringLiteral("Update"), topControlsAndTimeline);
    checkboxNetwork_ = new QCheckBox(QStringLiteral("Network"), topControlsAndTimeline);

    checkboxSystem_->setChecked(true);
    checkboxGPU_->setChecked(true);
    checkboxThermal_->setChecked(true);
    checkboxProcess_->setChecked(true);
    checkboxUpdate_->setChecked(true);
    checkboxNetwork_->setChecked(true);

    filters->addWidget(checkboxSystem_);
    filters->addWidget(checkboxGPU_);
    filters->addWidget(checkboxThermal_);
    filters->addWidget(checkboxProcess_);
    filters->addWidget(checkboxUpdate_);
    filters->addWidget(checkboxNetwork_);
    filters->addStretch(1);

    topLayout->addLayout(filters);

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

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *exportJsonAction = fileMenu->addAction(tr("Export as JSON…"));
    QAction *exportCsvAction = fileMenu->addAction(tr("Export as CSV…"));
    QAction *exportSnapshotAction = fileMenu->addAction(tr("Export Snapshot…"));

    connect(exportJsonAction, &QAction::triggered, this, &MainWindow::exportJson);
    connect(exportCsvAction, &QAction::triggered, this, &MainWindow::exportCsv);
    connect(exportSnapshotAction, &QAction::triggered, this, &MainWindow::exportSnapshot);

    connect(refreshButton_, &QPushButton::clicked, this, &MainWindow::refreshEvents);

    connect(searchEdit_, &QLineEdit::textChanged, this, &MainWindow::handleFilterChanged);
    connect(checkboxSystem_, &QCheckBox::stateChanged, this, &MainWindow::handleFilterChanged);
    connect(checkboxGPU_, &QCheckBox::stateChanged, this, &MainWindow::handleFilterChanged);
    connect(checkboxThermal_, &QCheckBox::stateChanged, this, &MainWindow::handleFilterChanged);
    connect(checkboxProcess_, &QCheckBox::stateChanged, this, &MainWindow::handleFilterChanged);
    connect(checkboxUpdate_, &QCheckBox::stateChanged, this, &MainWindow::handleFilterChanged);
    connect(checkboxNetwork_, &QCheckBox::stateChanged, this, &MainWindow::handleFilterChanged);

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
    if (checkboxSystem_ && checkboxSystem_->isChecked()) {
        categories.push_back(kpulse::Category::System);
    }
    if (checkboxGPU_ && checkboxGPU_->isChecked()) {
        categories.push_back(kpulse::Category::GPU);
    }
    if (checkboxThermal_ && checkboxThermal_->isChecked()) {
        categories.push_back(kpulse::Category::Thermal);
    }
    if (checkboxProcess_ && checkboxProcess_->isChecked()) {
        categories.push_back(kpulse::Category::Process);
    }
    if (checkboxUpdate_ && checkboxUpdate_->isChecked()) {
        categories.push_back(kpulse::Category::Update);
    }
    if (checkboxNetwork_ && checkboxNetwork_->isChecked()) {
        categories.push_back(kpulse::Category::Network);
    }

    if (checkboxSystem_ && checkboxGPU_ && checkboxThermal_ && checkboxProcess_ && checkboxUpdate_ &&
        checkboxNetwork_) {
        const bool anyChecked = checkboxSystem_->isChecked() || checkboxGPU_->isChecked() ||
                                checkboxThermal_->isChecked() || checkboxProcess_->isChecked() ||
                                checkboxUpdate_->isChecked() || checkboxNetwork_->isChecked();
        if (!anyChecked) {
            categories.clear();
        }
    }

    auto events = ipc_.getEvents(fromUtc, toUtc, categories);

    const QString filterText = searchEdit_ ? searchEdit_->text().trimmed() : QString();
    if (!filterText.isEmpty()) {
        const QString needle = filterText.toLower();
        std::vector<kpulse::Event> filtered;
        filtered.reserve(events.size());

        for (const auto &ev : events) {
            const QString label = ev.label;
            QString message;
            if (ev.details.contains(QStringLiteral("message"))) {
                message = ev.details.value(QStringLiteral("message")).toString();
            }

            if (label.toLower().contains(needle) || message.toLower().contains(needle)) {
                filtered.push_back(ev);
            }
        }

        events = std::move(filtered);
    }

    model_->setEvents(events);
    timelineView_->setEvents(events);

    if (events.empty()) {
        detailsEdit_->clear();
    }
}

void MainWindow::handleLiveEvent(const kpulse::Event &event)
{
    QDateTime fromUtc;
    QDateTime toUtc;
    updateTimeRange(fromUtc, toUtc);

    if (event.timestamp < fromUtc || event.timestamp > toUtc) {
        return;
    }

    const auto allowed = [&](kpulse::Category category) {
        switch (category) {
        case kpulse::Category::System:
            return checkboxSystem_->isChecked();
        case kpulse::Category::GPU:
            return checkboxGPU_->isChecked();
        case kpulse::Category::Thermal:
            return checkboxThermal_->isChecked();
        case kpulse::Category::Process:
            return checkboxProcess_->isChecked();
        case kpulse::Category::Update:
            return checkboxUpdate_->isChecked();
        case kpulse::Category::Network:
            return checkboxNetwork_->isChecked();
        }
        return true;
    };

    if (!allowed(event.category)) {
        return;
    }

    const QString needle = searchEdit_->text().trimmed().toLower();
    if (!needle.isEmpty()) {
        const QString label = event.label.toLower();
        const QString message = event.details.value(QStringLiteral("message")).toString().toLower();
        if (!label.contains(needle) && !message.contains(needle)) {
            return;
        }
    }

    model_->appendEvent(event);
    timelineView_->appendEvent(event);
}

void MainWindow::handleFilterChanged()
{
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

    if (event.isAnomalous) {
        text += QStringLiteral("\n⚠ Anomalous event detected\n");
        text += QStringLiteral("Anomaly score: %1\n").arg(event.anomalyScore, 0, 'f', 2);
    }

    QJsonDocument doc(event.details);
    text += QStringLiteral("Details:\n");
    text += QString::fromUtf8(doc.toJson(QJsonDocument::Indented));

    detailsEdit_->setPlainText(text);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup uiGroup(config, "UI");

    uiGroup.writeEntry("Geometry", saveGeometry());
    uiGroup.writeEntry("TimeRange", rangeCombo_->currentIndex());
    uiGroup.writeEntry("SearchText", searchEdit_->text());

    uiGroup.writeEntry("FilterSystem", checkboxSystem_->isChecked());
    uiGroup.writeEntry("FilterGPU", checkboxGPU_->isChecked());
    uiGroup.writeEntry("FilterThermal", checkboxThermal_->isChecked());
    uiGroup.writeEntry("FilterProcess", checkboxProcess_->isChecked());
    uiGroup.writeEntry("FilterUpdate", checkboxUpdate_->isChecked());
    uiGroup.writeEntry("FilterNetwork", checkboxNetwork_->isChecked());

    uiGroup.sync();
    QMainWindow::closeEvent(event);
}

void MainWindow::exportJson()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        tr("Export Events as JSON"),
        QStringLiteral("kpulse-events.json"),
        tr("JSON Files (*.json)"));

    if (path.isEmpty()) {
        return;
    }

    if (!model_->exportToJson(path)) {
        QMessageBox::critical(this, tr("Export Failed"), tr("Failed to write JSON file."));
    }
}

void MainWindow::exportCsv()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        tr("Export Events as CSV"),
        QStringLiteral("kpulse-events.csv"),
        tr("CSV Files (*.csv)"));

    if (path.isEmpty()) {
        return;
    }

    if (!model_->exportToCsv(path)) {
        QMessageBox::critical(this, tr("Export Failed"), tr("Failed to write CSV file."));
    }
}

void MainWindow::exportSnapshot()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        tr("Export KPulse Snapshot"),
        QStringLiteral("kpulse-snapshot.json"),
        tr("JSON Files (*.json)"));

    if (path.isEmpty()) {
        return;
    }

    QJsonObject root;
    root.insert(QStringLiteral("timestamp"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    root.insert(QStringLiteral("hostname"), QSysInfo::machineHostName());
    root.insert(QStringLiteral("kernel"), QSysInfo::kernelVersion());
    root.insert(QStringLiteral("kpulse"), QStringLiteral("0.1"));

    QJsonArray events;
    for (int i = 0; i < model_->rowCount(); ++i) {
        events.append(kpulse::eventToJson(model_->eventAt(i)));
    }
    root.insert(QStringLiteral("events"), events);

    QJsonDocument doc(root);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, tr("Export Failed"), tr("Failed to write snapshot file."));
        return;
    }

    f.write(doc.toJson(QJsonDocument::Indented));
}
