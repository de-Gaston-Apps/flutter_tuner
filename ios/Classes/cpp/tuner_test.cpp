#include "tuner.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void test_440hz() {
    int sampleRate = 44100;
    int bufferSize = 2048;
    TunerCPP tuner(sampleRate, bufferSize);

    std::vector<double> audioData(bufferSize);
    double frequency = 440.0;

    // Generate 440Hz sine wave
    for (int i = 0; i < bufferSize; ++i) {
        audioData[i] = std::sin(2.0 * M_PI * frequency * i / sampleRate);
    }

    double detected = tuner.findFrequency(audioData);
    std::cout << "Detected frequency: " << detected << " Hz" << std::endl;

    // Allow some margin of error for FFT binning/interpolation
    assert(std::abs(detected - 440.0) < 1.0);
    std::cout << "Test 440Hz passed!" << std::endl;
}

int main() {
    try {
        test_440hz();
        std::cout << "All C++ tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
    return 0;
}
