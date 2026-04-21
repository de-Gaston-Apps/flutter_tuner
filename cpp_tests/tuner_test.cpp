#include "tuner.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Test infrastructure --------------------------------------------

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define ASSERT_NEAR(actual, expected, tolerance, label)                        \
  do {                                                                         \
    double _a = (actual), _e = (expected), _t = (tolerance);                   \
    if (std::abs(_a - _e) <= _t) {                                             \
      std::cout << "  [PASS] " << (label) << "  (got " << _a << ")"            \
                << std::endl;                                                  \
      g_tests_passed++;                                                        \
    } else {                                                                   \
      std::cerr << "  [FAIL] " << (label) << "  expected=" << _e               \
                << " actual=" << _a << " tol=" << _t << std::endl;             \
      g_tests_failed++;                                                        \
    }                                                                          \
  } while (0)

#define ASSERT_EQ(actual, expected, label)                                     \
  ASSERT_NEAR(actual, expected, 0.001, label)

// --- Helpers --------------------------------------------------------

static const int SAMPLE_RATE = 44100;
static const int BUFFER_SIZE = 4096;

/** Generate a pure sine wave into a float buffer. */
static void generate_sine(float *buf, int len, double freq,
                          double amplitude = 0.9) {
  for (int i = 0; i < len; i++)
    buf[i] = static_cast<float>(amplitude *
                                std::sin(2.0 * M_PI * freq * i / SAMPLE_RATE));
}

/** Generate a pure sine wave into an int16_t buffer. */
static void generate_sine_i16(int16_t *buf, int len, double freq,
                              double amplitude = 0.9) {
  for (int i = 0; i < len; i++)
    buf[i] = static_cast<int16_t>(
        amplitude * 32767.0 * std::sin(2.0 * M_PI * freq * i / SAMPLE_RATE));
}

/** Add harmonics on top of a float buffer. Harmonic n is at n*freq. */
static void add_harmonic(float *buf, int len, double fundamentalFreq,
                         int harmonicNumber, double relativeAmplitude) {
  double hFreq = fundamentalFreq * harmonicNumber;
  for (int i = 0; i < len; i++)
    buf[i] += static_cast<float>(
        relativeAmplitude * std::sin(2.0 * M_PI * hFreq * i / SAMPLE_RATE));
}

/** Fill buffer with uniform random noise in [-amplitude, +amplitude]. */
static void generate_noise(float *buf, int len, double amplitude) {
  for (int i = 0; i < len; i++)
    buf[i] = static_cast<float>(amplitude * (2.0 * rand() / RAND_MAX - 1.0));
}

static void generate_noise_i16(int16_t *buf, int len, double amplitude) {
  for (int i = 0; i < len; i++)
    buf[i] = static_cast<int16_t>(amplitude * 32767.0 *
                                  (2.0 * rand() / RAND_MAX - 1.0));
}

// --- WAV loader (16-bit PCM mono) -----------------------------------

struct WavData {
  int sampleRate;
  int numSamples;
  std::vector<float> samples;
  bool loaded;
};

static WavData load_wav_mono_float(const char *path) {
  WavData w = {0, 0, {}, false};
  std::ifstream f(path, std::ios::binary);
  if (!f.is_open())
    return w;

  // RIFF header
  char riff[4];
  f.read(riff, 4);
  if (std::memcmp(riff, "RIFF", 4) != 0)
    return w;
  f.seekg(4, std::ios::cur); // skip file size
  char wave[4];
  f.read(wave, 4);
  if (std::memcmp(wave, "WAVE", 4) != 0)
    return w;

  int16_t numChannels = 0, bitsPerSample = 0;
  int32_t sr = 0, dataSize = 0;

  // Walk chunks
  while (f.good()) {
    char id[4];
    f.read(id, 4);
    int32_t sz;
    f.read(reinterpret_cast<char *>(&sz), 4);
    if (!f.good())
      break;

    if (std::memcmp(id, "fmt ", 4) == 0) {
      int16_t fmt;
      f.read(reinterpret_cast<char *>(&fmt), 2);
      f.read(reinterpret_cast<char *>(&numChannels), 2);
      f.read(reinterpret_cast<char *>(&sr), 4);
      f.seekg(6, std::ios::cur); // byte rate + block align
      f.read(reinterpret_cast<char *>(&bitsPerSample), 2);
      if (sz > 16)
        f.seekg(sz - 16, std::ios::cur);
    } else if (std::memcmp(id, "data", 4) == 0) {
      dataSize = sz;
      break;
    } else {
      f.seekg(sz, std::ios::cur);
    }
  }

  if (bitsPerSample != 16 || numChannels < 1 || dataSize <= 0)
    return w;

  int totalFrames = dataSize / (numChannels * 2);
  w.sampleRate = sr;
  w.numSamples = totalFrames;
  w.samples.resize(totalFrames);

  for (int i = 0; i < totalFrames; i++) {
    int16_t sample = 0;
    f.read(reinterpret_cast<char *>(&sample), 2);
    // Skip extra channels
    if (numChannels > 1)
      f.seekg((numChannels - 1) * 2, std::ios::cur);
    w.samples[i] = static_cast<float>(sample) / 32768.0f;
  }
  w.loaded = true;
  return w;
}

// ===================================================================
//  TEST SUITES
// ===================================================================

// --- 1. Silence → should return -2 ---------------------------------

static void test_silence_float() {
  std::cout << "\n=== Silence (float*) ===" << std::endl;
  TunerCPP tuner(SAMPLE_RATE, BUFFER_SIZE);
  float buf[BUFFER_SIZE];

  // All zeros
  std::memset(buf, 0, sizeof(buf));
  ASSERT_EQ(tuner.findFrequency(buf, BUFFER_SIZE), -2.0,
            "All-zero buffer → -2");

  // Near-zero (sub-noise-floor values)
  for (int i = 0; i < BUFFER_SIZE; i++)
    buf[i] = 1e-10f;
  ASSERT_EQ(tuner.findFrequency(buf, BUFFER_SIZE), -2.0,
            "Near-zero buffer → -2");
}

// --- 2. Too much noise → should return -2 --------------------------

static void test_too_much_noise_float() {
  std::cout << "\n=== Too much noise (float*) ===" << std::endl;
  TunerCPP tuner(SAMPLE_RATE, BUFFER_SIZE);
  float buf[BUFFER_SIZE];
  srand(42);

  // Full-scale white noise — no detectable pitch
  generate_noise(buf, BUFFER_SIZE, 1.0);
  double result = tuner.findFrequency(buf, BUFFER_SIZE);
  // With pure noise the confidence should be low → -2 (silence/noise)
  ASSERT_EQ(result, -2.0, "Full-scale white noise → -2");
}

// --- 3. Some noise but signal still dominant → detect pitch --------

static void test_noisy_signal_float() {
  std::cout << "\n=== Signal + moderate noise (float*) ===" << std::endl;
  TunerCPP tuner(SAMPLE_RATE, BUFFER_SIZE);
  float buf[BUFFER_SIZE];
  srand(123);

  double freq = 440.0;
  generate_sine(buf, BUFFER_SIZE, freq, 0.8);
  // Add noise at ~10% of signal amplitude (SNR ≈ 20 dB)
  float noise[BUFFER_SIZE];
  generate_noise(noise, BUFFER_SIZE, 0.08);
  for (int i = 0; i < BUFFER_SIZE; i++)
    buf[i] += noise[i];

  double detected = tuner.findFrequency(buf, BUFFER_SIZE);
  ASSERT_NEAR(detected, freq, freq * 0.02,
              "440 Hz + 10% noise → ~440 Hz (2% tol)");
}

// --- 4. Pure sine sweeps (both overloads) --------------------------

static void test_pure_sines_float() {
  std::cout << "\n=== Pure sines (float*) ===" << std::endl;
  TunerCPP tuner(SAMPLE_RATE, BUFFER_SIZE);
  float buf[BUFFER_SIZE];

  double freqs[] = {82.41, 110.0,  146.83, 220.0, 261.63,  329.63,
                    440.0, 523.25, 659.25, 880.0, 1046.50, 1760.0};
  int n = sizeof(freqs) / sizeof(freqs[0]);
  for (int f = 0; f < n; f++) {
    generate_sine(buf, BUFFER_SIZE, freqs[f]);
    double detected = tuner.findFrequency(buf, BUFFER_SIZE);
    char label[64];
    snprintf(label, sizeof(label), "Pure sine %.2f Hz (float)", freqs[f]);
    ASSERT_NEAR(detected, freqs[f], freqs[f] * 0.01, label);
  }
}

static void test_pure_sines_int16() {
  std::cout << "\n=== Pure sines (int16_t*) ===" << std::endl;
  TunerCPP tuner(SAMPLE_RATE, BUFFER_SIZE);
  int16_t buf[BUFFER_SIZE];

  double freqs[] = {110.0, 220.0, 440.0, 880.0};
  int n = sizeof(freqs) / sizeof(freqs[0]);
  for (int f = 0; f < n; f++) {
    generate_sine_i16(buf, BUFFER_SIZE, freqs[f]);
    double detected = tuner.findFrequency(buf, BUFFER_SIZE);
    char label[64];
    snprintf(label, sizeof(label), "Pure sine %.0f Hz (int16)", freqs[f]);
    ASSERT_NEAR(detected, freqs[f], freqs[f] * 0.01, label);
  }
}

// --- 5. Harmonics (1st-4th) — fundamental should still win --------

static void test_harmonics_float() {
  std::cout << "\n=== Harmonics 1st-4th ===" << std::endl;
  TunerCPP tuner(SAMPLE_RATE, BUFFER_SIZE);
  float buf[BUFFER_SIZE];

  double fundamental = 220.0; // A3

  // Fundamental only
  generate_sine(buf, BUFFER_SIZE, fundamental, 1.0);
  ASSERT_NEAR(tuner.findFrequency(buf, BUFFER_SIZE), fundamental,
              fundamental * 0.01, "Fundamental only (220 Hz)");

  // Fundamental + 2nd harmonic (440 Hz at 50% amplitude)
  generate_sine(buf, BUFFER_SIZE, fundamental, 1.0);
  add_harmonic(buf, BUFFER_SIZE, fundamental, 2, 0.5);
  ASSERT_NEAR(tuner.findFrequency(buf, BUFFER_SIZE), fundamental,
              fundamental * 0.02, "+2nd harmonic (440 Hz @ 0.5)");

  // Fundamental + 2nd + 3rd harmonics
  generate_sine(buf, BUFFER_SIZE, fundamental, 1.0);
  add_harmonic(buf, BUFFER_SIZE, fundamental, 2, 0.5);
  add_harmonic(buf, BUFFER_SIZE, fundamental, 3, 0.33);
  ASSERT_NEAR(tuner.findFrequency(buf, BUFFER_SIZE), fundamental,
              fundamental * 0.02, "+2nd+3rd harmonics");

  // Fundamental + 2nd + 3rd + 4th harmonics (full timbre)
  generate_sine(buf, BUFFER_SIZE, fundamental, 1.0);
  add_harmonic(buf, BUFFER_SIZE, fundamental, 2, 0.5);
  add_harmonic(buf, BUFFER_SIZE, fundamental, 3, 0.33);
  add_harmonic(buf, BUFFER_SIZE, fundamental, 4, 0.25);
  ASSERT_NEAR(tuner.findFrequency(buf, BUFFER_SIZE), fundamental,
              fundamental * 0.02, "+2nd+3rd+4th harmonics");
}

// --- 6. Edge cases -------------------------------------------------

static void test_edge_cases() {
  std::cout << "\n=== Edge cases ===" << std::endl;
  TunerCPP tuner(SAMPLE_RATE, BUFFER_SIZE);
  float buf[BUFFER_SIZE];

  // Buffer too short → -2
  float tiny[16] = {0};
  ASSERT_EQ(tuner.findFrequency(tiny, 16), -2.0,
            "Buffer shorter than required → -2");

  // Very low frequency (sub-bass) — may or may not detect
  generate_sine(buf, BUFFER_SIZE, 55.0, 0.9);
  double low = tuner.findFrequency(buf, BUFFER_SIZE);
  // We just ensure it doesn't crash; detection may be -2 at this length
  std::cout << "  [INFO] 55 Hz detection result: " << low << " (informational)"
            << std::endl;

  // Very high frequency near Nyquist/2
  generate_sine(buf, BUFFER_SIZE, 4000.0, 0.9);
  double high = tuner.findFrequency(buf, BUFFER_SIZE);
  if (high > 0) {
    ASSERT_NEAR(high, 4000.0, 4000.0 * 0.02, "4000 Hz near high range");
  } else {
    std::cout << "  [INFO] 4000 Hz returned " << high
              << " (may be out of confident range)" << std::endl;
  }

  // DC offset — constant value, no pitch
  for (int i = 0; i < BUFFER_SIZE; i++)
    buf[i] = 0.5f;
  ASSERT_EQ(tuner.findFrequency(buf, BUFFER_SIZE), -2.0,
            "DC offset (constant) → -2");
}

// --- 7. Real instrument samples (WAV files — stubbed) --------------
//
// Place 16-bit PCM mono WAV files in cpp_tests/fixtures/:
//   guitar_a4.wav   — Guitar playing A4 (440 Hz)
//   piano_c4.wav    — Piano playing C4 (261.63 Hz)
//   violin_g3.wav   — Violin playing G3 (196 Hz)
//   cello_c2.wav    — Cello playing C2 (65.41 Hz)
//   ukulele_a4.wav  — Ukulele playing A4 (440 Hz)
//   voice_a3.wav    — Human voice singing A3 (220 Hz)
//
// Each test loads the WAV via the built-in loader. If the file is
// missing the test is SKIPPED (not failed) so CI stays green until
// you drop the files in.

struct WavTestCase {
  const char *filename;
  double expectedFreq;
  double tolerancePct; // e.g. 0.02 = 2%
  const char *label;
};

static void test_real_samples() {
  std::cout << "\n=== Real instrument samples (WAV) ===" << std::endl;

  WavTestCase cases[] = {
      {"fixtures/guitar_a4.wav", 440.00, 0.03, "Guitar A4 (440 Hz)"},
      {"fixtures/piano_c4.wav", 261.63, 0.03, "Piano C4 (261.63 Hz)"},
      {"fixtures/violin_g3.wav", 196.00, 0.03, "Violin G3 (196 Hz)"},
      {"fixtures/cello_c2.wav", 65.41, 0.05, "Cello C2 (65.41 Hz)"},
      {"fixtures/ukulele_a4.wav", 440.00, 0.03, "Ukulele A4 (440 Hz)"},
      {"fixtures/voice_a3.wav", 220.00, 0.03, "Voice A3 (220 Hz)"},
  };
  int n = sizeof(cases) / sizeof(cases[0]);

  for (int c = 0; c < n; c++) {
    WavData wav = load_wav_mono_float(cases[c].filename);
    if (!wav.loaded) {
      std::cout << "  [SKIP] " << cases[c].label
                << "  (file not found: " << cases[c].filename << ")"
                << std::endl;
      continue;
    }

    // Use the WAV's own sample rate; buffer size must be <= numSamples
    int bufSz = BUFFER_SIZE;
    if (wav.numSamples < bufSz) {
      std::cout << "  [SKIP] " << cases[c].label
                << "  (WAV too short: " << wav.numSamples << " samples)"
                << std::endl;
      continue;
    }

    // Select the middle BUFFER_SIZE samples — the steadiest part of the
    // note, avoiding attack/release transients.  This mirrors what the
    // algorithm processes per frame in production.
    int midStart = (wav.numSamples - bufSz) / 2;
    const float *midFloat = wav.samples.data() + midStart;

    TunerCPP tuner(wav.sampleRate, bufSz);

    double detected = tuner.findFrequency(midFloat, bufSz);
    char labelF[128];
    snprintf(labelF, sizeof(labelF), "%s", cases[c].label);
    ASSERT_NEAR(detected, cases[c].expectedFreq,
                cases[c].expectedFreq * cases[c].tolerancePct, labelF);
  }
}

// ===================================================================

int main() {
  std::cout << "==========================================" << std::endl;
  std::cout << "       TunerCPP — Comprehensive Tests     " << std::endl;
  std::cout << "==========================================" << std::endl;

  // 1. Silence
  test_silence_float();

  // 2. Too much noise
  test_too_much_noise_float();

  // 3. Signal + moderate noise
  test_noisy_signal_float();

  // 4. Pure sine sweeps
  test_pure_sines_float();

  // 5. Harmonics
  test_harmonics_float();

  // 6. Edge cases
  test_edge_cases();

  // 7. Real instrument samples
  test_real_samples();

  // Summary
  std::cout << "\n==========================================" << std::endl;
  std::cout << "  PASSED: " << g_tests_passed << std::endl;
  std::cout << "  FAILED: " << g_tests_failed << std::endl;
  std::cout << "==========================================" << std::endl;

  if (g_tests_failed > 0) {
    std::cerr << "\n*** " << g_tests_failed << " test(s) FAILED ***"
              << std::endl;
    return 1;
  }
  std::cout << "\nAll tests passed!" << std::endl;
  return 0;
}
