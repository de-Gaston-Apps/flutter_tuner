#ifndef TUNER_H
#define TUNER_H

#ifdef __cplusplus
#include <vector>

// Wrap the C library header so it plays nicely with C++
extern "C"
{
#include "src/dywapitchtrack.h"
}

class TunerCPP
{
public:
    TunerCPP(int sampleRate, int bufferSize);
    ~TunerCPP();

    double findFrequency(const std::vector<double> &audioData);

private:
    int sampleRate;

    // The library's internal state tracker
    dywapitchtracker pitchTracker;

    // Pre-allocated buffer to safely pass data to the C library
    // without triggering real-time memory allocation lag
    std::vector<double> mutableBuffer;
};

#endif // __cplusplus
#endif // TUNER_H