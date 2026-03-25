#import <BearingsSpec/BearingsSpec.h>
#import <React/RCTEventEmitter.h>

@class BearingsLocationDelegate;

namespace bearings {
    class BearingsCore;
}

@interface Bearings : RCTEventEmitter <NativeBearingsSpec>

@property (nonatomic, strong) BearingsLocationDelegate* locationDelegate;

- (bearings::BearingsCore*)core;

@end
