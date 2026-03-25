#pragma once

#include <string>

namespace geomony {

struct Config;

class PlatformBridge {
public:
    virtual ~PlatformBridge() = default;

    virtual void startLocationUpdates(double desiredAccuracy, double distanceFilter) = 0;
    virtual void stopLocationUpdates() = 0;
    virtual void dispatchEvent(const std::string& name, const std::string& json) = 0;
    virtual std::string getDatabasePath() = 0;

    virtual void startMotionActivity() = 0;
    virtual void stopMotionActivity() = 0;
    virtual void startStationaryGeofence(double lat, double lon, double radius) = 0;
    virtual void stopStationaryGeofence() = 0;
    virtual void startStopTimer(int seconds) = 0;
    virtual void cancelStopTimer() = 0;

    virtual void addGeofence(const std::string& identifier, double lat, double lon,
        double radius, bool notifyOnEntry, bool notifyOnExit,
        bool notifyOnDwell, int loiteringDelay) = 0;
    virtual void removeGeofence(const std::string& identifier) = 0;
    virtual void removeAllGeofences() = 0;

    virtual void startScheduleTimer(int delaySeconds) = 0;
    virtual void cancelScheduleTimer() = 0;

    virtual void sendHTTPRequest(const std::string& url,
                                 const std::string& jsonPayload,
                                 int requestId) = 0;
    virtual void startSyncRetryTimer(int delaySeconds) = 0;
    virtual void cancelSyncRetryTimer() = 0;
};

} // namespace geomony
