#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace bearings {

struct Location {
    int64_t id = 0;
    std::string uuid;
    std::string timestamp;
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    double speed = -1.0;
    double heading = -1.0;
    double accuracy = -1.0;
    double speedAccuracy = -1.0;
    double headingAccuracy = -1.0;
    double altitudeAccuracy = -1.0;
    bool isMoving = false;
    std::string activityType = "unknown";
    int activityConfidence = 0;
    std::string event;
    std::string extras;
    bool synced = false;
    std::string createdAt;

    static Location fromJson(const std::string& json);
    std::string toJson() const;
    static std::string toJsonArray(const std::vector<Location>& locations);

    /// Haversine distance in meters to another location.
    double distanceTo(const Location& other) const;
};

} // namespace bearings
