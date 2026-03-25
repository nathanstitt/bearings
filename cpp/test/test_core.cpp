#include "test.h"
#include "MockPlatformBridge.h"
#include "geomony/GeomonyCore.h"
#include "geomony/Location.h"
#include <nlohmann/json.hpp>
#include <memory>

using json = nlohmann::json;
using namespace geomony;

// Helper: create a location at given lat/lon
static Location makeLoc(double lat, double lon) {
    Location loc;
    loc.latitude = lat;
    loc.longitude = lon;
    loc.timestamp = "2026-01-01T00:00:00Z";
    loc.createdAt = "2026-01-01T00:00:00Z";
    return loc;
}

// Helper: build a configured & started core with a mock bridge
struct TestHarness {
    std::shared_ptr<test::MockPlatformBridge> bridge;
    std::unique_ptr<GeomonyCore> core;

    TestHarness(double distanceFilter = 10.0) {
        bridge = std::make_shared<test::MockPlatformBridge>();
        core = std::make_unique<GeomonyCore>(bridge);
        json config = {{"distanceFilter", distanceFilter}};
        core->configure(config.dump());
        core->start();
        bridge->clearEvents(); // ignore startup events
    }
};

// ---- Location::distanceTo ----

TEST(distanceTo_same_point_is_zero) {
    Location a = makeLoc(40.0, -74.0);
    Location b = makeLoc(40.0, -74.0);
    ASSERT_NEAR(a.distanceTo(b), 0.0, 0.01);
}

TEST(distanceTo_known_distance) {
    // NYC (40.7128, -74.0060) to Statue of Liberty (40.6892, -74.0445)
    // ~4.17 km
    Location a = makeLoc(40.7128, -74.0060);
    Location b = makeLoc(40.6892, -74.0445);
    double d = a.distanceTo(b);
    ASSERT_GT(d, 4000.0);
    ASSERT_LT(d, 4500.0);
}

// ---- Distance filter ----

TEST(first_location_always_dispatched) {
    TestHarness h;
    h.core->onLocationReceived(makeLoc(40.0, -74.0));

    auto locs = h.bridge->eventsNamed("location");
    ASSERT_EQ(locs.size(), 1u);
}

TEST(location_within_distance_filter_is_suppressed) {
    TestHarness h(50.0); // 50m filter

    h.core->onLocationReceived(makeLoc(40.0, -74.0));
    h.bridge->clearEvents();

    // ~11m north — well within 50m filter
    h.core->onLocationReceived(makeLoc(40.0001, -74.0));

    auto locs = h.bridge->eventsNamed("location");
    ASSERT_EQ(locs.size(), 0u);
}

TEST(location_beyond_distance_filter_is_dispatched) {
    TestHarness h(50.0); // 50m filter

    h.core->onLocationReceived(makeLoc(40.0, -74.0));
    h.bridge->clearEvents();

    // ~111m north — beyond 50m filter
    h.core->onLocationReceived(makeLoc(40.001, -74.0));

    auto locs = h.bridge->eventsNamed("location");
    ASSERT_EQ(locs.size(), 1u);
}

TEST(distance_filter_measured_from_last_dispatched_not_last_received) {
    TestHarness h(100.0); // 100m filter

    // First location — dispatched (baseline)
    h.core->onLocationReceived(makeLoc(40.0, -74.0));
    h.bridge->clearEvents();

    // ~55m — filtered
    h.core->onLocationReceived(makeLoc(40.0005, -74.0));
    ASSERT_EQ(h.bridge->eventsNamed("location").size(), 0u);

    // ~55m further from prev but only ~110m from baseline — dispatched
    h.core->onLocationReceived(makeLoc(40.001, -74.0));
    ASSERT_EQ(h.bridge->eventsNamed("location").size(), 1u);
}

TEST(distance_filter_resets_baseline_after_dispatch) {
    TestHarness h(100.0);

    h.core->onLocationReceived(makeLoc(40.0, -74.0));       // dispatched (first)
    h.core->onLocationReceived(makeLoc(40.001, -74.0));      // dispatched (~111m)
    h.bridge->clearEvents();

    // 50m from second dispatch point — should be filtered
    h.core->onLocationReceived(makeLoc(40.00145, -74.0));
    ASSERT_EQ(h.bridge->eventsNamed("location").size(), 0u);

    // 111m from second dispatch point — should pass
    h.core->onLocationReceived(makeLoc(40.002, -74.0));
    ASSERT_EQ(h.bridge->eventsNamed("location").size(), 1u);
}

TEST(locations_not_dispatched_when_not_tracking) {
    auto bridge = std::make_shared<test::MockPlatformBridge>();
    GeomonyCore core(bridge);
    json config = {{"distanceFilter", 10.0}};
    core.configure(config.dump());
    // don't call start()

    core.onLocationReceived(makeLoc(40.0, -74.0));
    ASSERT_EQ(bridge->eventsNamed("location").size(), 0u);
}

TEST(distance_filter_resets_on_stop_start) {
    TestHarness h(100.0);

    h.core->onLocationReceived(makeLoc(40.0, -74.0)); // dispatched
    h.core->stop();
    h.core->start();
    h.bridge->clearEvents();

    // Same point — should dispatch because baseline was reset
    h.core->onLocationReceived(makeLoc(40.0, -74.0));
    ASSERT_EQ(h.bridge->eventsNamed("location").size(), 1u);
}

// ---- Motion change always dispatches ----

TEST(motion_change_dispatches_location_regardless_of_distance) {
    TestHarness h(1000.0); // very large filter

    h.core->onLocationReceived(makeLoc(40.0, -74.0));
    h.bridge->clearEvents();

    // Tiny movement — would normally be filtered
    h.core->onLocationReceived(makeLoc(40.00001, -74.0));
    ASSERT_EQ(h.bridge->eventsNamed("location").size(), 0u);

    // Trigger motion: stationary detected -> stop timer -> transition
    h.core->onMotionDetected(1, 100); // STATIONARY
    h.core->onStopTimerFired();        // MOVING -> STATIONARY

    auto motionEvents = h.bridge->eventsNamed("motionchange");
    ASSERT_EQ(motionEvents.size(), 1u);
    // motionchange also emits a location event
    auto locEvents = h.bridge->eventsNamed("location");
    ASSERT_EQ(locEvents.size(), 1u);
}

TEST(motion_change_resets_distance_baseline) {
    TestHarness h(100.0);

    h.core->onLocationReceived(makeLoc(40.0, -74.0));

    // Transition to stationary (dispatches motionchange + location at current pos)
    h.core->onMotionDetected(1, 100);
    h.core->onStopTimerFired();
    h.bridge->clearEvents();

    // Transition back to moving (geofence exit)
    h.core->onGeofenceExit();
    h.bridge->clearEvents();

    // Feed location 50m from the motion-change location — should be filtered
    // because motionchange reset the baseline
    h.core->onLocationReceived(makeLoc(40.00045, -74.0));
    ASSERT_EQ(h.bridge->eventsNamed("location").size(), 0u);

    // Feed location 111m from the motion-change location — should pass
    h.core->onLocationReceived(makeLoc(40.001, -74.0));
    ASSERT_EQ(h.bridge->eventsNamed("location").size(), 1u);
}

// ---- Location store respects filter ----

TEST(filtered_locations_are_not_stored) {
    TestHarness h(100.0);

    h.core->onLocationReceived(makeLoc(40.0, -74.0));    // stored
    h.core->onLocationReceived(makeLoc(40.00001, -74.0)); // filtered — not stored
    h.core->onLocationReceived(makeLoc(40.00002, -74.0)); // filtered — not stored

    ASSERT_EQ(h.core->getCount(), 1);
}

TEST(dispatched_locations_are_stored) {
    TestHarness h(10.0);

    h.core->onLocationReceived(makeLoc(40.0, -74.0));     // stored
    h.core->onLocationReceived(makeLoc(40.001, -74.0));    // ~111m, stored
    h.core->onLocationReceived(makeLoc(40.002, -74.0));    // ~111m more, stored

    ASSERT_EQ(h.core->getCount(), 3);
}

// ---- State machine ----

TEST(start_sets_moving_state) {
    auto bridge = std::make_shared<test::MockPlatformBridge>();
    GeomonyCore core(bridge);
    core.configure(R"({"distanceFilter":10})");
    core.start();

    json state = json::parse(core.getState());
    ASSERT_TRUE(state["isMoving"].get<bool>());
    ASSERT_TRUE(state["tracking"].get<bool>());
}

TEST(stop_clears_state) {
    TestHarness h;
    h.core->stop();

    json state = json::parse(h.core->getState());
    ASSERT_FALSE(state["isMoving"].get<bool>());
    ASSERT_FALSE(state["tracking"].get<bool>());
}

TEST(stationary_transition_stops_location_updates) {
    TestHarness h;
    h.core->onLocationReceived(makeLoc(40.0, -74.0));

    h.core->onMotionDetected(1, 100); // STATIONARY activity
    h.core->onStopTimerFired();

    ASSERT_FALSE(h.bridge->locationUpdatesRunning);
    ASSERT_TRUE(h.bridge->stationaryGeofenceActive);
}

TEST(geofence_exit_transitions_to_moving) {
    TestHarness h;
    h.core->onLocationReceived(makeLoc(40.0, -74.0));
    h.core->onMotionDetected(1, 100);
    h.core->onStopTimerFired(); // now STATIONARY

    h.core->onGeofenceExit();

    ASSERT_TRUE(h.bridge->locationUpdatesRunning);
    ASSERT_FALSE(h.bridge->stationaryGeofenceActive);
    json state = json::parse(h.core->getState());
    ASSERT_TRUE(state["isMoving"].get<bool>());
}

// ---- Sync tests ----

// Helper: build a sync-configured core
struct SyncTestHarness {
    std::shared_ptr<test::MockPlatformBridge> bridge;
    std::unique_ptr<GeomonyCore> core;

    SyncTestHarness(int syncThreshold = 5, int maxBatchSize = 100) {
        bridge = std::make_shared<test::MockPlatformBridge>();
        core = std::make_unique<GeomonyCore>(bridge);
        json config = {
            {"distanceFilter", 0},
            {"url", "https://example.com/locations"},
            {"syncThreshold", syncThreshold},
            {"maxBatchSize", maxBatchSize},
            {"syncRetryBaseSeconds", 10},
        };
        core->configure(config.dump());
        core->start();
        bridge->clearEvents();
    }

    void insertLocations(int count) {
        for (int i = 0; i < count; i++) {
            Location loc = makeLoc(40.0 + i * 0.001, -74.0);
            core->onLocationReceived(loc);
        }
    }

    void addTestGeofence(const std::string& identifier) {
        json g = {
            {"identifier", identifier},
            {"latitude", 40.0},
            {"longitude", -74.0},
            {"radius", 200.0},
            {"notifyOnEntry", true},
            {"notifyOnExit", true},
        };
        core->addGeofence(g.dump());
    }
};

TEST(sync_disabled_when_no_url) {
    auto bridge = std::make_shared<test::MockPlatformBridge>();
    GeomonyCore core(bridge);
    json config = {{"distanceFilter", 0}, {"syncThreshold", 1}};
    core.configure(config.dump());
    core.start();

    core.onLocationReceived(makeLoc(40.0, -74.0));

    ASSERT_FALSE(bridge->sendWasCalled);
}

TEST(sync_not_triggered_below_threshold) {
    SyncTestHarness h(5);

    h.insertLocations(4);

    ASSERT_FALSE(h.bridge->sendWasCalled);
}

TEST(sync_triggered_at_threshold) {
    SyncTestHarness h(5);

    h.insertLocations(5);

    ASSERT_TRUE(h.bridge->sendWasCalled);
    // Verify payload contains 5 locations
    json payload = json::parse(h.bridge->lastSendPayload);
    ASSERT_EQ(payload["location"].size(), 5u);
    ASSERT_EQ(h.bridge->lastSendUrl, "https://example.com/locations");
}

TEST(sync_success_marks_synced) {
    SyncTestHarness h(5);

    h.insertLocations(5);
    ASSERT_TRUE(h.bridge->sendWasCalled);

    h.core->onSyncComplete(h.bridge->lastSendRequestId, true);

    // All should be synced now — getUnsyncedCount via getState
    json state = json::parse(h.core->getState());
    ASSERT_EQ(state["sync"]["unsyncedCount"].get<int>(), 0);
}

TEST(sync_success_flushes_remaining) {
    SyncTestHarness h(3, 3); // threshold=3, maxBatch=3

    h.insertLocations(7); // triggers sync at 3, then at 6
    // First sync in flight with 3 locations
    ASSERT_TRUE(h.bridge->sendWasCalled);
    json p1 = json::parse(h.bridge->lastSendPayload);
    ASSERT_EQ(p1["location"].size(), 3u);
    int rid1 = h.bridge->lastSendRequestId;

    // Complete first sync — should trigger second sync (4 remain unsynced)
    h.bridge->sendWasCalled = false;
    h.core->onSyncComplete(rid1, true);
    ASSERT_TRUE(h.bridge->sendWasCalled);
    json p2 = json::parse(h.bridge->lastSendPayload);
    ASSERT_EQ(p2["location"].size(), 3u);
    int rid2 = h.bridge->lastSendRequestId;

    // Complete second sync — should trigger third (1 remains)
    h.bridge->sendWasCalled = false;
    h.core->onSyncComplete(rid2, true);
    ASSERT_TRUE(h.bridge->sendWasCalled);
    json p3 = json::parse(h.bridge->lastSendPayload);
    ASSERT_EQ(p3["location"].size(), 1u);
    int rid3 = h.bridge->lastSendRequestId;

    // Complete third — all synced
    h.core->onSyncComplete(rid3, true);
    json state = json::parse(h.core->getState());
    ASSERT_EQ(state["sync"]["unsyncedCount"].get<int>(), 0);
}

TEST(sync_failure_schedules_retry) {
    SyncTestHarness h(5);

    h.insertLocations(5);
    ASSERT_TRUE(h.bridge->sendWasCalled);

    h.core->onSyncComplete(h.bridge->lastSendRequestId, false);

    ASSERT_TRUE(h.bridge->syncRetryTimerRunning);
    ASSERT_EQ(h.bridge->syncRetryTimerDelay, 10); // base = 10s
    // Locations still unsynced
    json state = json::parse(h.core->getState());
    ASSERT_EQ(state["sync"]["unsyncedCount"].get<int>(), 5);
}

TEST(sync_backoff_increases_exponentially) {
    SyncTestHarness h(5);
    h.insertLocations(5);

    // Fail 1: 10s
    h.core->onSyncComplete(h.bridge->lastSendRequestId, false);
    ASSERT_EQ(h.bridge->syncRetryTimerDelay, 10);

    // Retry timer fires, sync retries
    h.bridge->sendWasCalled = false;
    h.core->onSyncRetryTimerFired();
    ASSERT_TRUE(h.bridge->sendWasCalled);

    // Fail 2: 20s
    h.core->onSyncComplete(h.bridge->lastSendRequestId, false);
    ASSERT_EQ(h.bridge->syncRetryTimerDelay, 20);

    // Fail 3: 40s
    h.core->onSyncRetryTimerFired();
    h.core->onSyncComplete(h.bridge->lastSendRequestId, false);
    ASSERT_EQ(h.bridge->syncRetryTimerDelay, 40);

    // Fail 4: 80s
    h.core->onSyncRetryTimerFired();
    h.core->onSyncComplete(h.bridge->lastSendRequestId, false);
    ASSERT_EQ(h.bridge->syncRetryTimerDelay, 80);

    // Fail 5: 160s
    h.core->onSyncRetryTimerFired();
    h.core->onSyncComplete(h.bridge->lastSendRequestId, false);
    ASSERT_EQ(h.bridge->syncRetryTimerDelay, 160);

    // Fail 6: capped at 300s
    h.core->onSyncRetryTimerFired();
    h.core->onSyncComplete(h.bridge->lastSendRequestId, false);
    ASSERT_EQ(h.bridge->syncRetryTimerDelay, 300);
}

TEST(sync_failure_blocks_further_syncs) {
    SyncTestHarness h(3);

    h.insertLocations(3); // triggers sync
    ASSERT_TRUE(h.bridge->sendWasCalled);

    // Fail
    h.core->onSyncComplete(h.bridge->lastSendRequestId, false);
    h.bridge->sendWasCalled = false;

    // Insert more — should NOT trigger sync (connected_ = false)
    h.insertLocations(3);
    ASSERT_FALSE(h.bridge->sendWasCalled);
}

TEST(connectivity_restored_flushes) {
    SyncTestHarness h(3);

    // Go offline
    h.core->onConnectivityChange(false);

    // Insert locations while offline
    h.insertLocations(5);
    ASSERT_FALSE(h.bridge->sendWasCalled);

    // Come back online
    h.core->onConnectivityChange(true);
    ASSERT_TRUE(h.bridge->sendWasCalled);
    json payload = json::parse(h.bridge->lastSendPayload);
    ASSERT_EQ(payload["location"].size(), 5u);
}

TEST(connectivity_restored_resets_backoff) {
    SyncTestHarness h(5);
    h.insertLocations(5);

    // Fail 1 → 10s
    h.core->onSyncComplete(h.bridge->lastSendRequestId, false);
    ASSERT_EQ(h.bridge->syncRetryTimerDelay, 10);

    // Fail 2 → 20s
    h.core->onSyncRetryTimerFired();
    h.core->onSyncComplete(h.bridge->lastSendRequestId, false);
    ASSERT_EQ(h.bridge->syncRetryTimerDelay, 20);

    // Connectivity restored → resets backoff
    h.core->onConnectivityChange(true);
    // Sync fires, then fails again
    h.core->onSyncComplete(h.bridge->lastSendRequestId, false);
    // Should be back to base 10s, not 40s
    ASSERT_EQ(h.bridge->syncRetryTimerDelay, 10);
}

TEST(geofence_event_triggers_immediate_sync) {
    SyncTestHarness h(100); // Very high threshold

    h.insertLocations(1); // way below threshold
    ASSERT_FALSE(h.bridge->sendWasCalled);

    // Add a geofence and trigger an event
    h.addTestGeofence("office");
    h.core->onGeofenceEvent("office", "ENTER");

    // Sync should fire despite being below threshold
    ASSERT_TRUE(h.bridge->sendWasCalled);
}

TEST(no_concurrent_syncs) {
    SyncTestHarness h(3);

    h.insertLocations(3); // triggers sync
    ASSERT_TRUE(h.bridge->sendWasCalled);
    ASSERT_EQ(h.bridge->sendCallCount, 1);

    // Insert more while sync is in flight
    h.insertLocations(3);
    // Should NOT fire another sendHTTPRequest
    ASSERT_EQ(h.bridge->sendCallCount, 1);

    // Complete the first sync
    h.core->onSyncComplete(h.bridge->lastSendRequestId, true);
    // Now the second batch should sync
    ASSERT_EQ(h.bridge->sendCallCount, 2);
}

TEST(stale_callback_ignored) {
    SyncTestHarness h(5);

    h.insertLocations(5);
    int staleRequestId = h.bridge->lastSendRequestId;

    // Stop (resets sync state)
    h.core->stop();

    // Stale callback arrives — should not crash or mark anything
    h.core->onSyncComplete(staleRequestId, true);

    // Restart and verify locations are still unsynced
    h.core->start();
    json state = json::parse(h.core->getState());
    ASSERT_EQ(state["sync"]["unsyncedCount"].get<int>(), 5);
}

TEST(stop_cancels_sync_retry_timer) {
    SyncTestHarness h(5);

    h.insertLocations(5);
    h.core->onSyncComplete(h.bridge->lastSendRequestId, false);
    ASSERT_TRUE(h.bridge->syncRetryTimerRunning);

    h.core->stop();
    ASSERT_FALSE(h.bridge->syncRetryTimerRunning);
}

// ---- stopOnTerminate / onTerminate ----

TEST(getStopOnTerminate_defaults_to_true) {
    auto bridge = std::make_shared<test::MockPlatformBridge>();
    GeomonyCore core(bridge);
    core.configure(R"({"distanceFilter":10})");

    ASSERT_TRUE(core.getStopOnTerminate());
}

TEST(getStopOnTerminate_respects_config) {
    auto bridge = std::make_shared<test::MockPlatformBridge>();
    GeomonyCore core(bridge);
    core.configure(R"({"stopOnTerminate":false})");

    ASSERT_FALSE(core.getStopOnTerminate());
}

TEST(onTerminate_stops_when_stopOnTerminate_true) {
    TestHarness h;
    h.core->onLocationReceived(makeLoc(40.0, -74.0));

    h.core->onTerminate();

    json state = json::parse(h.core->getState());
    ASSERT_FALSE(state["tracking"].get<bool>());
    ASSERT_FALSE(h.bridge->locationUpdatesRunning);
    ASSERT_FALSE(h.bridge->motionActivityRunning);
}

TEST(onTerminate_leaves_geofence_when_stopOnTerminate_false) {
    auto bridge = std::make_shared<test::MockPlatformBridge>();
    auto core = std::make_unique<GeomonyCore>(bridge);
    core->configure(R"({"stopOnTerminate":false,"distanceFilter":10})");
    core->start();
    core->onLocationReceived(makeLoc(40.0, -74.0));
    bridge->clearEvents();

    core->onTerminate();

    // Location updates and motion activity should be stopped
    ASSERT_FALSE(bridge->locationUpdatesRunning);
    ASSERT_FALSE(bridge->motionActivityRunning);
    // But a stationary geofence should be placed (was MOVING with a last location)
    ASSERT_TRUE(bridge->stationaryGeofenceActive);
}

TEST(onTerminate_preserves_existing_geofence_when_stationary) {
    auto bridge = std::make_shared<test::MockPlatformBridge>();
    auto core = std::make_unique<GeomonyCore>(bridge);
    core->configure(R"({"stopOnTerminate":false,"distanceFilter":10})");
    core->start();
    core->onLocationReceived(makeLoc(40.0, -74.0));

    // Transition to stationary — geofence placed by state machine
    core->onMotionDetected(1, 100); // STATIONARY
    core->onStopTimerFired();
    ASSERT_TRUE(bridge->stationaryGeofenceActive);

    core->onTerminate();

    // Geofence should still be active (not removed by stop())
    ASSERT_TRUE(bridge->stationaryGeofenceActive);
    ASSERT_FALSE(bridge->locationUpdatesRunning);
    ASSERT_FALSE(bridge->motionActivityRunning);
}

TEST(onTerminate_is_idempotent) {
    TestHarness h;
    h.core->onLocationReceived(makeLoc(40.0, -74.0));

    h.core->onTerminate();
    h.core->onTerminate(); // should not crash
}

TEST(destructor_respects_stopOnTerminate_false) {
    auto bridge = std::make_shared<test::MockPlatformBridge>();
    {
        auto core = std::make_unique<GeomonyCore>(bridge);
        core->configure(R"({"stopOnTerminate":false,"distanceFilter":10})");
        core->start();
        core->onLocationReceived(makeLoc(40.0, -74.0));
    } // destructor fires here

    // Geofence should be active after destruction
    ASSERT_TRUE(bridge->stationaryGeofenceActive);
}

TEST(destructor_stops_when_stopOnTerminate_true) {
    auto bridge = std::make_shared<test::MockPlatformBridge>();
    {
        auto core = std::make_unique<GeomonyCore>(bridge);
        core->configure(R"({"stopOnTerminate":true,"distanceFilter":10})");
        core->start();
        core->onLocationReceived(makeLoc(40.0, -74.0));
    } // destructor fires here

    // Everything should be cleaned up
    ASSERT_FALSE(bridge->locationUpdatesRunning);
    ASSERT_FALSE(bridge->motionActivityRunning);
    ASSERT_FALSE(bridge->stationaryGeofenceActive);
}

int main() {
    return test::run();
}
