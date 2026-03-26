#pragma once

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>
#import <CoreMotion/CoreMotion.h>

namespace geomony {
    class GeomonyCore;
}

@interface GeomonyLocationDelegate : NSObject <CLLocationManagerDelegate>

@property (nonatomic, assign) geomony::GeomonyCore* core;
@property (nonatomic, strong) CLLocationManager* locationManager;
@property (nonatomic, strong) CMMotionActivityManager* motionActivityManager;
@property (nonatomic, strong) CLCircularRegion* stationaryRegion;
@property (nonatomic, assign) dispatch_source_t stopTimer;
@property (nonatomic, assign) dispatch_source_t scheduleTimer;
@property (nonatomic, assign) BOOL schedulerActive;
@property (nonatomic, strong) NSMutableDictionary<NSString*, dispatch_source_t>* dwellTimers;
@property (nonatomic, strong) NSMutableDictionary<NSString*, NSDictionary*>* geofenceConfigs;
@property (nonatomic, assign) dispatch_source_t syncRetryTimer;

- (void)startWithDesiredAccuracy:(double)desiredAccuracy distanceFilter:(double)distanceFilter;
- (void)stop;
- (void)startMotionActivity;
- (void)stopMotionActivity;
- (void)startStationaryGeofenceAtLatitude:(double)latitude longitude:(double)longitude radius:(double)radius;
- (void)stopStationaryGeofence;
- (void)startStopTimerWithSeconds:(int)seconds;
- (void)cancelStopTimer;

- (void)addGeofenceWithIdentifier:(NSString*)identifier
                         latitude:(double)latitude
                        longitude:(double)longitude
                           radius:(double)radius
                   notifyOnEntry:(BOOL)notifyOnEntry
                    notifyOnExit:(BOOL)notifyOnExit
                   notifyOnDwell:(BOOL)notifyOnDwell
                  loiteringDelay:(int)loiteringDelay;
- (void)removeGeofenceWithIdentifier:(NSString*)identifier;
- (void)removeAllGeofences;

- (void)startScheduleTimerWithSeconds:(int)seconds;
- (void)cancelScheduleTimer;
- (void)evaluateScheduleNow;

- (void)sendHTTPRequest:(NSString*)url payload:(NSString*)jsonPayload headers:(NSString*)headersJson requestId:(int)requestId;
- (void)startSyncRetryTimerWithSeconds:(int)seconds;
- (void)cancelSyncRetryTimer;

@end
