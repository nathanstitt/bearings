#pragma once

#include "Geofence.h"
#include <vector>
#include <string>

struct sqlite3;
struct sqlite3_stmt;

namespace geotrack {

class GeofenceStore {
public:
    GeofenceStore();
    ~GeofenceStore();

    bool open(const std::string& dbPath);
    void close();

    bool insert(const Geofence& geofence);
    bool remove(const std::string& identifier);
    bool removeAll();
    std::vector<Geofence> getAll();
    Geofence get(const std::string& identifier);
    int getCount();

private:
    void createTable();
    Geofence rowToGeofence(sqlite3_stmt* stmt);

    sqlite3* db_ = nullptr;
};

} // namespace geotrack
