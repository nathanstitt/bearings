#include "geomony/GeofenceEventStore.h"
#include "geomony/Logger.h"
#include "geomony_sqlite3/sqlite3.h"

namespace geomony {

GeofenceEventStore::GeofenceEventStore() = default;

GeofenceEventStore::~GeofenceEventStore() {
    close();
}

bool GeofenceEventStore::open(const std::string& dbPath) {
    if (db_) close();

    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        Logger::instance().error("Failed to open geofence event database: " + std::string(sqlite3_errmsg(db_)));
        db_ = nullptr;
        return false;
    }

    createTable();
    return true;
}

void GeofenceEventStore::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

void GeofenceEventStore::createTable() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS geofence_events ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  identifier TEXT NOT NULL,"
        "  action TEXT NOT NULL,"
        "  latitude REAL,"
        "  longitude REAL,"
        "  accuracy REAL,"
        "  extras TEXT,"
        "  timestamp TEXT NOT NULL,"
        "  synced INTEGER NOT NULL DEFAULT 0,"
        "  created_at TEXT NOT NULL"
        ");";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        Logger::instance().error("Failed to create geofence_events table: " + std::string(errMsg));
        sqlite3_free(errMsg);
    }
}

bool GeofenceEventStore::insert(const GeofenceEventRecord& event) {
    if (!db_) return false;

    const char* sql =
        "INSERT INTO geofence_events "
        "(identifier, action, latitude, longitude, accuracy, extras, timestamp, synced, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        Logger::instance().error("Failed to prepare geofence event insert: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    sqlite3_bind_text(stmt, 1, event.identifier.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, event.action.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, event.latitude);
    sqlite3_bind_double(stmt, 4, event.longitude);
    sqlite3_bind_double(stmt, 5, event.accuracy);
    sqlite3_bind_text(stmt, 6, event.extras.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, event.timestamp.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 8, event.synced ? 1 : 0);
    sqlite3_bind_text(stmt, 9, event.createdAt.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        Logger::instance().error("Failed to insert geofence event: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    return true;
}

std::vector<GeofenceEventRecord> GeofenceEventStore::getAll() {
    std::vector<GeofenceEventRecord> events;
    if (!db_) return events;

    const char* sql = "SELECT * FROM geofence_events ORDER BY id ASC;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return events;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        events.push_back(rowToRecord(stmt));
    }
    sqlite3_finalize(stmt);
    return events;
}

int GeofenceEventStore::getCount() {
    if (!db_) return 0;

    const char* sql = "SELECT COUNT(*) FROM geofence_events;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return 0;

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

bool GeofenceEventStore::destroyAll() {
    if (!db_) return false;

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, "DELETE FROM geofence_events;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        Logger::instance().error("Failed to destroy geofence events: " + std::string(errMsg));
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

std::vector<GeofenceEventRecord> GeofenceEventStore::getUnsynced(int limit) {
    std::vector<GeofenceEventRecord> events;
    if (!db_) return events;

    const char* sql = "SELECT * FROM geofence_events WHERE synced = 0 ORDER BY id ASC LIMIT ?;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return events;

    sqlite3_bind_int(stmt, 1, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        events.push_back(rowToRecord(stmt));
    }
    sqlite3_finalize(stmt);
    return events;
}

int GeofenceEventStore::getUnsyncedCount() {
    if (!db_) return 0;

    const char* sql = "SELECT COUNT(*) FROM geofence_events WHERE synced = 0;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return 0;

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

bool GeofenceEventStore::markSynced(const std::vector<int64_t>& ids) {
    if (!db_ || ids.empty()) return false;

    char* errMsg = nullptr;
    sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
    if (errMsg) { sqlite3_free(errMsg); return false; }

    const char* sql = "UPDATE geofence_events SET synced = 1 WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }

    for (int64_t id : ids) {
        sqlite3_reset(stmt);
        sqlite3_bind_int64(stmt, 1, id);
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
            return false;
        }
    }
    sqlite3_finalize(stmt);

    sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
    if (errMsg) {
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

GeofenceEventRecord GeofenceEventStore::rowToRecord(sqlite3_stmt* stmt) {
    GeofenceEventRecord ev;
    ev.id = sqlite3_column_int64(stmt, 0);
    ev.identifier = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    ev.action = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    ev.latitude = sqlite3_column_double(stmt, 3);
    ev.longitude = sqlite3_column_double(stmt, 4);
    ev.accuracy = sqlite3_column_double(stmt, 5);

    const char* extras = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    ev.extras = extras ? extras : "";

    ev.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    ev.synced = sqlite3_column_int(stmt, 8) != 0;
    ev.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));

    return ev;
}

} // namespace geomony
