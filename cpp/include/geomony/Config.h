#pragma once

#include <string>
#include <vector>

namespace geomony {

struct Config {
    double desiredAccuracy = -1;   // -1 = best, -2 = 10m, -3 = 100m, etc.
    double distanceFilter = 10.0;  // Minimum distance (m) between updates
    double stationaryRadius = 25.0;
    double stopTimeout = 5.0;          // minutes of "still" before MOVING→STATIONARY
    bool debug = false;
    bool stopOnTerminate = true;
    bool startOnBoot = false;
    std::string url;
    int syncThreshold = 5;
    int maxBatchSize = 100;
    int syncRetryBaseSeconds = 10;
    bool enabled = false;
    std::vector<std::string> schedule;
    bool scheduleUseAlarmManager = false;

    static Config fromJson(const std::string& json);
    std::string toJson() const;
    void merge(const std::string& json);
};

} // namespace geomony
