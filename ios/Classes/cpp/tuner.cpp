#include "tuner.h"
#include <algorithm>
#include <cmath>

TunerCPP::TunerCPP(int sampleRate, int bufferSize)
    : sampleRate(sampleRate)
{
    mutableBuffer.resize(bufferSize, 0.0);
    dywapitch_inittracking(&pitchTracker);
}

TunerCPP::~TunerCPP() = default;

double TunerCPP::findFrequency(const std::vector<double> &audioData)
{
    if (audioData.empty())
        return -1.0;

    // 1. Calculate the RMS volume of the current buffer
    double sumSquares = 0.0;
    for (double sample : audioData)
    {
        sumSquares += sample * sample;
    }
    double rms = std::sqrt(sumSquares / audioData.size());

    // 2. THE TWEAK: If the audio is very quiet (a note ended), reset the tracker's memory.
    // You may need to adjust this 0.01 threshold depending on your microphone's noise floor.
    if (rms < 0.01)
    {
        dywapitch_inittracking(&pitchTracker);
    }

    if (mutableBuffer.size() < audioData.size())
    {
        mutableBuffer.resize(audioData.size());
    }
    std::copy(audioData.begin(), audioData.end(), mutableBuffer.begin());

    double rawPitch = dywapitch_computepitch(
        &pitchTracker,
        mutableBuffer.data(),
        0,
        static_cast<int>(audioData.size()));

    if (rawPitch == 0.0)
        return -2.0;

    return rawPitch * (static_cast<double>(sampleRate) / 44100.0);
}