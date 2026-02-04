#include "tuner.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void test_clean_signal() {
    int sampleRate = 44100;
    int bufferSize = 1024;
    TunerCPP tuner(sampleRate, bufferSize);

    double frequency = 440.0;
    std::vector<double> audioData(bufferSize);
    for (int i = 0; i < bufferSize; ++i) {
        audioData[i] = std::sin(2.0 * M_PI * frequency * i / sampleRate);
    }

    double detectedFreq = tuner.findFrequency(audioData);
    std::cout << "Clean 440Hz: detected " << detectedFreq << " Hz" << std::endl;
    assert(std::abs(detectedFreq - 440.0) < 2.0);
}

void test_noise_only() {
    int sampleRate = 44100;
    int bufferSize = 1024;
    TunerCPP tuner(sampleRate, bufferSize);

    std::vector<double> audioData(bufferSize);
    for (int i = 0; i < bufferSize; ++i) {
        audioData[i] = ((double)rand() / RAND_MAX) * 0.001; // Very low level noise
    }

    double detectedFreq = tuner.findFrequency(audioData);
    std::cout << "Noise only: detected " << detectedFreq << std::endl;
    assert(detectedFreq == -2.0); // TOO_QUIET_FREQ
}

void test_smoothing() {
    int sampleRate = 44100;
    int bufferSize = 1024;
    TunerCPP tuner(sampleRate, bufferSize);

    double frequency = 440.0;

    // Send 3 frames of 440Hz
    for (int j = 0; j < 3; ++j) {
        std::vector<double> audioData(bufferSize);
        for (int i = 0; i < bufferSize; ++i) {
            audioData[i] = std::sin(2.0 * M_PI * frequency * i / sampleRate);
        }
        tuner.findFrequency(audioData);
    }

    // Send 1 frame of "glitch" 1000Hz
    // We need to make it strong enough to be detected
    std::vector<double> glitchData(bufferSize);
    for (int i = 0; i < bufferSize; ++i) {
        glitchData[i] = std::sin(2.0 * M_PI * 1000.0 * i / sampleRate);
    }
    double detectedFreq = tuner.findFrequency(glitchData);

    std::cout << "After glitch: detected " << detectedFreq << " Hz" << std::endl;
    // History should be [~438, ~438, ~438, ~1000] -> median ~438
    assert(std::abs(detectedFreq - 440.0) < 2.0);

    // Send another glitch
    detectedFreq = tuner.findFrequency(glitchData);
    std::cout << "After 2nd glitch: detected " << detectedFreq << " Hz" << std::endl;
    // History should be [~438, ~438, ~438, ~1000, ~1000] -> median ~438
    assert(std::abs(detectedFreq - 440.0) < 2.0);

    // Send 3rd glitch -> history [~438, ~438, ~1000, ~1000, ~1000] -> median ~1000
    detectedFreq = tuner.findFrequency(glitchData);
    std::cout << "After 3rd glitch: detected " << detectedFreq << " Hz" << std::endl;
    assert(std::abs(detectedFreq - 1000.0) < 5.0); // Higher freq might have slightly more error in Hz
}

int main() {
    srand(42);
    test_clean_signal();
    test_noise_only();
    test_smoothing();
    std::cout << "All C++ tests passed!" << std::endl;
    return 0;
}
