#include "bearings/BearingsCore.h"
#include "bearings/Logger.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace bearings {

bool isMovingForActivity(ActivityType type) {
    switch (type) {
        case ActivityType::WALKING:
        case ActivityType::RUNNING:
        case ActivityType::CYCLING:
        case ActivityType::AUTOMOTIVE:
            return true;
        case ActivityType::STATIONARY:
        case ActivityType::UNKNOWN:
        default:
            return false;
    }
}

std::string activityTypeToString(ActivityType type) {
    switch (type) {
        case ActivityType::STATIONARY: return "stationary";
        case ActivityType::WALKING:    return "walking";
        case ActivityType::RUNNING:    return "running";
        case ActivityType::CYCLING:    return "cycling";
        case ActivityType::AUTOMOTIVE: return "automotive";
        case ActivityType::UNKNOWN:
        default:                       return "unknown";
    }
}

ActivityType activityTypeFromInt(int type) {
    switch (type) {
        case 1: return ActivityType::STATIONARY;
        case 2: return ActivityType::WALKING;
        case 3: return ActivityType::RUNNING;
        case 4: return ActivityType::CYCLING;
        case 5: return ActivityType::AUTOMOTIVE;
        default: return ActivityType::UNKNOWN;
    }
}

BearingsCore::BearingsCore(std::shared_ptr<PlatformBridge> bridge)
    : bridge_(std::move(bridge)) {
}

BearingsCore::~BearingsCore() {
    if (schedulerRunning_) {
        stopSchedule();
    }
    if (tracking_) {
        stop();
    }
}

void BearingsCore::configure(const std::string& configJson) {
    config_.merge(configJson);

    if (!configured_) {
        std::string dbPath = bridge_->getDatabasePath();
        if (!store_.open(dbPath)) {
            Logger::instance().error("Failed to open location database");
            return;
        }
        if (!geofenceStore_.open(dbPath)) {
            Logger::instance().error("Failed to open geofence database");
            return;
        }
        configured_ = true;

        // Re-register persisted geofences with the platform
        auto geofences = geofenceStore_.getAll();
        for (const auto& g : geofences) {
            bridge_->addGeofence(g.identifier, g.latitude, g.longitude,
                g.radius, g.notifyOnEntry, g.notifyOnExit,
                g.notifyOnDwell, g.loiteringDelay);
        }
        if (!geofences.empty()) {
            Logger::instance().info("Re-registered " + std::to_string(geofences.size()) + " geofences");
        }
    }

    if (!config_.schedule.empty()) {
        schedule_.setRules(config_.schedule);
    }

    if (config_.debug) {
        Logger::instance().setMinLevel(LogLevel::Debug);
    }

    Logger::instance().info("Configured: " + config_.toJson());
}

void BearingsCore::start() {
    if (!configured_) {
        Logger::instance().error("Cannot start: not configured");
        return;
    }

    if (tracking_) {
        Logger::instance().info("Already tracking");
        return;
    }

    tracking_ = true;
    config_.enabled = true;
    motionState_ = MotionState::MOVING;
    bridge_->startLocationUpdates(config_.desiredAccuracy, config_.distanceFilter);
    bridge_->startMotionActivity();
    Logger::instance().info("Tracking started");
}

void BearingsCore::stop() {
    if (!tracking_) return;

    // Clean up timer
    if (stopTimerRunning_) {
        bridge_->cancelStopTimer();
        stopTimerRunning_ = false;
    }

    // Clean up motion activity monitoring
    bridge_->stopMotionActivity();

    // Clean up geofence if stationary
    if (motionState_ == MotionState::STATIONARY) {
        bridge_->stopStationaryGeofence();
    }

    tracking_ = false;
    config_.enabled = false;
    schedulerTracking_ = false;
    motionState_ = MotionState::UNKNOWN;
    currentActivity_ = ActivityType::UNKNOWN;
    currentActivityConfidence_ = 0;
    hasLastLocation_ = false;
    hasLastDispatchedLocation_ = false;
    bridge_->stopLocationUpdates();
    Logger::instance().info("Tracking stopped");
}

std::string BearingsCore::getState() {
    json j = {
        {"enabled", tracking_},
        {"tracking", tracking_},
        {"isMoving", motionState_ == MotionState::MOVING},
        {"schedulerRunning", schedulerRunning_},
        {"activity", {
            {"type", activityTypeToString(currentActivity_)},
            {"confidence", currentActivityConfidence_},
        }},
        {"config", json::parse(config_.toJson())},
    };
    return j.dump();
}

std::string BearingsCore::getLocations() {
    auto locations = store_.getAll();
    return Location::toJsonArray(locations);
}

int BearingsCore::getCount() {
    return store_.getCount();
}

bool BearingsCore::destroyLocations() {
    return store_.destroyAll();
}

bool BearingsCore::addGeofence(const std::string& json) {
    if (!configured_) {
        Logger::instance().error("Cannot add geofence: not configured");
        return false;
    }

    Geofence g = Geofence::fromJson(json);

    if (g.identifier.empty()) {
        Logger::instance().error("Cannot add geofence: identifier is empty");
        return false;
    }

    // Reject reserved prefix
    if (g.identifier.rfind("__bearings_", 0) == 0) {
        Logger::instance().error("Cannot add geofence: identifier prefix '__bearings_' is reserved");
        return false;
    }

    int count = geofenceStore_.getCount();
    if (count >= 19) {
        Logger::instance().warning("Geofence count is " + std::to_string(count + 1) +
            " — iOS supports a maximum of 20 monitored regions (one is used by stationary geofence)");
    }

    if (!geofenceStore_.insert(g)) {
        Logger::instance().error("Failed to persist geofence: " + g.identifier);
        return false;
    }

    bridge_->addGeofence(g.identifier, g.latitude, g.longitude,
        g.radius, g.notifyOnEntry, g.notifyOnExit,
        g.notifyOnDwell, g.loiteringDelay);

    Logger::instance().info("Added geofence: " + g.identifier);
    return true;
}

bool BearingsCore::removeGeofence(const std::string& identifier) {
    if (!configured_) return false;

    bridge_->removeGeofence(identifier);
    geofenceStore_.remove(identifier);
    Logger::instance().info("Removed geofence: " + identifier);
    return true;
}

bool BearingsCore::removeAllGeofences() {
    if (!configured_) return false;

    bridge_->removeAllGeofences();
    geofenceStore_.removeAll();
    Logger::instance().info("Removed all geofences");
    return true;
}

std::string BearingsCore::getGeofences() {
    if (!configured_) return "[]";
    return Geofence::toJsonArray(geofenceStore_.getAll());
}

void BearingsCore::onGeofenceEvent(const std::string& identifier, const std::string& action) {
    Geofence g = geofenceStore_.get(identifier);
    if (g.identifier.empty()) {
        Logger::instance().warning("Geofence event for unknown identifier: " + identifier);
        return;
    }

    json j = {
        {"identifier", identifier},
        {"action", action},
        {"extras", g.extras},
    };
    if (hasLastLocation_) {
        j["location"] = json::parse(lastLocation_.toJson());
    }

    bridge_->dispatchEvent("geofence", j.dump());
    Logger::instance().info("Geofence event: " + action + " for " + identifier);
}

void BearingsCore::onLocationReceived(const Location& location) {
    if (!tracking_) return;

    // Override isMoving from state machine, not raw speed
    Location loc = location;
    loc.isMoving = (motionState_ == MotionState::MOVING);
    loc.activityType = activityTypeToString(currentActivity_);
    loc.activityConfidence = currentActivityConfidence_;

    // Always update the cached position (used by geofence events, motion change, etc.)
    lastLocation_ = loc;

    // Distance filter: only store/dispatch if this is the first location
    // or we've moved at least distanceFilter meters from the last dispatched location.
    // Motion change and geofence events are dispatched independently of this filter.
    if (hasLastDispatchedLocation_) {
        double dist = lastDispatchedLocation_.distanceTo(loc);
        if (dist < config_.distanceFilter) {
            Logger::instance().debug("Location filtered: " +
                std::to_string(dist) + "m < distanceFilter " +
                std::to_string(config_.distanceFilter) + "m");
            hasLastLocation_ = true;
            return;
        }
    }

    hasLastLocation_ = true;
    lastDispatchedLocation_ = loc;
    hasLastDispatchedLocation_ = true;

    store_.insert(loc);
    bridge_->dispatchEvent("location", loc.toJson());

    Logger::instance().debug("Location dispatched: " +
        std::to_string(loc.latitude) + ", " +
        std::to_string(loc.longitude));
}

void BearingsCore::onMotionDetected(int activityTypeInt, int confidence) {
    if (!tracking_) return;

    ActivityType activity = activityTypeFromInt(activityTypeInt);
    bool moving = isMovingForActivity(activity);

    // Dispatch activity change event if type changed
    if (activity != currentActivity_) {
        currentActivity_ = activity;
        currentActivityConfidence_ = confidence;
        dispatchActivityChange();
    } else {
        currentActivityConfidence_ = confidence;
    }

    if (moving) {
        // Movement detected — cancel stop timer if running
        if (stopTimerRunning_) {
            bridge_->cancelStopTimer();
            stopTimerRunning_ = false;
            Logger::instance().debug("Stop timer cancelled — motion detected");
        }
        // If stationary, transition to moving
        if (motionState_ == MotionState::STATIONARY) {
            transitionToMoving();
        }
    } else {
        // "Still" detected — start stop timer if moving and no timer running
        if (motionState_ == MotionState::MOVING && !stopTimerRunning_) {
            int seconds = static_cast<int>(config_.stopTimeout * 60);
            bridge_->startStopTimer(seconds);
            stopTimerRunning_ = true;
            Logger::instance().debug("Stop timer started: " + std::to_string(seconds) + "s");
        }
    }
}

void BearingsCore::onStopTimerFired() {
    if (!tracking_) return;
    stopTimerRunning_ = false;

    if (motionState_ == MotionState::MOVING) {
        transitionToStationary();
    }
}

void BearingsCore::onGeofenceExit() {
    if (!tracking_) return;

    if (motionState_ == MotionState::STATIONARY) {
        transitionToMoving();
    }
}

void BearingsCore::transitionToMoving() {
    Logger::instance().info("STATIONARY -> MOVING");
    motionState_ = MotionState::MOVING;
    bridge_->stopStationaryGeofence();
    bridge_->startLocationUpdates(config_.desiredAccuracy, config_.distanceFilter);
    dispatchMotionChange();
}

void BearingsCore::transitionToStationary() {
    Logger::instance().info("MOVING -> STATIONARY");
    motionState_ = MotionState::STATIONARY;
    bridge_->stopLocationUpdates();
    dispatchMotionChange();

    // Start geofence at last known location
    if (hasLastLocation_) {
        bridge_->startStationaryGeofence(
            lastLocation_.latitude,
            lastLocation_.longitude,
            config_.stationaryRadius);
    }
}

void BearingsCore::dispatchMotionChange() {
    if (!hasLastLocation_) return;

    Location loc = lastLocation_;
    loc.event = "motionchange";
    loc.isMoving = (motionState_ == MotionState::MOVING);
    loc.activityType = activityTypeToString(currentActivity_);
    loc.activityConfidence = currentActivityConfidence_;

    store_.insert(loc);
    bridge_->dispatchEvent("motionchange", loc.toJson());
    bridge_->dispatchEvent("location", loc.toJson());

    // Update dispatched baseline so subsequent locations are filtered from here
    lastDispatchedLocation_ = loc;
    hasLastDispatchedLocation_ = true;
}

void BearingsCore::dispatchActivityChange() {
    std::string typeStr = activityTypeToString(currentActivity_);
    json j = {
        {"activity", {
            {"type", typeStr},
            {"confidence", currentActivityConfidence_},
        }},
    };
    bridge_->dispatchEvent("activitychange", j.dump());
    Logger::instance().info("Activity change: " + typeStr + " (" + std::to_string(currentActivityConfidence_) + "%)");
}

// --- Schedule ---

void BearingsCore::startSchedule() {
    if (!configured_) {
        Logger::instance().error("Cannot start schedule: not configured");
        return;
    }

    if (!schedule_.hasRules()) {
        Logger::instance().warning("Cannot start schedule: no rules configured");
        return;
    }

    schedulerRunning_ = true;
    Logger::instance().info("Schedule started");
    // Immediate evaluation
    bridge_->startScheduleTimer(0);
}

void BearingsCore::stopSchedule() {
    bridge_->cancelScheduleTimer();

    if (schedulerTracking_) {
        stop();
        schedulerTracking_ = false;
    }

    schedulerRunning_ = false;
    Logger::instance().info("Schedule stopped");
}

void BearingsCore::onScheduleTimerFired(int year, int month, int day, int dayOfWeek,
                                         int hour, int minute, int second) {
    if (!schedulerRunning_) return;

    EvalResult result = schedule_.evaluate(year, month, day, dayOfWeek, hour, minute, second);

    if (result.shouldTrack && !schedulerTracking_) {
        Logger::instance().info("Schedule: starting tracking");
        start();
        schedulerTracking_ = true;
        bridge_->dispatchEvent("schedule", "{\"enabled\":true}");
    } else if (!result.shouldTrack && schedulerTracking_) {
        Logger::instance().info("Schedule: stopping tracking");
        stop();
        schedulerTracking_ = false;
        bridge_->dispatchEvent("schedule", "{\"enabled\":false}");
    }

    // Schedule next evaluation
    int nextDelay = result.secondsUntilNextTransition;
    if (nextDelay <= 0) {
        nextDelay = 900; // 15 minute fallback
    }

    Logger::instance().debug("Schedule: next evaluation in " + std::to_string(nextDelay) + "s");
    bridge_->startScheduleTimer(nextDelay);
}

} // namespace bearings
