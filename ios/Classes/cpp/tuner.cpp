//#include "tuner.h"
//#include <algorithm>
//#include <cstdlib> // For free()
//#include <cmath>
//
//TunerCPP::TunerCPP(int sampleRate, int bufferSize)
//    : sampleRate(sampleRate)
//{
//    // Initialize YIN with the exact buffer size we will analyze per call.
//    // 0.25f threshold chosen by previous code; you can tune this.
//    Yin_init(&yinState, static_cast<int16_t>(bufferSize), 0.25f);
//
//    // Pre-allocate the integer buffer
//    intBuffer.resize(static_cast<size_t>(bufferSize), 0);
//}
//
//TunerCPP::~TunerCPP()
//{
//    // Free internal YIN buffer allocated in Yin_init.
//    if (yinState.yinBuffer != nullptr)
//    {
//        free(yinState.yinBuffer);
//        yinState.yinBuffer = nullptr;
//    }
//}
//
//double TunerCPP::findFrequency(const std::vector<double> &audioData)
//{
//    // Guard: require at least yinState.bufferSize samples.
//    const int required = yinState.bufferSize;
//    if (audioData.size() < static_cast<size_t>(required))
//    {
//        return -2.0; // not enough data
//    }
//
//    // Ensure conversion buffer is large enough
//    if (intBuffer.size() < static_cast<size_t>(required))
//    {
//        intBuffer.resize(static_cast<size_t>(required));
//    }
//
//    // Clear YIN accumulation buffer before each run
//    if (yinState.yinBuffer != nullptr)
//    {
//        for (int i = 0; i < yinState.halfBufferSize; i++)
//        {
//            yinState.yinBuffer[i] = 0.0f;
//        }
//    }
//
//    // Auto-gain normalization on the window we will analyze (first 'required' samples)
//    double maxAmplitude = 0.0;
//    for (int i = 0; i < required; i++)
//    {
//        double s = audioData[static_cast<size_t>(i)];
//        double a = std::abs(s);
//        if (a > maxAmplitude) maxAmplitude = a;
//    }
//
//    if (maxAmplitude < 1e-6)
//    {
//        return -2.0; // silence
//    }
//
//    // Convert to 16-bit PCM for YIN (only the first 'required' samples)
//    for (int i = 0; i < required; i++)
//    {
//        double normalizedSample = audioData[static_cast<size_t>(i)] / maxAmplitude;
//        // Clamp before casting to avoid overflow
//        if (normalizedSample > 1.0) normalizedSample = 1.0;
//        if (normalizedSample < -1.0) normalizedSample = -1.0;
//        intBuffer[static_cast<size_t>(i)] = static_cast<int16_t>(normalizedSample * 32767.0);
//    }
//
//    // Compute the pitch
//    double rawPitch = Yin_getPitch(&yinState, intBuffer.data());
//
//    if (rawPitch <= 0.0)
//    {
//        return -2.0;
//    }
//
//    // Adjust for actual sample rate if Yin.c uses a fixed 44100
//    return rawPitch * (static_cast<double>(sampleRate) / 44100.0);
//}

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
