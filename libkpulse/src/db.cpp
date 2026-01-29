#include "kpulse/db.hpp"

#include "kpulse/event.hpp"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QDebug>

namespace kpulse {

namespace {
constexpr const char *kConnectionName = "kpulse_event_store";
constexpr int kSchemaVersion = 1;

QString lastErrorString(const QSqlDatabase &db)
{
    return db.lastError().text();
}

QString lastErrorString(const QSqlQuery &query)
{
    return query.lastError().text();
}

} // namespace

EventStore::EventStore(const QString &dbPath)
    : dbPath_(dbPath)
{
}

bool EventStore::ensureConnection()
{
    if (db_.isValid() && db_.isOpen()) {
        return true;
    }

    if (QSqlDatabase::contains(kConnectionName)) {
        db_ = QSqlDatabase::database(kConnectionName);
    } else {
        db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), kConnectionName);
    }

    db_.setDatabaseName(dbPath_);

    if (!db_.open()) {
        qWarning() << "EventStore: failed to open database:"
                   << dbPath_ << "-" << lastErrorString(db_);
        return false;
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

    QSqlQuery query(db_);

    // Base events table. Adjust column names/types if your schema differs.
    const char *createSql = R"(
        CREATE TABLE IF NOT EXISTS events (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp_ms INTEGER NOT NULL,
            category    INTEGER NOT NULL,
            severity    INTEGER NOT NULL,
            label       TEXT    NOT NULL,
            details     TEXT,
            window_id   INTEGER
        )
    )";

    if (!query.exec(QString::fromUtf8(createSql))) {
        qWarning() << "EventStore: failed to create events table:"
                   << lastErrorString(query);
        return false;
    }

    // Optional meta table for schema versioning
    const char *metaSql = R"(
        CREATE TABLE IF NOT EXISTS meta (
            key   TEXT PRIMARY KEY,
            value TEXT NOT NULL
        )
    )";

    if (!query.exec(QString::fromUtf8(metaSql))) {
        qWarning() << "EventStore: failed to create meta table:"
                   << lastErrorString(query);
        return false;
    }

    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO meta (key, value) VALUES ('schema_version', ?)"
    ));
    query.addBindValue(QString::number(kSchemaVersion));
    if (!query.exec()) {
        qWarning() << "EventStore: failed to write schema version:"
                   << lastErrorString(query);
        // Not fatal, but indicates DB mismatch.
    }

    return true;
}

bool EventStore::insertEvent(const Event &event, qint64 *outId)
{
    if (!ensureConnection()) {
        return false;
    }

    QSqlQuery query(db_);

    query.prepare(QStringLiteral(R"(
        INSERT INTO events (
            timestamp_ms,
            category,
            severity,
            label,
            details,
            window_id
        ) VALUES (?, ?, ?, ?, ?, ?)
    )"));

    // Timestamp stored as UTC msecs since epoch
    query.addBindValue(event.timestamp.toMSecsSinceEpoch());

    query.addBindValue(static_cast<int>(event.category));
    query.addBindValue(static_cast<int>(event.severity));
    query.addBindValue(event.label);

    // Details as compact JSON, or NULL if empty
    if (event.details.isEmpty()) {
        query.addBindValue(QVariant());  // NULL
    } else {
        QJsonDocument doc(event.details);
        query.addBindValue(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    }

    // Optional window id (assumes Event has std::optional<qint64> windowId)
    if (event.windowId.has_value()) {
        query.addBindValue(static_cast<qint64>(*event.windowId));
    } else {
        query.addBindValue(QVariant());  // NULL
    }

    if (!query.exec()) {
        qWarning() << "EventStore: insertEvent failed:" << lastErrorString(query);
        return false;
    }

    if (outId && query.lastInsertId().isValid()) {
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

    QSqlQuery query(db_);

    QString sql = QStringLiteral(
        "SELECT id, timestamp_ms, category, severity, label, details, window_id "
        "FROM events WHERE timestamp_ms BETWEEN ? AND ?"
    );

    // Optional category filter
    if (!categories.empty()) {
        sql += QStringLiteral(" AND category IN (");
        for (std::size_t i = 0; i < categories.size(); ++i) {
            if (i != 0) {
                sql += QStringLiteral(", ");
            }
            sql += QStringLiteral("?");
        }
        sql += QStringLiteral(")");
    }

    sql += QStringLiteral(" ORDER BY timestamp_ms ASC");

    if (!query.prepare(sql)) {
        qWarning() << "EventStore: queryEvents prepare failed:"
                   << lastErrorString(query);
        return results;
    }

    const qint64 fromMs = from.toMSecsSinceEpoch();
    const qint64 toMs   = to.toMSecsSinceEpoch();

    query.addBindValue(fromMs);
    query.addBindValue(toMs);

    for (Category cat : categories) {
        query.addBindValue(static_cast<int>(cat));
    }

    if (!query.exec()) {
        qWarning() << "EventStore: queryEvents exec failed:"
                   << lastErrorString(query);
        return results;
    }

    while (query.next()) {
        Event ev;

        const qint64 tsMs = query.value(1).toLongLong();
        ev.timestamp = QDateTime::fromMSecsSinceEpoch(tsMs, Qt::UTC); // warning-only in Qt6

        ev.category = static_cast<Category>(query.value(2).toInt());
        ev.severity = static_cast<Severity>(query.value(3).toInt());
        ev.label    = query.value(4).toString();

        const QString detailsJson = query.value(5).toString();
        if (!detailsJson.isEmpty()) {
            const QJsonDocument detailsDoc =
                QJsonDocument::fromJson(detailsJson.toUtf8());
            if (detailsDoc.isObject()) {
                ev.details = detailsDoc.object();
            }
        }

        if (!query.value(6).isNull()) {
            ev.windowId = query.value(6).toLongLong();
        }

        results.push_back(std::move(ev));
    }

    return results;
}

} // namespace kpulse
