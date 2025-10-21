#include <gtest/gtest.h>
#include "tuner.h"
#include <vector>
#include <cmath>

// Test to see if the tuner can correctly identify the frequency of a pure sine wave.
TEST(TunerTest, SineWave) {
    const int sampleRate = 44100;
    const int bufferSize = 4096;
    const double frequency = 440.0; // A4 note

    // Generate a sine wave
    std::vector<double> audioData(bufferSize);
    for (int i = 0; i < bufferSize; ++i) {
        audioData[i] = sin(2 * M_PI * frequency * i / sampleRate);
    }

    // Initialize the tuner
    TunerCPP tuner(sampleRate, bufferSize);

    // Find the frequency
    double detectedFrequency = tuner.findFrequency(audioData);

    // Check if the detected frequency is close to the actual frequency
    ASSERT_NEAR(detectedFrequency, frequency, 1.0); // Tolerance of 1.0 Hz
}
