#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface TunerWrapper : NSObject

- (instancetype)initWithSampleRate:(int)sampleRate bufferSize:(int)bufferSize;
- (double)findFrequency:(const double*)audioData dataSize:(int)dataSize;

@end

NS_ASSUME_NONNULL_END
