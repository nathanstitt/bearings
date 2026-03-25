#include "geomony/GeofenceStore.h"
#include "geomony/Logger.h"
#include "sqlite3.h"

namespace geomony {

GeofenceStore::GeofenceStore() = default;

GeofenceStore::~GeofenceStore() {
    close();
}

bool GeofenceStore::open(const std::string& dbPath) {
    if (db_) close();

    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        Logger::instance().error("Failed to open geofence database: " + std::string(sqlite3_errmsg(db_)));
        db_ = nullptr;
        return false;
    }

    createTable();
    return true;
}

void GeofenceStore::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

void GeofenceStore::createTable() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS geofences ("
        "  identifier TEXT NOT NULL UNIQUE,"
        "  latitude REAL NOT NULL,"
        "  longitude REAL NOT NULL,"
        "  radius REAL NOT NULL,"
        "  notify_on_entry INTEGER NOT NULL DEFAULT 1,"
        "  notify_on_exit INTEGER NOT NULL DEFAULT 1,"
        "  notify_on_dwell INTEGER NOT NULL DEFAULT 0,"
        "  loitering_delay INTEGER NOT NULL DEFAULT 30000,"
        "  extras TEXT"
        ");";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        Logger::instance().error("Failed to create geofences table: " + std::string(errMsg));
        sqlite3_free(errMsg);
    }
}

bool GeofenceStore::insert(const Geofence& geofence) {
    if (!db_) return false;

    const char* sql =
        "INSERT OR REPLACE INTO geofences "
        "(identifier, latitude, longitude, radius, notify_on_entry, notify_on_exit, "
        "notify_on_dwell, loitering_delay, extras) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        Logger::instance().error("Failed to prepare geofence insert: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    sqlite3_bind_text(stmt, 1, geofence.identifier.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, geofence.latitude);
    sqlite3_bind_double(stmt, 3, geofence.longitude);
    sqlite3_bind_double(stmt, 4, geofence.radius);
    sqlite3_bind_int(stmt, 5, geofence.notifyOnEntry ? 1 : 0);
    sqlite3_bind_int(stmt, 6, geofence.notifyOnExit ? 1 : 0);
    sqlite3_bind_int(stmt, 7, geofence.notifyOnDwell ? 1 : 0);
    sqlite3_bind_int(stmt, 8, geofence.loiteringDelay);
    sqlite3_bind_text(stmt, 9, geofence.extras.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        Logger::instance().error("Failed to insert geofence: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    return true;
}

bool GeofenceStore::remove(const std::string& identifier) {
    if (!db_) return false;

    const char* sql = "DELETE FROM geofences WHERE identifier = ?;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, identifier.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool GeofenceStore::removeAll() {
    if (!db_) return false;

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, "DELETE FROM geofences;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        Logger::instance().error("Failed to remove all geofences: " + std::string(errMsg));
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

std::vector<Geofence> GeofenceStore::getAll() {
    std::vector<Geofence> geofences;
    if (!db_) return geofences;

    const char* sql = "SELECT * FROM geofences ORDER BY identifier ASC;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        Logger::instance().error("Failed to prepare geofence getAll: " + std::string(sqlite3_errmsg(db_)));
        return geofences;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        geofences.push_back(rowToGeofence(stmt));
    }
    sqlite3_finalize(stmt);

    return geofences;
}

Geofence GeofenceStore::get(const std::string& identifier) {
    Geofence g;
    if (!db_) return g;

    const char* sql = "SELECT * FROM geofences WHERE identifier = ?;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return g;

    sqlite3_bind_text(stmt, 1, identifier.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        g = rowToGeofence(stmt);
    }
    sqlite3_finalize(stmt);

    return g;
}

int GeofenceStore::getCount() {
    if (!db_) return 0;

    const char* sql = "SELECT COUNT(*) FROM geofences;";
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

Geofence GeofenceStore::rowToGeofence(sqlite3_stmt* stmt) {
    Geofence g;
    g.identifier = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    g.latitude = sqlite3_column_double(stmt, 1);
    g.longitude = sqlite3_column_double(stmt, 2);
    g.radius = sqlite3_column_double(stmt, 3);
    g.notifyOnEntry = sqlite3_column_int(stmt, 4) != 0;
    g.notifyOnExit = sqlite3_column_int(stmt, 5) != 0;
    g.notifyOnDwell = sqlite3_column_int(stmt, 6) != 0;
    g.loiteringDelay = sqlite3_column_int(stmt, 7);

    const char* extras = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
    g.extras = extras ? extras : "";

    return g;
}

} // namespace geomony
