#import <GeotrackSpec/GeotrackSpec.h>
#import <React/RCTEventEmitter.h>

@class GeotrackLocationDelegate;

namespace geotrack {
    class GeotrackCore;
}

@interface Geotrack : RCTEventEmitter <NativeGeotrackSpec>

@property (nonatomic, strong) GeotrackLocationDelegate* locationDelegate;

- (geotrack::GeotrackCore*)core;

@end
