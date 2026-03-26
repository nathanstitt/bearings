#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace geomony {

struct GeofenceEventRecord {
    int64_t id = 0;
    std::string identifier;
    std::string action;
    double latitude = 0.0;
    double longitude = 0.0;
    double accuracy = -1.0;
    std::string extras;
    std::string timestamp;
    bool synced = false;
    std::string createdAt;

    std::string toJson() const;
    static std::string toJsonArray(const std::vector<GeofenceEventRecord>& events);
};

} // namespace geomony
