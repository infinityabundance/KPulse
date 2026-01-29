#include "kpulse_daemon.hpp"

#include "kpulse/common.hpp"
#include "kpulse/db.hpp"
#include "kpulse/event.hpp"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtGlobal>

namespace kpulse {

KPulseDaemon::KPulseDaemon(const QString &dbPath, QObject *parent)
    : QObject(parent)
    , dbPath_(dbPath)
    , store_(dbPath)
    , journald_(this)
    , metrics_(this)
{
    connect(&journald_, &JournaldReader::eventDetected, this, &KPulseDaemon::handleEventDetected);
    connect(&metrics_, &MetricsCollector::eventDetected, this, &KPulseDaemon::handleEventDetected);
}

bool KPulseDaemon::init()
{
    if (!store_.open()) {
        qCritical() << "KPulseDaemon: failed to open DB at" << dbPath_;
        return false;
    }
    if (!store_.initSchema()) {
        qCritical() << "KPulseDaemon: failed to init DB schema";
        return false;
    }

    if (!journald_.init()) {
        qWarning() << "KPulseDaemon: failed to init JournaldReader";
    }

    journald_.start();
    metrics_.start();

    return true;
}

QString KPulseDaemon::GetEvents(qint64 fromMs, qint64 toMs, const QStringList &categories)
{
    const QDateTime from = QDateTime::fromMSecsSinceEpoch(fromMs, Qt::UTC);
    const QDateTime to = QDateTime::fromMSecsSinceEpoch(toMs, Qt::UTC);

    std::vector<Category> catEnums;
    catEnums.reserve(categories.size());
    for (const QString &s : categories) {
        catEnums.push_back(categoryFromString(s));
    }

    const auto events = store_.queryEvents(from, to, catEnums);

    QJsonArray arr;
    for (const auto &ev : events) {
        arr.append(eventToJson(ev));
    }

    QJsonDocument doc(arr);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

void KPulseDaemon::handleEventDetected(const kpulse::Event &event)
{
    Event ev = event;
    if (!ev.timestamp.isValid()) {
        ev.timestamp = nowUtc();
    }

    qint64 id = 0;
    if (!store_.insertEvent(ev, &id)) {
        qWarning() << "KPulseDaemon: failed to store event";
        return;
    }

    const QString json = eventToJsonString(ev);
    emit EventAdded(json);
}

} // namespace kpulse
