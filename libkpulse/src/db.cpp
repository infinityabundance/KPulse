#include "kpulse/db.hpp"

#include "kpulse/event.hpp"

#include <QJsonDocument>
#include <QMetaType>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtGlobal>

namespace kpulse {

namespace {
constexpr const char *kConnectionName = "kpulse_event_store";
}

EventStore::EventStore(const QString &dbPath)
    : dbPath_(dbPath)
{
}

bool EventStore::ensureConnection()
{
    if (!db_.isValid()) {
        if (QSqlDatabase::contains(QLatin1String(kConnectionName))) {
            db_ = QSqlDatabase::database(QLatin1String(kConnectionName));
        } else {
            db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QLatin1String(kConnectionName));
        }
        db_.setDatabaseName(dbPath_);
    }

    if (!db_.isOpen()) {
        if (!db_.open()) {
            qWarning() << "Failed to open KPulse database" << db_.lastError().text();
            return false;
        }
    }

    return true;
}

bool EventStore::open()
{
    return ensureConnection();
}

bool EventStore::initSchema()
{
    if (!ensureConnection()) {
        return false;
    }

    const QStringList statements = {
        QStringLiteral("CREATE TABLE IF NOT EXISTS events ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "timestamp_ms INTEGER NOT NULL, "
                       "category TEXT NOT NULL, "
                       "severity TEXT NOT NULL, "
                       "label TEXT NOT NULL, "
                       "details TEXT NOT NULL, "
                       "window_id INTEGER"
                       ")"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_events_timestamp ON events(timestamp_ms)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_events_category ON events(category)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_events_window ON events(window_id)")
    };

    for (const QString &statement : statements) {
        QSqlQuery query(db_);
        if (!query.exec(statement)) {
            qWarning() << "Failed to init schema" << query.lastError().text();
            return false;
        }
    }

    return true;
}

bool EventStore::insertEvent(const Event &event, qint64 *outId)
{
    if (!ensureConnection()) {
        return false;
    }

    QSqlQuery query(db_);
    query.prepare(QStringLiteral("INSERT INTO events (timestamp_ms, category, severity, label, details, window_id) "
                                 "VALUES (?, ?, ?, ?, ?, ?)"));

    const qint64 timestampMs = event.timestamp.toMSecsSinceEpoch();
    query.addBindValue(timestampMs);
    query.addBindValue(categoryToString(event.category));
    query.addBindValue(severityToString(event.severity));
    query.addBindValue(event.label);

    QJsonDocument detailsDoc(event.details);
    query.addBindValue(QString::fromUtf8(detailsDoc.toJson(QJsonDocument::Compact)));

    if (event.windowId.has_value()) {
        query.addBindValue(static_cast<qint64>(event.windowId.value()));
    } else {
        query.addBindValue(QVariant(QMetaType::LongLong));
    }

    if (!query.exec()) {
        qWarning() << "Failed to insert event" << query.lastError().text();
        return false;
    }

    if (outId != nullptr) {
        *outId = query.lastInsertId().toLongLong();
    }

    return true;
}

std::vector<Event> EventStore::queryEvents(const QDateTime &from,
                                           const QDateTime &to,
                                           const std::vector<Category> &categories)
{
    std::vector<Event> results;

    if (!ensureConnection()) {
        return results;
    }

    QString statement = QStringLiteral("SELECT id, timestamp_ms, category, severity, label, details, window_id "
                                       "FROM events "
                                       "WHERE timestamp_ms >= ? AND timestamp_ms <= ?");

    if (!categories.empty()) {
        QStringList placeholders;
        placeholders.reserve(static_cast<int>(categories.size()));
        for (size_t i = 0; i < categories.size(); ++i) {
            placeholders.append(QStringLiteral("?"));
        }
        statement += QStringLiteral(" AND category IN (") + placeholders.join(QStringLiteral(", ")) +
                     QStringLiteral(")");
    }

    statement += QStringLiteral(" ORDER BY timestamp_ms ASC");

    QSqlQuery query(db_);
    query.prepare(statement);
    query.addBindValue(from.toMSecsSinceEpoch());
    query.addBindValue(to.toMSecsSinceEpoch());

    for (Category category : categories) {
        query.addBindValue(categoryToString(category));
    }

    if (!query.exec()) {
        qWarning() << "Failed to query events" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        Event ev;
        ev.id = query.value(0).toLongLong();
        const qint64 timestampMs = query.value(1).toLongLong();
        ev.timestamp = QDateTime::fromMSecsSinceEpoch(timestampMs, Qt::UTC);
        ev.category = categoryFromString(query.value(2).toString());
        ev.severity = severityFromString(query.value(3).toString());
        ev.label = query.value(4).toString();

        const QJsonDocument detailsDoc = QJsonDocument::fromJson(query.value(5).toString().toUtf8());
        if (detailsDoc.isObject()) {
            ev.details = detailsDoc.object();
        }

        if (!query.value(6).isNull()) {
            ev.windowId = query.value(6).toLongLong();
        }

        results.push_back(ev);
    }

    return results;
}

} // namespace kpulse
