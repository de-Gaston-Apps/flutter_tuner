#ifndef TUNER_H
#define TUNER_H

#ifdef __cplusplus
#include <vector>
#include <complex>

class TunerCPP {
public:
    TunerCPP(int sampleRate, int bufferSize);
    ~TunerCPP();

    double findFrequency(const std::vector<double>& audioData);

private:
    // Constants
    static const int RECORDER_CHANNELS = 1;
    static const int BYTES_PER_ELEMENT = 2;

    // Tuner specific values
    static constexpr double ATTENUATION = 0.80;
    static const int NOISE_SENSE_LOW = 5;
    static const int NOISE_SENSE_MED = 10;
    static const int NOISE_SENSE_HIGH = 15;
    static const int NOISE_SENSE_VERY_HIGH = 40;

    // Member variables
    int sampleRate;
    int bufferSize;

    // DSP functions
    std::vector<double> hamming(int length);
    std::vector<std::complex<double>> rfft(const std::vector<double>& data);
    std::vector<double> arrayComplexAbs(const std::vector<std::complex<double>>& complex);
    double mean(const std::vector<double>& arr);
    std::vector<double> arrayLog(const std::vector<double>& arr);
    std::vector<double> parabolic(const std::vector<double>& y, int i);
    void fft(const std::vector<std::complex<double>>& input, std::vector<std::complex<double>>& output);
    int scorePowers(const std::vector<double>& powers, const std::vector<int>& peaks);
};

#endif // __cplusplus
#endif // TUNER_H