#pragma once

#include "Location.h"
#include <vector>
#include <string>

struct sqlite3;
struct sqlite3_stmt;

namespace geomony {

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
    std::vector<Location> getUnsynced(int limit);
    int getUnsyncedCount();
    bool markSynced(const std::vector<int64_t>& ids);

private:
    void createTable();
    Location rowToLocation(sqlite3_stmt* stmt);

    sqlite3* db_ = nullptr;
};

} // namespace geomony
