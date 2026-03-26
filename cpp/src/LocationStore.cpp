#include "geomony/LocationStore.h"
#include "geomony/Logger.h"
#ifdef __APPLE__
#include <sqlite3.h>
#else
#include "geomony_sqlite3/sqlite3.h"
#endif

namespace geomony {

LocationStore::LocationStore() = default;

LocationStore::~LocationStore() {
    close();
}

bool LocationStore::open(const std::string& dbPath) {
    if (db_) close();

    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        Logger::instance().error("Failed to open database: " + std::string(sqlite3_errmsg(db_)));
        db_ = nullptr;
        return false;
    }

    createTable();
    Logger::instance().info("Database opened: " + dbPath);
    return true;
}

void LocationStore::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

void LocationStore::createTable() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS locations ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  uuid TEXT NOT NULL,"
        "  timestamp TEXT NOT NULL,"
        "  latitude REAL NOT NULL,"
        "  longitude REAL NOT NULL,"
        "  altitude REAL,"
        "  speed REAL,"
        "  heading REAL,"
        "  accuracy REAL,"
        "  speed_accuracy REAL,"
        "  heading_accuracy REAL,"
        "  altitude_accuracy REAL,"
        "  is_moving INTEGER NOT NULL DEFAULT 0,"
        "  event TEXT,"
        "  extras TEXT,"
        "  synced INTEGER NOT NULL DEFAULT 0,"
        "  created_at TEXT NOT NULL"
        ");";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        Logger::instance().error("Failed to create table: " + std::string(errMsg));
        sqlite3_free(errMsg);
    }

    // Schema migration: add activity columns if missing
    const char* migrations[] = {
        "ALTER TABLE locations ADD COLUMN activity_type TEXT DEFAULT 'unknown';",
        "ALTER TABLE locations ADD COLUMN activity_confidence INTEGER DEFAULT 0;",
    };
    for (const char* migrationSql : migrations) {
        errMsg = nullptr;
        sqlite3_exec(db_, migrationSql, nullptr, nullptr, &errMsg);
        if (errMsg) {
            // "duplicate column name" is expected on subsequent opens
            sqlite3_free(errMsg);
        }
    }
}

bool LocationStore::insert(const Location& location) {
    if (!db_) return false;

    const char* sql =
        "INSERT INTO locations "
        "(uuid, timestamp, latitude, longitude, altitude, speed, heading, "
        "accuracy, speed_accuracy, heading_accuracy, altitude_accuracy, "
        "is_moving, event, extras, synced, created_at, activity_type, activity_confidence) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        Logger::instance().error("Failed to prepare insert: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    sqlite3_bind_text(stmt, 1, location.uuid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, location.timestamp.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, location.latitude);
    sqlite3_bind_double(stmt, 4, location.longitude);
    sqlite3_bind_double(stmt, 5, location.altitude);
    sqlite3_bind_double(stmt, 6, location.speed);
    sqlite3_bind_double(stmt, 7, location.heading);
    sqlite3_bind_double(stmt, 8, location.accuracy);
    sqlite3_bind_double(stmt, 9, location.speedAccuracy);
    sqlite3_bind_double(stmt, 10, location.headingAccuracy);
    sqlite3_bind_double(stmt, 11, location.altitudeAccuracy);
    sqlite3_bind_int(stmt, 12, location.isMoving ? 1 : 0);
    sqlite3_bind_text(stmt, 13, location.event.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 14, location.extras.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 15, location.synced ? 1 : 0);
    sqlite3_bind_text(stmt, 16, location.createdAt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 17, location.activityType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 18, location.activityConfidence);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        Logger::instance().error("Failed to insert location: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    return true;
}

std::vector<Location> LocationStore::getAll() {
    std::vector<Location> locations;
    if (!db_) return locations;

    const char* sql = "SELECT * FROM locations ORDER BY id ASC;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        Logger::instance().error("Failed to prepare getAll: " + std::string(sqlite3_errmsg(db_)));
        return locations;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        locations.push_back(rowToLocation(stmt));
    }
    sqlite3_finalize(stmt);

    return locations;
}

int LocationStore::getCount() {
    if (!db_) return 0;

    const char* sql = "SELECT COUNT(*) FROM locations;";
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

bool LocationStore::destroyAll() {
    if (!db_) return false;

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, "DELETE FROM locations;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        Logger::instance().error("Failed to destroy locations: " + std::string(errMsg));
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

std::vector<Location> LocationStore::getUnsynced(int limit) {
    std::vector<Location> locations;
    if (!db_) return locations;

    const char* sql = "SELECT * FROM locations WHERE synced = 0 ORDER BY id ASC LIMIT ?;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        Logger::instance().error("Failed to prepare getUnsynced: " + std::string(sqlite3_errmsg(db_)));
        return locations;
    }

    sqlite3_bind_int(stmt, 1, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        locations.push_back(rowToLocation(stmt));
    }
    sqlite3_finalize(stmt);
    return locations;
}

int LocationStore::getUnsyncedCount() {
    if (!db_) return 0;

    const char* sql = "SELECT COUNT(*) FROM locations WHERE synced = 0;";
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

bool LocationStore::markSynced(const std::vector<int64_t>& ids) {
    if (!db_ || ids.empty()) return false;

    char* errMsg = nullptr;
    sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
    if (errMsg) { sqlite3_free(errMsg); return false; }

    const char* sql = "UPDATE locations SET synced = 1 WHERE id = ?;";
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

Location LocationStore::rowToLocation(sqlite3_stmt* stmt) {
    Location loc;
    loc.id = sqlite3_column_int64(stmt, 0);
    loc.uuid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    loc.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    loc.latitude = sqlite3_column_double(stmt, 3);
    loc.longitude = sqlite3_column_double(stmt, 4);
    loc.altitude = sqlite3_column_double(stmt, 5);
    loc.speed = sqlite3_column_double(stmt, 6);
    loc.heading = sqlite3_column_double(stmt, 7);
    loc.accuracy = sqlite3_column_double(stmt, 8);
    loc.speedAccuracy = sqlite3_column_double(stmt, 9);
    loc.headingAccuracy = sqlite3_column_double(stmt, 10);
    loc.altitudeAccuracy = sqlite3_column_double(stmt, 11);
    loc.isMoving = sqlite3_column_int(stmt, 12) != 0;

    const char* event = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
    loc.event = event ? event : "";

    const char* extras = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
    loc.extras = extras ? extras : "";

    loc.synced = sqlite3_column_int(stmt, 15) != 0;
    loc.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 16));

    // Activity columns (may be NULL on old rows)
    int colCount = sqlite3_column_count(stmt);
    if (colCount > 17) {
        const char* actType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 17));
        loc.activityType = actType ? actType : "unknown";
    }
    if (colCount > 18) {
        loc.activityConfidence = sqlite3_column_int(stmt, 18);
    }

    return loc;
}

} // namespace geomony
