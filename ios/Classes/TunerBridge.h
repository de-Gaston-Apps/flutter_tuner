#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface TunerBridge : NSObject

- (instancetype)initWithSampleRate:(int)sampleRate bufferSize:(int)bufferSize;
- (void)pushData:(const double *)data length:(int)length;
- (double)findFrequency;
- (double)findFrequency:(const double *)data length:(int)length;
- (void)dispose;

@end

NS_ASSUME_NONNULL_END