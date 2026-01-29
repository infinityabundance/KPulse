#pragma once

#include <QDateTime>
#include <QMainWindow>

#include "kpulse/ipc_client.hpp"

class QComboBox;
class QPushButton;
class QTableView;
class QTextEdit;
class QLineEdit;
class QCheckBox;
class EventModel;
class TimelineView;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void refreshEvents();
    void handleLiveEvent(const kpulse::Event &event);
    void handleTableSelectionChanged();
    void handleTimelineClicked(const kpulse::Event &event);
    void handleFilterChanged();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUi();
    void updateTimeRange(QDateTime &from, QDateTime &to) const;
    void showEventDetails(const kpulse::Event &event);

    kpulse::IpcClient ipc_;
    EventModel *model_ = nullptr;
    TimelineView *timelineView_ = nullptr;
    QTableView *tableView_ = nullptr;
    QComboBox *rangeCombo_ = nullptr;
    QPushButton *refreshButton_ = nullptr;
    QTextEdit *detailsEdit_ = nullptr;
    QLineEdit *searchEdit_ = nullptr;

    QCheckBox *checkboxSystem_ = nullptr;
    QCheckBox *checkboxGPU_ = nullptr;
    QCheckBox *checkboxThermal_ = nullptr;
    QCheckBox *checkboxProcess_ = nullptr;
    QCheckBox *checkboxUpdate_ = nullptr;
    QCheckBox *checkboxNetwork_ = nullptr;
};
