//
//  yinfft.h
//  Pods
//
//  Created by Jonathan de Gaston on 4/14/26.
//

#ifndef STANDALONE_YINFFT_H
#define STANDALONE_YINFFT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _pitch_yinfft_t pitch_yinfft_t;

/**
 * Creates a new YinFFT pitch detector.
 * @param samplerate Audio sample rate (e.g., 44100)
 * @param bufsize Buffer size (must be power of 2, e.g., 2048)
 */
pitch_yinfft_t *new_pitch_yinfft(int samplerate, int bufsize);

/**
 * Executes pitch detection.
 * @param p The detector instance
 * @param input Array of float samples of length 'bufsize'
 * @return Fundamental frequency in Hz, or 0.0 if no pitch detected
 */
float pitch_yinfft_do(pitch_yinfft_t *p, const float *input);

/**
 * Gets confidence of the last detection (0.0 to 1.0)
 */
float pitch_yinfft_get_confidence(pitch_yinfft_t *p);

void del_pitch_yinfft(pitch_yinfft_t *p);

#ifdef __cplusplus
}
#endif

#endif
