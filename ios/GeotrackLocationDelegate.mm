#import "GeotrackLocationDelegate.h"
#include "geotrack/GeotrackCore.h"
#include "geotrack/Location.h"

#include <chrono>
#include <sstream>
#include <iomanip>

@implementation GeotrackLocationDelegate

- (instancetype)init {
    self = [super init];
    if (self) {
        _locationManager = [[CLLocationManager alloc] init];
        _locationManager.delegate = self;
        _motionActivityManager = [[CMMotionActivityManager alloc] init];
        _dwellTimers = [NSMutableDictionary new];
        _geofenceConfigs = [NSMutableDictionary new];
    }
    return self;
}

- (void)startWithDesiredAccuracy:(double)desiredAccuracy distanceFilter:(double)distanceFilter {
    [_locationManager requestAlwaysAuthorization];

    if (desiredAccuracy == -1) {
        _locationManager.desiredAccuracy = kCLLocationAccuracyBest;
    } else if (desiredAccuracy == -2) {
        _locationManager.desiredAccuracy = kCLLocationAccuracyNearestTenMeters;
    } else if (desiredAccuracy == -3) {
        _locationManager.desiredAccuracy = kCLLocationAccuracyHundredMeters;
    } else {
        _locationManager.desiredAccuracy = desiredAccuracy;
    }

    _locationManager.distanceFilter = distanceFilter;
    _locationManager.allowsBackgroundLocationUpdates = YES;
    _locationManager.pausesLocationUpdatesAutomatically = NO;
    _locationManager.showsBackgroundLocationIndicator = YES;

    [_locationManager startUpdatingLocation];
}

- (void)stop {
    [_locationManager stopUpdatingLocation];
}

#pragma mark - Motion Activity

- (void)startMotionActivity {
    if (![CMMotionActivityManager isActivityAvailable]) {
        return;
    }

    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    queue.maxConcurrentOperationCount = 1;

    __weak __typeof(self) weakSelf = self;
    [_motionActivityManager startActivityUpdatesToQueue:queue withHandler:^(CMMotionActivity* _Nullable activity) {
        if (!activity || !weakSelf || !weakSelf.core) return;

        // Ignore unknown/low-confidence
        if (activity.confidence == CMMotionActivityConfidenceLow) return;

        // Map CMMotionActivity to integer activity type code
        // 0=unknown, 1=stationary, 2=walking, 3=running, 4=cycling, 5=automotive
        int activityType = 0;
        if (activity.automotive) activityType = 5;
        else if (activity.cycling) activityType = 4;
        else if (activity.running) activityType = 3;
        else if (activity.walking) activityType = 2;
        else if (activity.stationary) activityType = 1;

        int confidence = 0;
        switch (activity.confidence) {
            case CMMotionActivityConfidenceLow: confidence = 25; break;
            case CMMotionActivityConfidenceMedium: confidence = 50; break;
            case CMMotionActivityConfidenceHigh: confidence = 100; break;
        }

        if (activityType > 0) {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (weakSelf.core) {
                    weakSelf.core->onMotionDetected(activityType, confidence);
                }
            });
        }
    }];
}

- (void)stopMotionActivity {
    [_motionActivityManager stopActivityUpdates];
}

#pragma mark - Stationary Geofence

- (void)startStationaryGeofenceAtLatitude:(double)latitude longitude:(double)longitude radius:(double)radius {
    CLLocationCoordinate2D center = CLLocationCoordinate2DMake(latitude, longitude);
    double clampedRadius = MIN(radius, _locationManager.maximumRegionMonitoringDistance);

    _stationaryRegion = [[CLCircularRegion alloc] initWithCenter:center
                                                          radius:clampedRadius
                                                      identifier:@"__geotrack_stationary"];
    _stationaryRegion.notifyOnEntry = NO;
    _stationaryRegion.notifyOnExit = YES;

    [_locationManager startMonitoringForRegion:_stationaryRegion];
}

- (void)stopStationaryGeofence {
    if (_stationaryRegion) {
        [_locationManager stopMonitoringForRegion:_stationaryRegion];
        _stationaryRegion = nil;
    }
}

- (void)locationManager:(CLLocationManager *)manager didEnterRegion:(CLRegion *)region {
    if ([region.identifier hasPrefix:@"__geotrack_"]) return;
    if (!_core) return;

    _core->onGeofenceEvent([region.identifier UTF8String], "ENTER");

    // Start dwell timer if configured
    NSDictionary* config = _geofenceConfigs[region.identifier];
    if (config && [config[@"notifyOnDwell"] boolValue]) {
        int delay = [config[@"loiteringDelay"] intValue];
        [self startDwellTimerForIdentifier:region.identifier delay:delay];
    }
}

- (void)locationManager:(CLLocationManager *)manager didExitRegion:(CLRegion *)region {
    if ([region.identifier isEqualToString:@"__geotrack_stationary"]) {
        if (_core) _core->onGeofenceExit();
        return;
    }
    if ([region.identifier hasPrefix:@"__geotrack_"]) return;
    if (!_core) return;

    // Cancel dwell timer if running
    [self cancelDwellTimerForIdentifier:region.identifier];
    _core->onGeofenceEvent([region.identifier UTF8String], "EXIT");
}

#pragma mark - User Geofences

- (void)addGeofenceWithIdentifier:(NSString*)identifier
                         latitude:(double)latitude
                        longitude:(double)longitude
                           radius:(double)radius
                   notifyOnEntry:(BOOL)notifyOnEntry
                    notifyOnExit:(BOOL)notifyOnExit
                   notifyOnDwell:(BOOL)notifyOnDwell
                  loiteringDelay:(int)loiteringDelay {
    CLLocationCoordinate2D center = CLLocationCoordinate2DMake(latitude, longitude);
    double clampedRadius = MIN(radius, _locationManager.maximumRegionMonitoringDistance);

    CLCircularRegion* region = [[CLCircularRegion alloc] initWithCenter:center
                                                                 radius:clampedRadius
                                                             identifier:identifier];
    region.notifyOnEntry = notifyOnEntry;
    region.notifyOnExit = notifyOnExit;

    [_locationManager startMonitoringForRegion:region];

    // Store dwell config
    _geofenceConfigs[identifier] = @{
        @"notifyOnDwell": @(notifyOnDwell),
        @"loiteringDelay": @(loiteringDelay)
    };
}

- (void)removeGeofenceWithIdentifier:(NSString*)identifier {
    if ([identifier hasPrefix:@"__geotrack_"]) return;

    for (CLRegion* region in _locationManager.monitoredRegions) {
        if ([region.identifier isEqualToString:identifier]) {
            [_locationManager stopMonitoringForRegion:region];
            break;
        }
    }

    [self cancelDwellTimerForIdentifier:identifier];
    [_geofenceConfigs removeObjectForKey:identifier];
}

- (void)removeAllGeofences {
    NSMutableArray* toRemove = [NSMutableArray new];
    for (CLRegion* region in _locationManager.monitoredRegions) {
        if (![region.identifier hasPrefix:@"__geotrack_"]) {
            [toRemove addObject:region];
        }
    }
    for (CLRegion* region in toRemove) {
        [_locationManager stopMonitoringForRegion:region];
    }

    // Cancel all dwell timers
    for (NSString* identifier in _dwellTimers.allKeys) {
        dispatch_source_t timer = _dwellTimers[identifier];
        dispatch_source_cancel(timer);
    }
    [_dwellTimers removeAllObjects];
    [_geofenceConfigs removeAllObjects];
}

#pragma mark - Dwell Timers

- (void)startDwellTimerForIdentifier:(NSString*)identifier delay:(int)delayMs {
    [self cancelDwellTimerForIdentifier:identifier];

    __weak __typeof(self) weakSelf = self;
    dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    int64_t delayNs = (int64_t)delayMs * NSEC_PER_MSEC;
    dispatch_source_set_timer(timer,
                              dispatch_time(DISPATCH_TIME_NOW, delayNs),
                              DISPATCH_TIME_FOREVER,
                              (int64_t)(100 * NSEC_PER_MSEC));
    dispatch_source_set_event_handler(timer, ^{
        if (weakSelf.core) {
            weakSelf.core->onGeofenceEvent([identifier UTF8String], "DWELL");
        }
        dispatch_source_cancel(timer);
        [weakSelf.dwellTimers removeObjectForKey:identifier];
    });
    dispatch_resume(timer);
    _dwellTimers[identifier] = timer;
}

- (void)cancelDwellTimerForIdentifier:(NSString*)identifier {
    dispatch_source_t timer = _dwellTimers[identifier];
    if (timer) {
        dispatch_source_cancel(timer);
        [_dwellTimers removeObjectForKey:identifier];
    }
}

#pragma mark - Stop Timer

- (void)startStopTimerWithSeconds:(int)seconds {
    [self cancelStopTimer];

    __weak __typeof(self) weakSelf = self;
    dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    dispatch_source_set_timer(timer,
                              dispatch_time(DISPATCH_TIME_NOW, (int64_t)(seconds * NSEC_PER_SEC)),
                              DISPATCH_TIME_FOREVER,
                              (int64_t)(1 * NSEC_PER_SEC));
    dispatch_source_set_event_handler(timer, ^{
        if (weakSelf.core) {
            weakSelf.core->onStopTimerFired();
        }
        dispatch_source_cancel(timer);
        weakSelf.stopTimer = nil;
    });
    dispatch_resume(timer);
    _stopTimer = timer;
}

- (void)cancelStopTimer {
    if (_stopTimer) {
        dispatch_source_cancel(_stopTimer);
        _stopTimer = nil;
    }
}

#pragma mark - Schedule Timer

- (void)startScheduleTimerWithSeconds:(int)seconds {
    [self cancelScheduleTimer];

    if (seconds == 0) {
        [self evaluateScheduleNow];
        return;
    }

    __weak __typeof(self) weakSelf = self;
    dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    dispatch_source_set_timer(timer,
                              dispatch_time(DISPATCH_TIME_NOW, (int64_t)(seconds * NSEC_PER_SEC)),
                              DISPATCH_TIME_FOREVER,
                              (int64_t)(1 * NSEC_PER_SEC));
    dispatch_source_set_event_handler(timer, ^{
        [weakSelf evaluateScheduleNow];
        dispatch_source_cancel(timer);
        weakSelf.scheduleTimer = nil;
    });
    dispatch_resume(timer);
    _scheduleTimer = timer;
}

- (void)cancelScheduleTimer {
    if (_scheduleTimer) {
        dispatch_source_cancel(_scheduleTimer);
        _scheduleTimer = nil;
    }
}

- (void)evaluateScheduleNow {
    if (!_core) return;

    NSCalendar* calendar = [NSCalendar currentCalendar];
    NSDateComponents* components = [calendar components:(NSCalendarUnitYear | NSCalendarUnitMonth | NSCalendarUnitDay |
                                                          NSCalendarUnitWeekday | NSCalendarUnitHour |
                                                          NSCalendarUnitMinute | NSCalendarUnitSecond)
                                                fromDate:[NSDate date]];
    // NSCalendar weekday: 1=Sunday, which matches our convention
    _core->onScheduleTimerFired(
        (int)components.year,
        (int)components.month,
        (int)components.day,
        (int)components.weekday,
        (int)components.hour,
        (int)components.minute,
        (int)components.second
    );
}

#pragma mark - CLLocationManagerDelegate

- (void)locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray<CLLocation *> *)locations {
    if (!_core) return;

    // Piggyback schedule evaluation on location events
    if (_schedulerActive) {
        [self evaluateScheduleNow];
    }

    for (CLLocation* clLocation in locations) {
        geotrack::Location loc;
        loc.latitude = clLocation.coordinate.latitude;
        loc.longitude = clLocation.coordinate.longitude;
        loc.altitude = clLocation.altitude;
        loc.speed = clLocation.speed;
        loc.heading = clLocation.course;
        loc.accuracy = clLocation.horizontalAccuracy;
        loc.altitudeAccuracy = clLocation.verticalAccuracy;
        loc.speedAccuracy = clLocation.speedAccuracy;
        loc.headingAccuracy = clLocation.courseAccuracy;
        // isMoving is set by the core state machine

        // ISO 8601 timestamp
        NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
        formatter.dateFormat = @"yyyy-MM-dd'T'HH:mm:ss.SSS'Z'";
        formatter.timeZone = [NSTimeZone timeZoneWithAbbreviation:@"UTC"];
        NSString* ts = [formatter stringFromDate:clLocation.timestamp];
        loc.timestamp = [ts UTF8String];

        // Generate UUID
        loc.uuid = [[[NSUUID UUID] UUIDString] UTF8String];

        NSString* now = [formatter stringFromDate:[NSDate date]];
        loc.createdAt = [now UTF8String];

        _core->onLocationReceived(loc);
    }
}

- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error {
    NSLog(@"[Geotrack] Location error: %@", error.localizedDescription);
}

@end
