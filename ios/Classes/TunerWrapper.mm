#import "TunerWrapper.h"
#import "tuner.h"
#import <vector>

@interface TunerWrapper ()
{
    TunerCPP* _tuner;
}
@end

@implementation TunerWrapper

- (instancetype)initWithSampleRate:(int)sampleRate bufferSize:(int)bufferSize
{
    self = [super init];
    if (self) {
        _tuner = new TunerCPP(sampleRate, bufferSize);
    }
    return self;
}

- (void)dealloc
{
    delete _tuner;
}

- (double)findFrequency:(const double*)audioData dataSize:(int)dataSize
{
    std::vector<double> audioDataVec(audioData, audioData + dataSize);
    return _tuner->findFrequency(audioDataVec);
}

@end
