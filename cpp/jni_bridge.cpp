#include <jni.h>
#include "tuner.h"
#include <vector>

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_degastonapps_flutter_1tuner_Tuner_createTunerJNI(JNIEnv *env, jobject thiz, jint sample_rate, jint buffer_size) {
    auto* tuner = new TunerCPP(sample_rate, buffer_size);
    return reinterpret_cast<jlong>(tuner);
}

JNIEXPORT void JNICALL
Java_com_degastonapps_flutter_1tuner_Tuner_destroyTunerJNI(JNIEnv *env, jobject thiz, jlong tuner_ptr) {
    auto* tuner = reinterpret_cast<TunerCPP*>(tuner_ptr);
    delete tuner;
}

JNIEXPORT jdouble JNICALL
Java_com_degastonapps_flutter_1tuner_Tuner_findFrequencyJNI(JNIEnv *env, jobject thiz, jlong tuner_ptr, jdoubleArray audio_data) {
    auto* tuner = reinterpret_cast<TunerCPP*>(tuner_ptr);
    if (!tuner) {
        return -1.0;
    }

    jdouble *audio_data_ptr = env->GetDoubleArrayElements(audio_data, nullptr);
    int len = env->GetArrayLength(audio_data);
    std::vector<double> audio_data_vec(audio_data_ptr, audio_data_ptr + len);

    double freq = tuner->findFrequency(audio_data_vec);

    env->ReleaseDoubleArrayElements(audio_data, audio_data_ptr, 0);

    return freq;
}

}