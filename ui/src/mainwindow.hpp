#pragma once

#include <QMainWindow>
#include <QDateTime>
#include <QToolButton>

class QComboBox;
class QLabel;
class QPushButton;
class QTableView;

#include "event_model.hpp"
#include "kpulse/ipc_client.hpp"
#include "timeline_view.hpp"

// Main window with timeline, table, context menu, CSV export,
// live updates, and an About dialog.
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onRefreshClicked();
    void onTimeRangeChanged(int index);

    // Context menu + actions
    void onTableContextMenuRequested(const QPoint &pos);
    void copyEventText();
    void copyEventJson();
    void exportCsv();

    // Live update from daemon
    void onEventReceived(const kpulse::Event &ev);

    // Hover from timeline
    void onTimelineEventHovered(int index);
    void onDaemonToggleClicked();
    void onStartTrayClicked();
    void refreshDaemonStatus();
    void showAboutDialog();

private:
    void updateTimeRange(QDateTime &from, QDateTime &to) const;
    void loadEvents();

    QString eventToText(const kpulse::Event &ev) const;
    QString eventToJsonString(const kpulse::Event &ev) const;

    QComboBox *rangeCombo_ = nullptr;
    QPushButton *refreshButton_ = nullptr;
    QPushButton *exportButton_ = nullptr;
    QPushButton *daemonToggleButton_ = nullptr;
    QPushButton *startTrayButton_ = nullptr;
    QLabel *daemonStatusLabel_ = nullptr;
    QToolButton *aboutButton_ = nullptr;
    QTableView *tableView_ = nullptr;
    TimelineView *timelineView_ = nullptr;

    EventModel *model_ = nullptr;
    kpulse::IpcClient *ipcClient_ = nullptr;

    // Last known selection row for context menu actions
    int contextRow_ = -1;
};
