#ifndef TUNER_H
#define TUNER_H

#ifdef __cplusplus
#include <vector>
#include <cstdint>

// Wrap the C library header for C++ compatibility
extern "C"
{
#include "Yin.h"
}

class TunerCPP
{
public:
    TunerCPP(int sampleRate, int bufferSize);
    ~TunerCPP();

    double findFrequency(const std::vector<double> &audioData);

private:
    int sampleRate;

    // The library's state tracker
    Yin yinState;

    // Pre-allocated buffer for double -> int16_t conversion
    std::vector<int16_t> intBuffer;
};

#endif // __cplusplus
#endif // TUNER_H