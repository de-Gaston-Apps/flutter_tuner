#import "TunerBridge.h"
#import "tuner_wrapper.h" // This comes from your ../cpp folder

@interface TunerBridge ()
// This is the "Class Extension" - it needs the Interface above to exist first
@property (nonatomic) Tuner* handle; 
@end

@implementation TunerBridge

- (instancetype)initWithSampleRate:(int)sampleRate bufferSize:(int)bufferSize {
    self = [super init];
    if (self) {
        _handle = create_tuner(sampleRate, bufferSize);
    }
    return self;
}

- (double)findFrequency:(const double *)data length:(int)length {
    if (!_handle) return -1.0;
    return find_frequency(_handle, data, length);
}

- (void)dispose {
    if (_handle) {
        destroy_tuner(_handle);
        _handle = NULL;
    }
}

- (void)dealloc {
    [self dispose];
}

@end