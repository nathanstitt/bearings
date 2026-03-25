#import "GeotrackPlatformBridge.h"
#import "Geotrack.h"
#import "GeotrackLocationDelegate.h"

GeotrackPlatformBridgeImpl::GeotrackPlatformBridgeImpl(Geotrack* module)
    : module_(module) {
}

void GeotrackPlatformBridgeImpl::startLocationUpdates(double desiredAccuracy, double distanceFilter) {
    Geotrack* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate startWithDesiredAccuracy:desiredAccuracy distanceFilter:distanceFilter];
    });
}

void GeotrackPlatformBridgeImpl::stopLocationUpdates() {
    Geotrack* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate stop];
    });
}

void GeotrackPlatformBridgeImpl::dispatchEvent(const std::string& name, const std::string& json) {
    Geotrack* mod = module_;
    if (!mod) return;

    NSString* eventName = [NSString stringWithUTF8String:name.c_str()];
    NSString* eventBody = [NSString stringWithUTF8String:json.c_str()];

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod sendEventWithName:eventName body:eventBody];
    });
}

std::string GeotrackPlatformBridgeImpl::getDatabasePath() {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* documentsDir = [paths firstObject];
    NSString* dbPath = [documentsDir stringByAppendingPathComponent:@"geotrack.db"];
    return [dbPath UTF8String];
}

void GeotrackPlatformBridgeImpl::startMotionActivity() {
    Geotrack* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate startMotionActivity];
    });
}

void GeotrackPlatformBridgeImpl::stopMotionActivity() {
    Geotrack* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate stopMotionActivity];
    });
}

void GeotrackPlatformBridgeImpl::startStationaryGeofence(double lat, double lon, double radius) {
    Geotrack* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate startStationaryGeofenceAtLatitude:lat longitude:lon radius:radius];
    });
}

void GeotrackPlatformBridgeImpl::stopStationaryGeofence() {
    Geotrack* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate stopStationaryGeofence];
    });
}

void GeotrackPlatformBridgeImpl::startStopTimer(int seconds) {
    Geotrack* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate startStopTimerWithSeconds:seconds];
    });
}

void GeotrackPlatformBridgeImpl::cancelStopTimer() {
    Geotrack* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate cancelStopTimer];
    });
}

void GeotrackPlatformBridgeImpl::addGeofence(const std::string& identifier, double lat, double lon,
    double radius, bool notifyOnEntry, bool notifyOnExit,
    bool notifyOnDwell, int loiteringDelay) {
    Geotrack* mod = module_;
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

void GeotrackPlatformBridgeImpl::removeGeofence(const std::string& identifier) {
    Geotrack* mod = module_;
    if (!mod) return;

    NSString* nsId = [NSString stringWithUTF8String:identifier.c_str()];
    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate removeGeofenceWithIdentifier:nsId];
    });
}

void GeotrackPlatformBridgeImpl::removeAllGeofences() {
    Geotrack* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate removeAllGeofences];
    });
}

void GeotrackPlatformBridgeImpl::startScheduleTimer(int delaySeconds) {
    Geotrack* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate startScheduleTimerWithSeconds:delaySeconds];
    });
}

void GeotrackPlatformBridgeImpl::cancelScheduleTimer() {
    Geotrack* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate cancelScheduleTimer];
    });
}
