#import "TunerBridge.h"
#import "cpp/tuner_wrapper.h" // This comes from your ../cpp folder

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

- (void)pushData:(const double *)data length:(int)length {
    if (!_handle) return;
    tuner_push_data(_handle, data, length);
}

- (double)findFrequency {
    if (!_handle) return -1.0;
    return tuner_analyze(_handle);
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