#include "tuner.h"
#include "yinfft.h"
#include <algorithm>
#include <vector>
#include <cmath>

TunerCPP::TunerCPP(int sampleRate, int bufferSize)
{
    // Create the standalone YinFFT instance
    // Note: ensure your tuner.h defines tunerPtr as pitch_yinfft_t* // or use a void pointer and cast it.
    tunerPtr = new_pitch_yinfft(sampleRate, bufferSize);
    
    // Pre-allocate a float buffer for processing
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

double TunerCPP::findFrequency(const std::vector<double> &audioData)
{
    // 1. Guard: Check if tuner instance exists
    if (tunerPtr == nullptr) return -1.0;

    // 2. Guard: Ensure we have enough data to fill the FFT window
    // Accessing the struct directly or passing the expected size is safer
    // Assuming bufsize from constructor is stored in this class
    const size_t required = floatBuffer.size();
    if (audioData.size() < required)
    {
        return -2.0; // Not enough data
    }

    // 3. Convert double input to float
    // The new algorithm operates on floats, which is standard for FFTs
    for (size_t i = 0; i < required; ++i)
    {
        floatBuffer[i] = static_cast<float>(audioData[i]);
    }

    // 4. Compute the frequency
    // pitch_yinfft_do handles weighting, windowing, and FFT correlation
    float frequency = pitch_yinfft_do(
        static_cast<pitch_yinfft_t*>(tunerPtr),
        floatBuffer.data()
    );

    // 5. Confidence Check
    // If confidence is too low, we return -2.0 to signify noise/unstable pitch
    float confidence = pitch_yinfft_get_confidence(static_cast<pitch_yinfft_t*>(tunerPtr));
    
    if (frequency <= 0.0f || confidence < 0.5f)
    {
        return -2.0; // Silence or noise
    }

    return static_cast<double>(frequency);
}
