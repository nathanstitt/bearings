#pragma once

#include "Config.h"
#include "Geofence.h"
#include "GeofenceStore.h"
#include "Location.h"
#include "LocationStore.h"
#include "PlatformBridge.h"
#include "Schedule.h"

#include <memory>
#include <string>

namespace bearings {

enum class MotionState { UNKNOWN, MOVING, STATIONARY };

enum class ActivityType {
    UNKNOWN = 0,
    STATIONARY = 1,
    WALKING = 2,
    RUNNING = 3,
    CYCLING = 4,
    AUTOMOTIVE = 5,
};

bool isMovingForActivity(ActivityType type);
std::string activityTypeToString(ActivityType type);
ActivityType activityTypeFromInt(int type);

class BearingsCore {
public:
    explicit BearingsCore(std::shared_ptr<PlatformBridge> bridge);
    ~BearingsCore();

    void configure(const std::string& configJson);
    void start();
    void stop();
    std::string getState();
    std::string getLocations();
    int getCount();
    bool destroyLocations();

    void onLocationReceived(const Location& location);

    // Geofence API
    bool addGeofence(const std::string& json);
    bool removeGeofence(const std::string& identifier);
    bool removeAllGeofences();
    std::string getGeofences();
    void onGeofenceEvent(const std::string& identifier, const std::string& action);

    // Schedule API
    void startSchedule();
    void stopSchedule();
    void onScheduleTimerFired(int year, int month, int day, int dayOfWeek,
                              int hour, int minute, int second);

    // Callbacks from platform layer
    void onMotionDetected(int activityType, int confidence);
    void onStopTimerFired();
    void onGeofenceExit();

private:
    void transitionToMoving();
    void transitionToStationary();
    void dispatchMotionChange();
    void dispatchActivityChange();

    std::shared_ptr<PlatformBridge> bridge_;
    Config config_;
    LocationStore store_;
    GeofenceStore geofenceStore_;
    bool tracking_ = false;
    bool configured_ = false;

    MotionState motionState_ = MotionState::UNKNOWN;
    Location lastLocation_;
    bool hasLastLocation_ = false;
    Location lastDispatchedLocation_;
    bool hasLastDispatchedLocation_ = false;
    bool stopTimerRunning_ = false;

    ActivityType currentActivity_ = ActivityType::UNKNOWN;
    int currentActivityConfidence_ = 0;

    Schedule schedule_;
    bool schedulerRunning_ = false;
    bool schedulerTracking_ = false;
};

} // namespace bearings
