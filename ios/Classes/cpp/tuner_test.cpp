#include "tuner.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <iomanip>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void testFrequency(double targetFreq, int sampleRate, int bufferSize) {
    TunerCPP tuner(sampleRate, bufferSize);
    std::vector<double> audioData(bufferSize);

    // Generate sine wave with harmonics
    for (int i = 0; i < bufferSize; i++) {
        double t = (double)i / sampleRate;
        // Fundamental
        audioData[i] = std::sin(2.0 * M_PI * targetFreq * t);
        // 2nd harmonic (stronger than fundamental sometimes in real instruments)
        audioData[i] += 0.8 * std::sin(2.0 * M_PI * 2.0 * targetFreq * t);
        // 3rd harmonic
        audioData[i] += 0.5 * std::sin(2.0 * M_PI * 3.0 * targetFreq * t);
    }

    double detectedFreq = tuner.findFrequency(audioData);
    std::cout << "Target: " << std::fixed << std::setprecision(2) << targetFreq
              << " Hz, Detected: " << detectedFreq << " Hz";

    double error = std::abs(detectedFreq - targetFreq);
    std::cout << " (Error: " << error << " Hz)" << std::endl;

    // Accuracy should be within 0.5 Hz for these clean signals
    assert(error < 0.5);
}

int main() {
    std::cout << "Running Tuner Accuracy Tests..." << std::endl;

    // Test some guitar-relevant frequencies
    testFrequency(82.41, 16000, 4096);  // Low E
    testFrequency(110.00, 16000, 4096); // A
    testFrequency(146.83, 16000, 4096); // D
    testFrequency(196.00, 16000, 4096); // G
    testFrequency(246.94, 16000, 4096); // B
    testFrequency(329.63, 16000, 4096); // High E

    // Higher frequency
    testFrequency(440.00, 16000, 4096); // A4

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
