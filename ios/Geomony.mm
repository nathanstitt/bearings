#import "Geomony.h"
#import "GeomonyPlatformBridge.h"
#import "GeomonyLocationDelegate.h"

#include "geomony/GeomonyCore.h"
#include "geomony/Logger.h"

#include <memory>

@implementation Geomony {
    std::shared_ptr<geomony::GeomonyCore> _core;
    std::shared_ptr<GeomonyPlatformBridgeImpl> _bridge;
    BOOL _hasListeners;
}

+ (NSString *)moduleName {
    return @"Geomony";
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _locationDelegate = [[GeomonyLocationDelegate alloc] init];
        _bridge = std::make_shared<GeomonyPlatformBridgeImpl>(self);
        _core = std::make_shared<geomony::GeomonyCore>(_bridge);
        _locationDelegate.core = _core.get();

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(applicationWillTerminate:)
                                                     name:UIApplicationWillTerminateNotification
                                                   object:nil];

        geomony::Logger::instance().setHandler([](geomony::LogLevel level, const std::string& message) {
            NSString* msg = [NSString stringWithUTF8String:message.c_str()];
            switch (level) {
                case geomony::LogLevel::Debug:
                case geomony::LogLevel::Info:
                    NSLog(@"[Geomony] %@", msg);
                    break;
                case geomony::LogLevel::Warning:
                    NSLog(@"[Geomony WARN] %@", msg);
                    break;
                case geomony::LogLevel::Error:
                    NSLog(@"[Geomony ERROR] %@", msg);
                    break;
            }
        });
    }
    return self;
}

- (geomony::GeomonyCore*)core {
    return _core.get();
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    if (_core) {
        _core->onTerminate();
    }
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (NSArray<NSString *> *)supportedEvents {
    return @[@"location", @"motionchange", @"activitychange", @"geofence", @"schedule", @"sync", @"error"];
}

- (void)startObserving {
    _hasListeners = YES;
}

- (void)stopObserving {
    _hasListeners = NO;
}

+ (BOOL)requiresMainQueueSetup {
    return NO;
}

- (void)configure:(NSString *)config
          resolve:(RCTPromiseResolveBlock)resolve
           reject:(RCTPromiseRejectBlock)reject {
    std::string configStr = [config UTF8String];
    _core->configure(configStr);

    NSString* state = [NSString stringWithUTF8String:_core->getState().c_str()];
    resolve(state);
}

- (void)start:(RCTPromiseResolveBlock)resolve
       reject:(RCTPromiseRejectBlock)reject {
    _core->start();
    NSString* state = [NSString stringWithUTF8String:_core->getState().c_str()];
    resolve(state);
}

- (void)stop:(RCTPromiseResolveBlock)resolve
      reject:(RCTPromiseRejectBlock)reject {
    _core->stop();
    NSString* state = [NSString stringWithUTF8String:_core->getState().c_str()];
    resolve(state);
}

- (void)getState:(RCTPromiseResolveBlock)resolve
          reject:(RCTPromiseRejectBlock)reject {
    NSString* state = [NSString stringWithUTF8String:_core->getState().c_str()];
    resolve(state);
}

- (void)getLocations:(RCTPromiseResolveBlock)resolve
              reject:(RCTPromiseRejectBlock)reject {
    NSString* locations = [NSString stringWithUTF8String:_core->getLocations().c_str()];
    resolve(locations);
}

- (void)getCount:(RCTPromiseResolveBlock)resolve
          reject:(RCTPromiseRejectBlock)reject {
    resolve(@(_core->getCount()));
}

- (void)destroyLocations:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject {
    BOOL result = _core->destroyLocations();
    resolve(@(result));
}

- (void)addGeofence:(NSString *)geofenceJson
            resolve:(RCTPromiseResolveBlock)resolve
             reject:(RCTPromiseRejectBlock)reject {
    std::string json = [geofenceJson UTF8String];
    BOOL result = _core->addGeofence(json);
    resolve(@(result));
}

- (void)removeGeofence:(NSString *)identifier
               resolve:(RCTPromiseResolveBlock)resolve
                reject:(RCTPromiseRejectBlock)reject {
    std::string id = [identifier UTF8String];
    BOOL result = _core->removeGeofence(id);
    resolve(@(result));
}

- (void)removeGeofences:(RCTPromiseResolveBlock)resolve
                 reject:(RCTPromiseRejectBlock)reject {
    BOOL result = _core->removeAllGeofences();
    resolve(@(result));
}

- (void)getGeofences:(RCTPromiseResolveBlock)resolve
              reject:(RCTPromiseRejectBlock)reject {
    NSString* geofences = [NSString stringWithUTF8String:_core->getGeofences().c_str()];
    resolve(geofences);
}

- (void)startSchedule:(RCTPromiseResolveBlock)resolve
               reject:(RCTPromiseRejectBlock)reject {
    _locationDelegate.schedulerActive = YES;
    _core->startSchedule();
    NSString* state = [NSString stringWithUTF8String:_core->getState().c_str()];
    resolve(state);
}

- (void)stopSchedule:(RCTPromiseResolveBlock)resolve
              reject:(RCTPromiseRejectBlock)reject {
    _locationDelegate.schedulerActive = NO;
    _core->stopSchedule();
    NSString* state = [NSString stringWithUTF8String:_core->getState().c_str()];
    resolve(state);
}

- (void)addListener:(NSString *)eventName {
    [super addListener:eventName];
}

- (void)removeListeners:(double)count {
    [super removeListeners:count];
}

- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams &)params {
    return std::make_shared<facebook::react::NativeGeomonySpecJSI>(params);
}

@end
