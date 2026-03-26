#include "geomony/GeomonyCore.h"
#include "geomony/GeofenceEventRecord.h"
#include "geomony/Logger.h"
#include <nlohmann/json.hpp>
#include <chrono>

using json = nlohmann::json;

namespace geomony {

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

GeomonyCore::GeomonyCore(std::shared_ptr<PlatformBridge> bridge)
    : bridge_(std::move(bridge)) {
}

GeomonyCore::~GeomonyCore() {
    onTerminate();
}

bool GeomonyCore::getStopOnTerminate() const {
    return config_.stopOnTerminate;
}

void GeomonyCore::onTerminate() {
    if (terminated_) return;
    terminated_ = true;

    if (config_.stopOnTerminate) {
        if (schedulerRunning_) stopSchedule();
        if (tracking_) stop();
    } else if (tracking_) {
        // Cancel timers that won't survive process death
        if (stopTimerRunning_) {
            bridge_->cancelStopTimer();
            stopTimerRunning_ = false;
        }
        if (syncRetryTimerRunning_) {
            bridge_->cancelSyncRetryTimer();
            syncRetryTimerRunning_ = false;
        }
        bridge_->stopMotionActivity();
        bridge_->stopLocationUpdates();

        // Ensure a stationary geofence is active for iOS relaunch
        if (motionState_ == MotionState::MOVING && hasLastLocation_) {
            bridge_->startStationaryGeofence(
                lastLocation_.latitude, lastLocation_.longitude,
                config_.stationaryRadius);
        }
        // If already STATIONARY, geofence is already active — leave it.
        // Do NOT call stop() — that would remove the geofence.
    }
}

void GeomonyCore::configure(const std::string& configJson) {
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
        if (!geofenceEventStore_.open(dbPath)) {
            Logger::instance().error("Failed to open geofence event database");
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

void GeomonyCore::start() {
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

void GeomonyCore::stop() {
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

    // Clean up sync state
    if (syncRetryTimerRunning_) {
        bridge_->cancelSyncRetryTimer();
        syncRetryTimerRunning_ = false;
    }
    syncInFlight_ = false;
    activeRequestId_ = 0;
    pendingSyncIds_.clear();
    pendingGeofenceEventSyncIds_.clear();
    syncRetryCount_ = 0;
    connected_ = true;

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

std::string GeomonyCore::getState() {
    json j = {
        {"enabled", tracking_},
        {"tracking", tracking_},
        {"isMoving", motionState_ == MotionState::MOVING},
        {"schedulerRunning", schedulerRunning_},
        {"activity", {
            {"type", activityTypeToString(currentActivity_)},
            {"confidence", currentActivityConfidence_},
        }},
        {"sync", {
            {"enabled", isSyncEnabled()},
            {"connected", connected_},
            {"syncInFlight", syncInFlight_},
            {"unsyncedCount", configured_ ? store_.getUnsyncedCount() : 0},
            {"geofenceEventUnsyncedCount", configured_ ? geofenceEventStore_.getUnsyncedCount() : 0},
        }},
        {"config", json::parse(config_.toJson())},
    };
    return j.dump();
}

std::string GeomonyCore::getLocations() {
    auto locations = store_.getAll();
    return Location::toJsonArray(locations);
}

int GeomonyCore::getCount() {
    return store_.getCount();
}

bool GeomonyCore::destroyLocations() {
    return store_.destroyAll();
}

bool GeomonyCore::addGeofence(const std::string& json) {
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
    if (g.identifier.rfind("__geomony_", 0) == 0) {
        Logger::instance().error("Cannot add geofence: identifier prefix '__geomony_' is reserved");
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

bool GeomonyCore::removeGeofence(const std::string& identifier) {
    if (!configured_) return false;

    bridge_->removeGeofence(identifier);
    geofenceStore_.remove(identifier);
    Logger::instance().info("Removed geofence: " + identifier);
    return true;
}

bool GeomonyCore::removeAllGeofences() {
    if (!configured_) return false;

    bridge_->removeAllGeofences();
    geofenceStore_.removeAll();
    Logger::instance().info("Removed all geofences");
    return true;
}

std::string GeomonyCore::getGeofences() {
    if (!configured_) return "[]";
    return Geofence::toJsonArray(geofenceStore_.getAll());
}

void GeomonyCore::onGeofenceEvent(const std::string& identifier, const std::string& action) {
    Geofence g = geofenceStore_.get(identifier);
    if (g.identifier.empty()) {
        Logger::instance().warning("Geofence event for unknown identifier: " + identifier);
        return;
    }

    json j = {
        {"identifier", identifier},
        {"action", action},
    };

    // Emit extras as JSON object if parseable, otherwise as string
    if (!g.extras.empty()) {
        auto parsed = json::parse(g.extras, nullptr, false);
        if (!parsed.is_discarded()) {
            j["extras"] = parsed;
        } else {
            j["extras"] = g.extras;
        }
    }

    if (hasLastLocation_) {
        j["location"] = json::parse(lastLocation_.toJson());
    }

    bridge_->dispatchEvent("geofence", j.dump());
    Logger::instance().info("Geofence event: " + action + " for " + identifier);

    // Persist geofence event for sync
    if (configured_) {
        GeofenceEventRecord record;
        record.identifier = identifier;
        record.action = action;
        record.extras = g.extras;
        if (hasLastLocation_) {
            record.latitude = lastLocation_.latitude;
            record.longitude = lastLocation_.longitude;
            record.accuracy = lastLocation_.accuracy;
            record.timestamp = lastLocation_.timestamp;
        } else {
            record.timestamp = "";
        }
        // Generate createdAt
        auto now = std::chrono::system_clock::now();
        auto time_t_val = std::chrono::system_clock::to_time_t(now);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", std::gmtime(&time_t_val));
        record.createdAt = std::string(buf) + "Z";
        record.timestamp = record.createdAt;

        geofenceEventStore_.insert(record);
    }

    maybeSync();
}

void GeomonyCore::onLocationReceived(const Location& location) {
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

    int totalUnsynced = store_.getUnsyncedCount() + geofenceEventStore_.getUnsyncedCount();
    if (isSyncEnabled() && totalUnsynced >= config_.syncThreshold) {
        maybeSync();
    }
}

void GeomonyCore::onMotionDetected(int activityTypeInt, int confidence) {
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

void GeomonyCore::onStopTimerFired() {
    if (!tracking_) return;
    stopTimerRunning_ = false;

    if (motionState_ == MotionState::MOVING) {
        transitionToStationary();
    }
}

void GeomonyCore::onGeofenceExit() {
    if (!tracking_) return;

    if (motionState_ == MotionState::STATIONARY) {
        transitionToMoving();
    }
}

void GeomonyCore::transitionToMoving() {
    Logger::instance().info("STATIONARY -> MOVING");
    motionState_ = MotionState::MOVING;
    bridge_->stopStationaryGeofence();
    bridge_->startLocationUpdates(config_.desiredAccuracy, config_.distanceFilter);
    dispatchMotionChange();
}

void GeomonyCore::transitionToStationary() {
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

void GeomonyCore::dispatchMotionChange() {
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

void GeomonyCore::dispatchActivityChange() {
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

// --- Geofence Event Store API ---

std::string GeomonyCore::getGeofenceEvents() {
    if (!configured_) return "[]";
    return GeofenceEventRecord::toJsonArray(geofenceEventStore_.getAll());
}

int GeomonyCore::getGeofenceEventCount() {
    if (!configured_) return 0;
    return geofenceEventStore_.getCount();
}

bool GeomonyCore::destroyGeofenceEvents() {
    if (!configured_) return false;
    return geofenceEventStore_.destroyAll();
}

// --- Schedule ---

void GeomonyCore::startSchedule() {
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

void GeomonyCore::stopSchedule() {
    bridge_->cancelScheduleTimer();

    if (schedulerTracking_) {
        stop();
        schedulerTracking_ = false;
    }

    schedulerRunning_ = false;
    Logger::instance().info("Schedule stopped");
}

void GeomonyCore::onScheduleTimerFired(int year, int month, int day, int dayOfWeek,
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

// --- Sync ---

bool GeomonyCore::isSyncEnabled() const {
    return !config_.url.empty();
}

void GeomonyCore::maybeSync() {
    if (!isSyncEnabled()) return;
    if (!configured_) return;
    if (syncInFlight_) return;
    if (!connected_) return;

    performSync();
}

void GeomonyCore::performSync() {
    auto unsynced = store_.getUnsynced(config_.maxBatchSize);
    if (unsynced.empty()) {
        // Check for unsynced geofence events
        auto unsyncedGeoEvents = geofenceEventStore_.getUnsynced(config_.maxBatchSize);
        if (unsyncedGeoEvents.empty()) return;
    }

    auto unsyncedLocs = store_.getUnsynced(config_.maxBatchSize);
    auto unsyncedGeoEvents = geofenceEventStore_.getUnsynced(config_.maxBatchSize);

    if (unsyncedLocs.empty() && unsyncedGeoEvents.empty()) return;

    pendingSyncIds_.clear();
    for (const auto& loc : unsyncedLocs) {
        pendingSyncIds_.push_back(loc.id);
    }

    pendingGeofenceEventSyncIds_.clear();
    for (const auto& ev : unsyncedGeoEvents) {
        pendingGeofenceEventSyncIds_.push_back(ev.id);
    }

    json payload;
    if (!unsyncedLocs.empty()) {
        payload["location"] = json::parse(Location::toSyncJsonArray(unsyncedLocs));
    }
    if (!unsyncedGeoEvents.empty()) {
        json geoArr = json::array();
        for (const auto& ev : unsyncedGeoEvents) {
            geoArr.push_back(json::parse(ev.toJson()));
        }
        payload["geofence"] = geoArr;
    }

    // Serialize headers to JSON
    json headersJson = json(config_.headers);

    activeRequestId_ = nextRequestId_++;
    syncInFlight_ = true;
    bridge_->sendHTTPRequest(config_.url, payload.dump(), headersJson.dump(), activeRequestId_);
}

void GeomonyCore::onSyncComplete(int requestId, bool success,
                                   int httpStatus, const std::string& responseText) {
    if (requestId != activeRequestId_) return;

    syncInFlight_ = false;

    // Dispatch http event
    json httpEv = {
        {"status", httpStatus},
        {"responseText", responseText},
        {"success", success},
    };
    bridge_->dispatchEvent("http", httpEv.dump());

    if (success) {
        store_.markSynced(pendingSyncIds_);
        geofenceEventStore_.markSynced(pendingGeofenceEventSyncIds_);
        int syncedCount = static_cast<int>(pendingSyncIds_.size()) +
                          static_cast<int>(pendingGeofenceEventSyncIds_.size());
        pendingSyncIds_.clear();
        pendingGeofenceEventSyncIds_.clear();
        syncRetryCount_ = 0;
        awaitingAuthRefresh_ = false;

        json ev = {{"success", true}, {"count", syncedCount}};
        bridge_->dispatchEvent("sync", ev.dump());

        maybeSync();
    } else if (httpStatus == 401 || httpStatus == 403) {
        // Auth failure — ask JS to refresh credentials
        awaitingAuthRefresh_ = true;
        Logger::instance().info("Sync auth failure (" + std::to_string(httpStatus) +
            ") — requesting authorization refresh");
        bridge_->dispatchEvent("authorizationRefresh", httpEv.dump());
    } else {
        connected_ = false;
        syncRetryCount_++;

        int delay = config_.syncRetryBaseSeconds;
        for (int i = 1; i < syncRetryCount_; i++) {
            delay *= 2;
            if (delay >= 300) { delay = 300; break; }
        }

        syncRetryTimerRunning_ = true;
        bridge_->startSyncRetryTimer(delay);

        json ev = {{"success", false}};
        bridge_->dispatchEvent("sync", ev.dump());
    }
}

void GeomonyCore::updateAuthorizationHeaders(const std::string& headersJson) {
    json j = json::parse(headersJson, nullptr, false);
    if (j.is_discarded() || !j.is_object()) return;

    config_.headers.clear();
    for (auto& [key, val] : j.items()) {
        if (val.is_string()) config_.headers[key] = val.get<std::string>();
    }

    Logger::instance().info("Authorization headers updated");

    if (awaitingAuthRefresh_) {
        awaitingAuthRefresh_ = false;
        syncRetryCount_ = 0;
        connected_ = true;
        performSync();
    }
}

void GeomonyCore::onSyncRetryTimerFired() {
    syncRetryTimerRunning_ = false;
    connected_ = true;
    maybeSync();
}

void GeomonyCore::onConnectivityChange(bool connected) {
    connected_ = connected;

    if (connected) {
        syncRetryCount_ = 0;
        if (syncRetryTimerRunning_) {
            bridge_->cancelSyncRetryTimer();
            syncRetryTimerRunning_ = false;
        }
        maybeSync();
    }
}

} // namespace geomony
