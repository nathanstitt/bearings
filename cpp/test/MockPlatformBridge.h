#pragma once

#include "geomony/PlatformBridge.h"
#include <string>
#include <vector>
#include <utility>
#include <cstdio>
#include <filesystem>

namespace test {

struct DispatchedEvent {
    std::string name;
    std::string json;
};

class MockPlatformBridge : public geomony::PlatformBridge {
public:
    // Dispatched events, inspectable by tests
    std::vector<DispatchedEvent> events;

    // Track platform calls
    bool locationUpdatesRunning = false;
    bool motionActivityRunning = false;
    bool stationaryGeofenceActive = false;
    bool stopTimerRunning = false;
    int stopTimerSeconds = 0;
    double lastDesiredAccuracy = 0;
    double lastDistanceFilter = 0;

    // Use a temp file for the database
    std::string dbPath;

    MockPlatformBridge() {
        auto tmp = std::filesystem::temp_directory_path() / "geomony_test_XXXXXX";
        dbPath = tmp.string() + ".db";
    }

    ~MockPlatformBridge() override {
        std::remove(dbPath.c_str());
    }

    void startLocationUpdates(double desiredAccuracy, double distanceFilter) override {
        locationUpdatesRunning = true;
        lastDesiredAccuracy = desiredAccuracy;
        lastDistanceFilter = distanceFilter;
    }

    void stopLocationUpdates() override {
        locationUpdatesRunning = false;
    }

    void dispatchEvent(const std::string& name, const std::string& json) override {
        events.push_back({name, json});
    }

    std::string getDatabasePath() override {
        return dbPath;
    }

    void startMotionActivity() override { motionActivityRunning = true; }
    void stopMotionActivity() override { motionActivityRunning = false; }

    void startStationaryGeofence(double, double, double) override {
        stationaryGeofenceActive = true;
    }
    void stopStationaryGeofence() override {
        stationaryGeofenceActive = false;
    }

    void startStopTimer(int seconds) override {
        stopTimerRunning = true;
        stopTimerSeconds = seconds;
    }
    void cancelStopTimer() override {
        stopTimerRunning = false;
    }

    void addGeofence(const std::string&, double, double, double, bool, bool, bool, int) override {}
    void removeGeofence(const std::string&) override {}
    void removeAllGeofences() override {}

    void startScheduleTimer(int) override {}
    void cancelScheduleTimer() override {}

    // Sync tracking
    bool syncRetryTimerRunning = false;
    int syncRetryTimerDelay = 0;
    int lastSendRequestId = 0;
    std::string lastSendUrl;
    std::string lastSendPayload;
    std::string lastSendHeaders;
    bool sendWasCalled = false;
    int sendCallCount = 0;

    void sendHTTPRequest(const std::string& url,
                         const std::string& jsonPayload,
                         const std::string& headersJson,
                         int requestId) override {
        sendWasCalled = true;
        sendCallCount++;
        lastSendUrl = url;
        lastSendPayload = jsonPayload;
        lastSendHeaders = headersJson;
        lastSendRequestId = requestId;
    }

    void startSyncRetryTimer(int delaySeconds) override {
        syncRetryTimerRunning = true;
        syncRetryTimerDelay = delaySeconds;
    }

    void cancelSyncRetryTimer() override {
        syncRetryTimerRunning = false;
    }

    // --- Helpers for tests ---

    std::vector<DispatchedEvent> eventsNamed(const std::string& name) const {
        std::vector<DispatchedEvent> result;
        for (auto& e : events) {
            if (e.name == name) result.push_back(e);
        }
        return result;
    }

    void clearEvents() { events.clear(); }
};

} // namespace test
