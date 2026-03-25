#include "test.h"
#include "MockPlatformBridge.h"
#include "bearings/BearingsCore.h"
#include "bearings/Location.h"
#include <nlohmann/json.hpp>
#include <memory>

using json = nlohmann::json;
using namespace bearings;

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
    std::unique_ptr<BearingsCore> core;

    TestHarness(double distanceFilter = 10.0) {
        bridge = std::make_shared<test::MockPlatformBridge>();
        core = std::make_unique<BearingsCore>(bridge);
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
    BearingsCore core(bridge);
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
    BearingsCore core(bridge);
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

int main() {
    return test::run();
}
