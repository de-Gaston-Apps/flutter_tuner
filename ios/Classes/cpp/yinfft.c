//
//  yinfft.c
//  Pods
//
//  Created by Jonathan de Gaston on 4/14/26.
//
#include "yinfft.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846

static int is_power_of_two_and_valid(int n) {
    return (n >= 4) && ((n & (n - 1)) == 0);
}

struct _pitch_yinfft_t {
    int samplerate;
    int bufsize;
    float *win;
    float *winput;
    float *sqrmag;
    float *weight;
    float *yinfft;
    float *fft_real;
    float *fft_imag;
    float tol;
    int peak_pos;
    int short_period;
};

// --- Psychoacoustic Weighting Constants from your source ---
//       DON'T FORGET THE SENTINEL VALUE '-1' at the end
static const float freqs_map[] = {0., 20., 25., 31.5, 40., 50., 63., 80., 100., 125., 160., 200., 250., 315., 400., 500., 630., 800., 1000., 1250., 1600., 2000., 2500., 3150., 4000., 5000., 6300., 8000., 9000., 10000., 12500., 15000., 20000., 25100., -1.};
static const float weight_map[] = {-75.8, -70.1, -60.8, -52.1, -44.2, -37.5, -31.3, -25.6, -20.9, -16.5, -12.6, -9.60, -7.00, -4.70, -3.00, -1.80, -0.80, -0.20, -0.00, 0.50, 1.60, 3.20, 5.40, 7.80, 8.10, 5.30, -2.40, -11.1, -12.8, -12.2, -7.40, -17.8, -17.8, -17.8};

//static const float freqs_violin[] = {0., 100., 150., 196., 500., 1000., 3000., 5000., 10000., -1.};
//static const float weight_violin[] = {-90.0, -60.0, -20.0, 0.0, 2.0, 5.0, 8.0, 2.0, -20.0};
//static const float freqs_guitar[] = {0., 40., 80., 200., 1000., 2000., 5000., 10000., -1.};
//static const float weight_guitar[] = {-90.0, -40.0, 0.0, 2.0, 5.0, 5.0, -10.0, -30.0};
//static const float freqs_ukulele[] = {0., 200., 350., 1000., 3000., 5000., 10000., -1.};
//static const float weight_ukulele[] = {-90.0, -50.0, 0.0, 5.0, 5.0, 0.0, -40.0};
//static const float freqs_bass[] = {0., 25., 31., 100., 500., 1000., 5000., -1.};
//static const float weight_bass[] = {-20.0, 0.0, 5.0, 5.0, 0.0, -20.0, -60.0};
//static const float freqs_cello[] = {0., 50., 65., 200., 1000., 4000., -1.};
//static const float weight_cello[] = {-60.0, -10.0, 0.0, 5.0, 5.0, -20.0};


// --- Basic Radix-2 FFT Implementation to replace aubio_fft ---
static void simple_fft(float *real, float *imag, int n) {
    int i, j, k, i1, l, l1, l2;
    float c1, c2, tx, ty, t1, t2, u1, u2;
    j = 0;
    for (i = 0; i < n - 1; i++) {
        if (i < j) {
            tx = real[i]; ty = imag[i];
            real[i] = real[j]; imag[i] = imag[j];
            real[j] = tx; imag[j] = ty;
        }
        k = n >> 1;
        while (k <= j) { j -= k; k >>= 1; }
        j += k;
    }
    l2 = 1;
    for (l = 0; l < (int)(log(n)/log(2)); l++) {
        l1 = l2; l2 <<= 1; u1 = 1.0; u2 = 0.0;
        c1 = cos(-PI / l1); c2 = sin(-PI / l1);
        for (j = 0; j < l1; j++) {
            for (i = j; i < n; i += l2) {
                i1 = i + l1;
                t1 = u1 * real[i1] - u2 * imag[i1];
                t2 = u1 * imag[i1] + u2 * real[i1];
                real[i1] = real[i] - t1; imag[i1] = imag[i] - t2;
                real[i] += t1; imag[i] += t2;
            }
            u1 = (t1 = u1) * c1 - u2 * c2; u2 = t1 * c2 + u2 * c1;
        }
    }
}

pitch_yinfft_t *new_pitch_yinfft(int samplerate, int bufsize) {
    // Make sure samplerate is valid
    if (samplerate <= 0) return NULL;
    
    // Validate bufsize is a power of 2 and at least 4
    if (!is_power_of_two_and_valid(bufsize)) {
        return NULL;
    }
    
    // Check the struct allocation before dereferencing
    pitch_yinfft_t *p = (pitch_yinfft_t *)calloc(1, sizeof(pitch_yinfft_t));
    if (!p) return NULL;
    
    p->samplerate = samplerate;
    p->bufsize = bufsize;
    p->winput = (float *)calloc(bufsize, sizeof(float));
    p->sqrmag = (float *)calloc(bufsize, sizeof(float));
    p->yinfft = (float *)calloc(bufsize / 2 + 1, sizeof(float));
    p->fft_real = (float *)calloc(bufsize, sizeof(float));
    p->fft_imag = (float *)calloc(bufsize, sizeof(float));
    p->win = (float *)calloc(bufsize, sizeof(float));
    p->weight = (float *)calloc(bufsize / 2 + 1, sizeof(float));
    
    // Check for partial allocation and cleanup if necessary
    if (!p->winput || !p->sqrmag || !p->yinfft ||
        !p->fft_real || !p->fft_imag || !p->win || !p->weight) {
        del_pitch_yinfft(p);
        return NULL;
    }
    
    p->tol = 0.85f;

    // Hanning Window
    for (int i = 0; i < bufsize; i++) p->win[i] = 0.5 * (1 - cos(2 * PI * i / (bufsize - 1)));

    // Spectral Weighting (Psychoacoustic)
    int j = 1;
    for (int i = 0; i < bufsize / 2 + 1; i++) {
        float freq = (float)i / (float)bufsize * (float)samplerate;
        while (freq > freqs_map[j] && freqs_map[j] > 0) j++;
        float f0 = freqs_map[j-1], f1 = freqs_map[j];
        float a0 = weight_map[j-1], a1 = weight_map[j];
        if (f0 == f1) p->weight[i] = a0;
        else p->weight[i] = (a1 - a0) / (f1 - f0) * (freq - f0) + a0;
        p->weight[i] = pow(10, p->weight[i] / 20.0f); // DB2LIN
    }
    p->short_period = (int)(samplerate / 1300.0);
    return p;
}

float pitch_yinfft_do(pitch_yinfft_t *p, const float *input) {
    if (!p || !input) return 0.0f;
    
    int n = p->bufsize;
    float sum = 0, tmp = 0;

    // Windowing and FFT
    for (int i = 0; i < n; i++) {
        p->fft_real[i] = input[i] * p->win[i];
        p->fft_imag[i] = 0;
    }
    simple_fft(p->fft_real, p->fft_imag, n);

    // Squared Magnitude + Weighting
    for (int l = 0; l < n / 2 + 1; l++) {
        float mag = (p->fft_real[l] * p->fft_real[l]) + (p->fft_imag[l] * p->fft_imag[l]);
        p->sqrmag[l] = mag * p->weight[l];
        if (l > 0 && l < n / 2) p->sqrmag[n - l] = p->sqrmag[l];
        sum += p->sqrmag[l];
    }
    sum *= 2.0f;

    // YIN-FFT Correlation (IFFT of squared magnitude)
    for (int i = 0; i < n; i++) {
        p->fft_real[i] = p->sqrmag[i];
        p->fft_imag[i] = 0;
    }
    simple_fft(p->fft_real, p->fft_imag, n);

    p->yinfft[0] = 1.0f;
    int best_tau = 0;
    float min_val = 1000.0f;

    for (int tau = 1; tau < n / 2 + 1; tau++) {
        p->yinfft[tau] = sum - p->fft_real[tau];
        tmp += p->yinfft[tau];
        if (tmp != 0) p->yinfft[tau] *= (float)tau / tmp;
        else p->yinfft[tau] = 1.0f;

        if (p->yinfft[tau] < min_val) {
            min_val = p->yinfft[tau];
            best_tau = tau;
        }
    }

    p->peak_pos = best_tau;
    if (min_val < p->tol && best_tau > 1 && best_tau < n/2) {
        // Simple quadratic interpolation for sub-sample accuracy
        float s0 = p->yinfft[best_tau-1], s1 = p->yinfft[best_tau], s2 = p->yinfft[best_tau+1];
        // Check divide by 0 errors
        float denom = 2.0f * (2.0f * s1 - s0 - s2);
        float adj = (denom != 0.0f) ? (s2 - s0) / denom : 0.0f;
        return (float)p->samplerate / (best_tau + adj);
    }
    return 0.0f;
}

float pitch_yinfft_get_confidence(pitch_yinfft_t *p) {
    if (!p || p->peak_pos == 0) return 0.0f;
    return 1.0f - p->yinfft[p->peak_pos];
}

void del_pitch_yinfft(pitch_yinfft_t *p) {
    if (!p) return;
    free(p->win); free(p->winput); free(p->sqrmag); free(p->yinfft);
    free(p->fft_real); free(p->fft_imag); free(p->weight);
    free(p);
}
