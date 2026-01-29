#pragma once

#include <QDateTime>
#include <QMainWindow>

#include "kpulse/ipc_client.hpp"

class QComboBox;
class QPushButton;
class QTableView;
class EventModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void refreshEvents();
    void handleLiveEvent(const kpulse::Event &event);

private:
    void setupUi();
    void updateTimeRange(QDateTime &from, QDateTime &to) const;

    kpulse::IpcClient ipc_;
    EventModel *model_ = nullptr;
    QTableView *tableView_ = nullptr;
    QComboBox *rangeCombo_ = nullptr;
    QPushButton *refreshButton_ = nullptr;
};
