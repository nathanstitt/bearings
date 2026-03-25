#import "BearingsPlatformBridge.h"
#import "Bearings.h"
#import "BearingsLocationDelegate.h"

BearingsPlatformBridgeImpl::BearingsPlatformBridgeImpl(Bearings* module)
    : module_(module) {
}

void BearingsPlatformBridgeImpl::startLocationUpdates(double desiredAccuracy, double distanceFilter) {
    Bearings* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate startWithDesiredAccuracy:desiredAccuracy distanceFilter:distanceFilter];
    });
}

void BearingsPlatformBridgeImpl::stopLocationUpdates() {
    Bearings* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate stop];
    });
}

void BearingsPlatformBridgeImpl::dispatchEvent(const std::string& name, const std::string& json) {
    Bearings* mod = module_;
    if (!mod) return;

    NSString* eventName = [NSString stringWithUTF8String:name.c_str()];
    NSString* eventBody = [NSString stringWithUTF8String:json.c_str()];

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod sendEventWithName:eventName body:eventBody];
    });
}

std::string BearingsPlatformBridgeImpl::getDatabasePath() {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* documentsDir = [paths firstObject];
    NSString* dbPath = [documentsDir stringByAppendingPathComponent:@"bearings.db"];
    return [dbPath UTF8String];
}

void BearingsPlatformBridgeImpl::startMotionActivity() {
    Bearings* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate startMotionActivity];
    });
}

void BearingsPlatformBridgeImpl::stopMotionActivity() {
    Bearings* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate stopMotionActivity];
    });
}

void BearingsPlatformBridgeImpl::startStationaryGeofence(double lat, double lon, double radius) {
    Bearings* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate startStationaryGeofenceAtLatitude:lat longitude:lon radius:radius];
    });
}

void BearingsPlatformBridgeImpl::stopStationaryGeofence() {
    Bearings* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate stopStationaryGeofence];
    });
}

void BearingsPlatformBridgeImpl::startStopTimer(int seconds) {
    Bearings* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate startStopTimerWithSeconds:seconds];
    });
}

void BearingsPlatformBridgeImpl::cancelStopTimer() {
    Bearings* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate cancelStopTimer];
    });
}

void BearingsPlatformBridgeImpl::addGeofence(const std::string& identifier, double lat, double lon,
    double radius, bool notifyOnEntry, bool notifyOnExit,
    bool notifyOnDwell, int loiteringDelay) {
    Bearings* mod = module_;
    if (!mod) return;

    NSString* nsId = [NSString stringWithUTF8String:identifier.c_str()];
    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate addGeofenceWithIdentifier:nsId
                                               latitude:lat
                                              longitude:lon
                                                 radius:radius
                                         notifyOnEntry:notifyOnEntry
                                          notifyOnExit:notifyOnExit
                                         notifyOnDwell:notifyOnDwell
                                        loiteringDelay:loiteringDelay];
    });
}

void BearingsPlatformBridgeImpl::removeGeofence(const std::string& identifier) {
    Bearings* mod = module_;
    if (!mod) return;

    NSString* nsId = [NSString stringWithUTF8String:identifier.c_str()];
    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate removeGeofenceWithIdentifier:nsId];
    });
}

void BearingsPlatformBridgeImpl::removeAllGeofences() {
    Bearings* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate removeAllGeofences];
    });
}

void BearingsPlatformBridgeImpl::startScheduleTimer(int delaySeconds) {
    Bearings* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate startScheduleTimerWithSeconds:delaySeconds];
    });
}

void BearingsPlatformBridgeImpl::cancelScheduleTimer() {
    Bearings* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate cancelScheduleTimer];
    });
}
