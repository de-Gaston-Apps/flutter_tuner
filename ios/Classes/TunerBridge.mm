#import "TunerBridge.h"
#import "tuner.h"
#import <vector>

@interface TunerBridge () {
    TunerCPP *cppTuner;
}
@end

@implementation TunerBridge

- (instancetype)initWithSampleRate:(int)sampleRate bufferSize:(int)bufferSize {
    self = [super init];
    if (self) {
        cppTuner = new TunerCPP(sampleRate, bufferSize);
        if (!cppTuner) {
            NSLog(@"[TunerBridge] Failed to allocate TunerCPP");
        }
    }
    return self;
}

- (double)findFrequency:(const float *)data length:(int)length {
    if (!cppTuner) {
        NSLog(@"[TunerBridge::findFrequency] cppTuner was null");
        return -1.0;
    }

    return cppTuner->findFrequency(data, length);
}

- (void)dispose {
    if (cppTuner) {
        delete cppTuner;
        cppTuner = nullptr;
    }
}

- (void)dealloc {
    [self dispose];
}

@end
