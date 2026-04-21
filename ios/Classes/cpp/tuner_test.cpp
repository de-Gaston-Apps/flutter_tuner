#include "tuner.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void test_frequency(int16_t frequency)
{
    int sampleRate = 44100;
    int bufferSize = 4096;
    TunerCPP tuner(sampleRate, bufferSize);
    float audioData[bufferSize];

    // Generate 440Hz sine wave
    for (int i = 0; i < bufferSize; ++i)
    {
        audioData[i] = static_cast<float>(std::sin(2.0 * M_PI * frequency * i / sampleRate));
    }

    double detected = tuner.findFrequency(audioData, bufferSize);
    std::cout << "Detected frequency: " << detected << " Hz" << std::endl;

    // Allow some margin of error for FFT binning/interpolation
    assert(std::abs(detected - frequency) < frequency * 0.01); // 1% tolerance
    std::cout << "Test " << frequency << "Hz passed!" << std::endl;
}

int main()
{
    try
    {
        // TODO: Add more test frequencies and edge cases (silence, noise, and real instrument samples)
        test_frequency(110);
        test_frequency(220);
        test_frequency(440);
        test_frequency(880);
        test_frequency(1760);
        std::cout << "All C++ tests passed!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
    return 0;
}
