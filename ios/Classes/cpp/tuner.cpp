#include "tuner.h"
#include <cmath>
#include <numeric>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

TunerCPP::TunerCPP(int sampleRate, int bufferSize) : sampleRate(sampleRate), bufferSize(bufferSize) {}

TunerCPP::~TunerCPP() = default;

double TunerCPP::findFrequency(const std::vector<double>& audioData) {
    if (audioData.size() < (size_t)bufferSize) {
        std::cerr << "Not enough samples! Expecting > " << bufferSize << ". Got " << audioData.size() << std::endl;
        return -1.0;
    }

    int fftSize = 16384;
    auto ham = hamming(audioData.size());
    std::vector<double> windowed(audioData.size());
    for (size_t i = 0; i < audioData.size(); i++) {
        windowed[i] = audioData[i] * ham[i];
    }

    auto f = rfft(windowed, fftSize);
    auto magnitudes = arrayComplexAbs(f);

    // HPS (Harmonic Product Spectrum)
    std::vector<double> hps = magnitudes;
    const int numHarmonics = 5;
    for (int h = 2; h <= numHarmonics; h++) {
        for (size_t i = 0; i < hps.size() / h; i++) {
            hps[i] *= magnitudes[i * h];
        }
    }

    // Find peak in HPS spectrum
    const int START_INDEX = static_cast<int>(20.0 * fftSize / sampleRate); // Ignore below 20Hz
    double maxVal = -1.0;
    int maxIdx = -1;
    for (size_t i = START_INDEX; i < hps.size() / numHarmonics; i++) {
        if (hps[i] > maxVal) {
            maxVal = hps[i];
            maxIdx = i;
        }
    }

    if (maxIdx == -1) return -2.0;

    // Noise detection: check if peak is significant
    double avgMag = mean(magnitudes);
    if (magnitudes[maxIdx] < avgMag * NOISE_SENSE_VERY_HIGH) {
        return -2.0; // TOO_QUIET
    }

    // Parabolic interpolation on HPS for sub-bin accuracy
    // Use log-magnitude for better interpolation
    std::vector<double> logHps = arrayLog(hps);
    auto par = parabolic(logHps, maxIdx);
    double trueI = par[0];

    double frequency = sampleRate * trueI / fftSize;
    return frequency;
}

std::vector<double> TunerCPP::hamming(int length) {
    std::vector<double> window(length);
    for (int i = 0; i < length; i++) {
        window[i] = 0.54 - 0.46 * std::cos(2.0 * M_PI * i / (length - 1));
    }
    return window;
}

std::vector<std::complex<double>> TunerCPP::rfft(const std::vector<double>& data, int fftSize) {
    if ((fftSize & (fftSize - 1)) != 0) {
        throw std::runtime_error("fftSize must be a power of 2");
    }
    std::vector<std::complex<double>> complex_input(fftSize, 0.0);
    for (size_t i = 0; i < std::min(data.size(), (size_t)fftSize); i++) {
        complex_input[i] = std::complex<double>(data[i], 0.0);
    }
    std::vector<std::complex<double>> spectrum(fftSize);
    fft(complex_input, spectrum);
    return std::vector<std::complex<double>>(spectrum.begin(), spectrum.begin() + fftSize / 2 + 1);
}

std::vector<double> TunerCPP::arrayComplexAbs(const std::vector<std::complex<double>>& complex) {
    std::vector<double> magnitudes(complex.size());
    for (size_t i = 0; i < complex.size(); ++i) {
        magnitudes[i] = std::abs(complex[i]);
    }
    return magnitudes;
}

double TunerCPP::mean(const std::vector<double>& arr) {
    if (arr.empty()) {
        return 0.0;
    }
    return std::accumulate(arr.begin(), arr.end(), 0.0) / arr.size();
}

std::vector<double> TunerCPP::arrayLog(const std::vector<double>& arr) {
    std::vector<double> logs(arr.size());
    for (size_t i = 0; i < arr.size(); ++i) {
        logs[i] = std::log(std::max(arr[i], 1e-9)); // Use a small epsilon
    }
    return logs;
}

std::vector<double> TunerCPP::parabolic(const std::vector<double>& y, int i) {
    int idx = i;
    if (i < 1) idx = 1;
    if (i > y.size() - 2) idx = y.size() - 2;

    double alpha = y[idx - 1];
    double beta = y[idx];
    double gamma = y[idx + 1];

    double p = 0.5 * (alpha - gamma) / (alpha - 2.0 * beta + gamma);
    double trueI = idx + p;
    double value = beta - 0.25 * (alpha - gamma) * p;

    return {trueI, value};
}

void fftRecursive(const std::vector<std::complex<double>>& input, std::vector<std::complex<double>>& output) {
    int n = input.size();
    if (n == 1) {
        output[0] = input[0];
        return;
    }
    if (n % 2 != 0) {
        throw std::runtime_error("FFT input length must be power of 2");
    }

    std::vector<std::complex<double>> even(n / 2), odd(n / 2);
    for (int i = 0; i < n / 2; i++) {
        even[i] = input[2 * i];
        odd[i] = input[2 * i + 1];
    }

    std::vector<std::complex<double>> fftEven(n / 2), fftOdd(n / 2);
    fftRecursive(even, fftEven);
    fftRecursive(odd, fftOdd);

    for (int k = 0; k < n / 2; k++) {
        double angle = -2.0 * M_PI * k / n;
        std::complex<double> wk(cos(angle), sin(angle));
        std::complex<double> t = wk * fftOdd[k];
        output[k] = fftEven[k] + t;
        output[k + n / 2] = fftEven[k] - t;
    }
}

void TunerCPP::fft(const std::vector<std::complex<double>>& input, std::vector<std::complex<double>>& output) {
    if (input.empty()) return;
    output.resize(input.size());
    fftRecursive(input, output);
}
