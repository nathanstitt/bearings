#pragma once

#include "Location.h"
#include <vector>
#include <string>

struct sqlite3;
struct sqlite3_stmt;

namespace geotrack {

class LocationStore {
public:
    LocationStore();
    ~LocationStore();

    bool open(const std::string& dbPath);
    void close();

    bool insert(const Location& location);
    std::vector<Location> getAll();
    int getCount();
    bool destroyAll();

private:
    void createTable();
    Location rowToLocation(sqlite3_stmt* stmt);

    sqlite3* db_ = nullptr;
};

} // namespace geotrack
