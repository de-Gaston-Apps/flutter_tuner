#include <jni.h>
#include "tuner.h"

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
Java_com_degastonapps_flutter_1tuner_Tuner_findFrequencyJNI(JNIEnv *env, jobject thiz, jlong tuner_ptr, jshortArray audio_data) {
    auto* tuner = reinterpret_cast<TunerCPP*>(tuner_ptr);
    if (!tuner) {
        return -1.0;
    }

    jshort *audio_data_ptr = env->GetShortArrayElements(audio_data, nullptr);
    int len = env->GetArrayLength(audio_data);

    double freq = tuner->findFrequency(reinterpret_cast<const int16_t*>(audio_data_ptr), len);

    env->ReleaseShortArrayElements(audio_data, audio_data_ptr, JNI_ABORT);

    return freq;
}

}