#pragma once

#include <string>
#include <vector>

namespace geomony {

struct Geofence {
    std::string identifier;
    double latitude = 0.0;
    double longitude = 0.0;
    double radius = 150.0;
    bool notifyOnEntry = true;
    bool notifyOnExit = true;
    bool notifyOnDwell = false;
    int loiteringDelay = 30000;
    std::string extras;

    static Geofence fromJson(const std::string& json);
    std::string toJson() const;
    static std::string toJsonArray(const std::vector<Geofence>& geofences);
};

} // namespace geomony
