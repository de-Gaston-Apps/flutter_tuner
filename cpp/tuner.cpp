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
    if (audioData.size() < bufferSize) {
        std::cerr << "Not enough samples! Expecting > " << bufferSize << ". Got " << audioData.size() << std::endl;
        return -1.0;
    }

    // Split into overlapping windows
    const int factor = 4;
    const int windowLength = audioData.size() / factor;
    int startIndex = 0;
    int windowsCalculated = 0;

    std::vector<double> powers(windowLength / 2 + 1, 0.0);
    auto ham = hamming(windowLength);

    while (windowsCalculated != factor * 2 - 1) {
        std::vector<double> windowed(windowLength);
        for (int i = 0; i < windowLength; i++) {
            windowed[i] = audioData[startIndex + i] * ham[i];
        }

        auto f = rfft(windowed);
        auto fAbs = arrayComplexAbs(f);

        for (size_t i = 0; i < powers.size(); i++) {
            if (i < fAbs.size()) {
                powers[i] += fAbs[i];
            }
        }

        startIndex += windowLength / 2;
        windowsCalculated++;
    }

    bool foundSignal = false;
    double avgPower = mean(powers);

    // Find top NUM_PEAKS peaks
    const int NUM_PEAKS = 5;
    const int START_FFT_INDEX = 2;
    std::vector<int> peaks;
    for (int i = 0; i < NUM_PEAKS; i++) {
        double tmpMax = -1.0;
        int tmpIdx = -1;
        for (size_t j = START_FFT_INDEX; j < powers.size(); j++) {
            bool is_peak = true;
            for(int p : peaks) {
                if (p == j) {
                    is_peak = false;
                    break;
                }
            }
            if (powers[j] > tmpMax && is_peak) {
                tmpIdx = j;
                tmpMax = powers[j];
                if (tmpMax > avgPower * NOISE_SENSE_VERY_HIGH) {
                    foundSignal = true;
                }
            }
        }
        peaks.push_back(tmpIdx);
    }

    if (!foundSignal) {
        return -2.0; // TOO_QUIET_FREQ
    }

    int maxIndex = scorePowers(powers, peaks);

    // Parabolic interpolation for sub-bin accuracy
    auto logPowers = arrayLog(powers);
    auto par = parabolic(logPowers, peaks[maxIndex]);
    double trueI = par[0];

    double newFreq = sampleRate * trueI / windowLength;
    if (newFreq < 0.0) return -1.0;

    return newFreq;
}

std::vector<double> TunerCPP::hamming(int length) {
    std::vector<double> window(length);
    for (int i = 0; i < length; i++) {
        window[i] = 0.54 - 0.46 * std::cos(2.0 * M_PI * i / (length - 1));
    }
    return window;
}

std::vector<std::complex<double>> TunerCPP::rfft(const std::vector<double>& data) {
    int n = data.size();
    if ((n & (n - 1)) != 0) {
        throw std::runtime_error("Input size must be a power of 2 for rfft");
    }
    std::vector<std::complex<double>> complex_input(n);
    for (int i = 0; i < n; i++) {
        complex_input[i] = std::complex<double>(data[i], 0.0);
    }
    std::vector<std::complex<double>> spectrum(n);
    fft(complex_input, spectrum);
    return std::vector<std::complex<double>>(spectrum.begin(), spectrum.begin() + n / 2 + 1);
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

int TunerCPP::scorePowers(const std::vector<double>& powers, const std::vector<int>& peaks) {
    std::vector<double> sums;
    std::vector<double> factors = {2.0, 3.0, 4.0};

    for (int peak : peaks) {
        double sum = 0.0;
        if (peak <= 0) {
            sums.push_back(sum);
            continue;
        }

        for (double fFactor : factors) {
            int newIndex = static_cast<int>(peak * fFactor);
            if (newIndex < 2) newIndex = 2;
            if (newIndex + 3 > powers.size()) newIndex = powers.size() - 3;

            if (newIndex - 1 < powers.size()) sum += powers[newIndex - 1] * powers[peak];
            if (newIndex < powers.size()) sum += powers[newIndex] * powers[peak];
            if (newIndex + 1 < powers.size()) sum += powers[newIndex + 1] * powers[peak];
        }
        sums.push_back(sum);
    }

    int maxIndex = 0;
    for (size_t i = 1; i < sums.size(); i++) {
        if (sums[i] > sums[maxIndex]) {
            maxIndex = i;
        }
    }

    return maxIndex;
}