#import <GeomonySpec/GeomonySpec.h>
#import <React/RCTEventEmitter.h>

@class GeomonyLocationDelegate;

namespace geomony {
    class GeomonyCore;
}

@interface Geomony : RCTEventEmitter <NativeGeomonySpec>

@property (nonatomic, strong) GeomonyLocationDelegate* locationDelegate;

- (geomony::GeomonyCore*)core;

@end
