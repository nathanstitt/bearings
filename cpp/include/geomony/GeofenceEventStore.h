#pragma once

#include "GeofenceEventRecord.h"
#include <vector>
#include <string>
#include <cstdint>

struct sqlite3;
struct sqlite3_stmt;

namespace geomony {

class GeofenceEventStore {
public:
    GeofenceEventStore();
    ~GeofenceEventStore();

    bool open(const std::string& dbPath);
    void close();

    bool insert(const GeofenceEventRecord& event);
    std::vector<GeofenceEventRecord> getAll();
    int getCount();
    bool destroyAll();
    std::vector<GeofenceEventRecord> getUnsynced(int limit);
    int getUnsyncedCount();
    bool markSynced(const std::vector<int64_t>& ids);

private:
    void createTable();
    GeofenceEventRecord rowToRecord(sqlite3_stmt* stmt);

    sqlite3* db_ = nullptr;
};

} // namespace geomony
