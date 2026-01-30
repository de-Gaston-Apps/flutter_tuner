#ifndef TUNER_WRAPPER_H
#define TUNER_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

struct Tuner;
typedef struct Tuner Tuner;

Tuner* create_tuner(int sample_rate, int buffer_size);
void destroy_tuner(Tuner* tuner);
double find_frequency(Tuner* tuner, const double* audio_data, int data_size);

#ifdef __cplusplus
}
#endif

#endif // TUNER_WRAPPER_H