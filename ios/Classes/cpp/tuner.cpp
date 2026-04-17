#include "tuner.h"
#include "yinfft.h"
#include <algorithm>
#include <vector>
#include <cmath>

TunerCPP::TunerCPP(int sampleRate, int bufferSize)
{
    tunerPtr = new_pitch_yinfft(sampleRate, bufferSize);
    floatBuffer.resize(static_cast<size_t>(bufferSize), 0.0f);
}

TunerCPP::~TunerCPP()
{
    if (tunerPtr != nullptr)
    {
        del_pitch_yinfft(static_cast<pitch_yinfft_t*>(tunerPtr));
        tunerPtr = nullptr;
    }
}

double TunerCPP::findFrequency(const float *audioData, int length)
{
    if (tunerPtr == nullptr) return -1.0;

    const size_t required = floatBuffer.size();
    if (static_cast<size_t>(length) < required)
    {
        return -2.0;
    }

    float frequency = pitch_yinfft_do(
        static_cast<pitch_yinfft_t*>(tunerPtr),
        audioData
    );

    float confidence = pitch_yinfft_get_confidence(static_cast<pitch_yinfft_t*>(tunerPtr));

    if (frequency <= 0.0f || confidence < 0.5f)
    {
        return -2.0;
    }

    return static_cast<double>(frequency);
}

double TunerCPP::findFrequency(const int16_t *audioData, int length)
{
    if (tunerPtr == nullptr) return -1.0;

    const size_t required = floatBuffer.size();
    if (static_cast<size_t>(length) < required)
    {
        return -2.0;
    }

    for (size_t i = 0; i < required; ++i)
    {
        floatBuffer[i] = static_cast<float>(audioData[i]) / 32768.0f;
    }

    float frequency = pitch_yinfft_do(
        static_cast<pitch_yinfft_t*>(tunerPtr),
        floatBuffer.data()
    );

    float confidence = pitch_yinfft_get_confidence(static_cast<pitch_yinfft_t*>(tunerPtr));
    
    if (frequency <= 0.0f || confidence < 0.5f)
    {
        return -2.0;
    }

    return static_cast<double>(frequency);
}
