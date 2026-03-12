#include "tuner.h"
#include <algorithm>
#include <cstdlib> // For free()

TunerCPP::TunerCPP(int sampleRate, int bufferSize)
    : sampleRate(sampleRate)
{

    // Initialize YIN.
    // 0.15 is the standard threshold proven in the original YIN academic paper.
    Yin_init(&yinState, bufferSize, 0.25f);

    // Pre-allocate the integer buffer
    intBuffer.resize(bufferSize, 0);
}

TunerCPP::~TunerCPP()
{
    // The YIN C library allocates a float array internally during Yin_init.
    // We must free it to prevent memory leaks.
    if (yinState.yinBuffer != nullptr)
    {
        free(yinState.yinBuffer);
    }
}

double TunerCPP::findFrequency(const std::vector<double> &audioData)
{
    if (audioData.empty())
        return -1.0;

    if (intBuffer.size() < audioData.size())
    {
        intBuffer.resize(audioData.size());
    }

    // 1. THE C LIBRARY FIX: Manually clear the YIN buffer!
    // The Yin.c library accumulates values using "+=" but forgets to zero
    // them out between continuous frames. We must do it here.
    if (yinState.yinBuffer != nullptr)
    {
        for (int i = 0; i < yinState.halfBufferSize; i++)
        {
            yinState.yinBuffer[i] = 0.0f;
        }
    }

    // 2. THE FLUTTER DATA FIX: Auto-Gain Normalization
    // Find the loudest sample to handle iOS (-1.0 to 1.0) and Android (-32768 to 32767)
    double maxAmplitude = 0.0;
    for (double sample : audioData)
    {
        if (std::abs(sample) > maxAmplitude)
        {
            maxAmplitude = std::abs(sample);
        }
    }

    // If it's pure silence, skip the math entirely
    if (maxAmplitude < 1e-6)
    {
        return -2.0;
    }

    // Normalize everything dynamically and convert to 16-bit PCM for YIN
    for (size_t i = 0; i < audioData.size(); i++)
    {
        double normalizedSample = audioData[i] / maxAmplitude;
        intBuffer[i] = static_cast<int16_t>(normalizedSample * 32767.0);
    }

    // 3. Compute the pitch
    double rawPitch = Yin_getPitch(&yinState, intBuffer.data());

    // The library returns -1 if it can't find a pitch
    if (rawPitch <= 0.0)
    {
        return -2.0;
    }

    // 4. Adjust for Sample Rate Scaling
    // The Yin library often hardcodes 44100 into its pitch calculation.
    // We scale it so it remains perfectly accurate on 48000Hz Android mics.
    return rawPitch * (static_cast<double>(sampleRate) / 44100.0);
}
