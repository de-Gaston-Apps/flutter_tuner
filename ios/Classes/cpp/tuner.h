#ifndef TUNER_H
#define TUNER_H

#ifdef __cplusplus

#include <vector>

// Forward declaration of the C struct to keep the header clean
struct _pitch_yinfft_t;
typedef struct _pitch_yinfft_t pitch_yinfft_t;

class TunerCPP {
public:
    /**
     * Constructor
     * @param sampleRate The audio sample rate (e.g., 44100 or 16000)
     * @param bufferSize The size of the FFT window (e.g., 2048 or 4096)
     */
    TunerCPP(int sampleRate, int bufferSize);

    /**
     * Destructor - handles memory cleanup for the YinFFT instance
     */
    ~TunerCPP();

    /**
     * Processes a block of audio data to find the fundamental frequency.
     * @param audioData Vector of double samples from the microphone
     * @return The detected frequency in Hz, -2.0 for silence/noise, or -1.0 for error
     */
    double findFrequency(const std::vector<double> &audioData);

private:
    // Opaque pointer to the C pitch detector instance
    pitch_yinfft_t *tunerPtr;

    // Internal buffer used to convert double input to float for processing
    std::vector<float> floatBuffer;
};

#endif // __cplusplus

#endif // TUNER_H
