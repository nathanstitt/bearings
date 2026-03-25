#import "GeomonyPlatformBridge.h"
#import "Geomony.h"
#import "GeomonyLocationDelegate.h"

GeomonyPlatformBridgeImpl::GeomonyPlatformBridgeImpl(Geomony* module)
    : module_(module) {
}

void GeomonyPlatformBridgeImpl::startLocationUpdates(double desiredAccuracy, double distanceFilter) {
    Geomony* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate startWithDesiredAccuracy:desiredAccuracy distanceFilter:distanceFilter];
    });
}

void GeomonyPlatformBridgeImpl::stopLocationUpdates() {
    Geomony* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate stop];
    });
}

void GeomonyPlatformBridgeImpl::dispatchEvent(const std::string& name, const std::string& json) {
    Geomony* mod = module_;
    if (!mod) return;

    NSString* eventName = [NSString stringWithUTF8String:name.c_str()];
    NSString* eventBody = [NSString stringWithUTF8String:json.c_str()];

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod sendEventWithName:eventName body:eventBody];
    });
}

std::string GeomonyPlatformBridgeImpl::getDatabasePath() {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* documentsDir = [paths firstObject];
    NSString* dbPath = [documentsDir stringByAppendingPathComponent:@"geomony.db"];
    return [dbPath UTF8String];
}

void GeomonyPlatformBridgeImpl::startMotionActivity() {
    Geomony* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate startMotionActivity];
    });
}

void GeomonyPlatformBridgeImpl::stopMotionActivity() {
    Geomony* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate stopMotionActivity];
    });
}

void GeomonyPlatformBridgeImpl::startStationaryGeofence(double lat, double lon, double radius) {
    Geomony* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate startStationaryGeofenceAtLatitude:lat longitude:lon radius:radius];
    });
}

void GeomonyPlatformBridgeImpl::stopStationaryGeofence() {
    Geomony* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate stopStationaryGeofence];
    });
}

void GeomonyPlatformBridgeImpl::startStopTimer(int seconds) {
    Geomony* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate startStopTimerWithSeconds:seconds];
    });
}

void GeomonyPlatformBridgeImpl::cancelStopTimer() {
    Geomony* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate cancelStopTimer];
    });
}

void GeomonyPlatformBridgeImpl::addGeofence(const std::string& identifier, double lat, double lon,
    double radius, bool notifyOnEntry, bool notifyOnExit,
    bool notifyOnDwell, int loiteringDelay) {
    Geomony* mod = module_;
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

void GeomonyPlatformBridgeImpl::removeGeofence(const std::string& identifier) {
    Geomony* mod = module_;
    if (!mod) return;

    NSString* nsId = [NSString stringWithUTF8String:identifier.c_str()];
    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate removeGeofenceWithIdentifier:nsId];
    });
}

void GeomonyPlatformBridgeImpl::removeAllGeofences() {
    Geomony* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate removeAllGeofences];
    });
}

void GeomonyPlatformBridgeImpl::startScheduleTimer(int delaySeconds) {
    Geomony* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate startScheduleTimerWithSeconds:delaySeconds];
    });
}

void GeomonyPlatformBridgeImpl::cancelScheduleTimer() {
    Geomony* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate cancelScheduleTimer];
    });
}

void GeomonyPlatformBridgeImpl::sendHTTPRequest(const std::string& url,
                                                  const std::string& jsonPayload,
                                                  int requestId) {
    Geomony* mod = module_;
    if (!mod) return;

    NSString* nsUrl = [NSString stringWithUTF8String:url.c_str()];
    NSString* nsPayload = [NSString stringWithUTF8String:jsonPayload.c_str()];
    int rid = requestId;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate sendHTTPRequest:nsUrl payload:nsPayload requestId:rid];
    });
}

void GeomonyPlatformBridgeImpl::startSyncRetryTimer(int delaySeconds) {
    Geomony* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate startSyncRetryTimerWithSeconds:delaySeconds];
    });
}

void GeomonyPlatformBridgeImpl::cancelSyncRetryTimer() {
    Geomony* mod = module_;
    if (!mod) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        [mod.locationDelegate cancelSyncRetryTimer];
    });
}
