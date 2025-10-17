#include "tuner_wrapper.h"
#include "tuner.h"
#include <vector>

struct Tuner {
    TunerCPP* instance;
};

Tuner* create_tuner(int sample_rate, int buffer_size) {
    auto* tuner = new Tuner;
    tuner->instance = new TunerCPP(sample_rate, buffer_size);
    return tuner;
}

void destroy_tuner(Tuner* tuner) {
    delete tuner->instance;
    delete tuner;
}

double find_frequency(Tuner* tuner, const double* audio_data, int data_size) {
    std::vector<double> audio_data_vec(audio_data, audio_data + data_size);
    return tuner->instance->findFrequency(audio_data_vec);
}