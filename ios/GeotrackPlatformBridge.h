#pragma once

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>
#include "geotrack/PlatformBridge.h"
#include <memory>

@class Geotrack;

class GeotrackPlatformBridgeImpl : public geotrack::PlatformBridge {
public:
    GeotrackPlatformBridgeImpl(Geotrack* module);

    void startLocationUpdates(double desiredAccuracy, double distanceFilter) override;
    void stopLocationUpdates() override;
    void dispatchEvent(const std::string& name, const std::string& json) override;
    std::string getDatabasePath() override;

    void startMotionActivity() override;
    void stopMotionActivity() override;
    void startStationaryGeofence(double lat, double lon, double radius) override;
    void stopStationaryGeofence() override;
    void startStopTimer(int seconds) override;
    void cancelStopTimer() override;

    void addGeofence(const std::string& identifier, double lat, double lon,
        double radius, bool notifyOnEntry, bool notifyOnExit,
        bool notifyOnDwell, int loiteringDelay) override;
    void removeGeofence(const std::string& identifier) override;
    void removeAllGeofences() override;

    void startScheduleTimer(int delaySeconds) override;
    void cancelScheduleTimer() override;

private:
    __weak Geotrack* module_;
};
