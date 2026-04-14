#import "TunerBridge.h"
#import "tuner.h" // Your new TunerCPP header
#import <vector>

@interface TunerBridge () {
    // We use a raw pointer to the C++ class here
    TunerCPP *cppTuner;
}
@end

@implementation TunerBridge

- (instancetype)initWithSampleRate:(int)sampleRate bufferSize:(int)bufferSize {
    self = [super init];
    if (self) {
        // Instantiate the C++ class directly
        cppTuner = new TunerCPP(sampleRate, bufferSize);
        if (!cppTuner) {
            NSLog(@"[TunerBridge] Failed to allocate TunerCPP");
        }
    }
    return self;
}

- (double)findFrequency:(const double *)data length:(int)length {
    if (!cppTuner) {
        return -1.0;
    }

    // Convert the raw double array from Swift/Objective-C into a std::vector
    // The C++ side expects a std::vector<double>
    std::vector<double> audioVector(data, data + length);

    // Call the C++ method
    return cppTuner->findFrequency(audioVector);
}

- (void)dispose {
    if (cppTuner) {
        // Standard C++ cleanup
        delete cppTuner;
        cppTuner = nullptr;
    }
}

- (void)dealloc {
    // Ensure cleanup happens even if dispose wasn't called manually
    [self dispose];
}

@end
