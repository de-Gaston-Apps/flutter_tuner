package com.degastonapps.flutter_tuner

import android.annotation.SuppressLint
import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import android.util.Log


class Tuner(val callback: (Double) -> Unit) {

    private var isRecording = false
    private var recorder: AudioRecord? = null
    private var recordingThread: Thread? = null
    private var tunerPtr: Long = 0

    private external fun createTunerJNI(sampleRate: Int, bufferSize: Int): Long
    private external fun destroyTunerJNI(tunerPtr: Long)
    private external fun findFrequencyJNI(tunerPtr: Long, audioData: DoubleArray): Double

    private fun findFrequency(sigIn: DoubleArray): Double {
        if (tunerPtr == 0L) return ERROR_FREQUENCY
        return findFrequencyJNI(tunerPtr, sigIn)
    }

    private fun collectData(): ShortArray {
        val sData = ShortArray(BUFFER_SIZE)
        val shortsRead = recorder!!.read(sData, 0, BUFFER_SIZE)
        if (shortsRead < 0) {
            Log.e(TAG, "collectData: Couldn't read from recorder! shortsRead=$shortsRead")
        }
        return sData
    }

    @SuppressLint("MissingPermission")
    fun start() {
        if (isRecording) return
        Log.d(TAG, "start: Starting tuner")

        tunerPtr = createTunerJNI(SAMPLE_RATE, BUFFER_SIZE)
        if (tunerPtr == 0L) {
            Log.e(TAG, "Failed to create tuner instance")
            return
        }

        recorder = AudioRecord(
            MediaRecorder.AudioSource.MIC,
            SAMPLE_RATE,
            RECORDER_CHANNELS,
            RECORDER_AUDIO_ENCODING,
            BUFFER_SIZE * BYTES_PER_ELEMENT
        )

        recorder?.startRecording()
        isRecording = true

        recordingThread = Thread({
            while (isRecording) {
                try {
                    val sData = collectData()
                    val doubleData = sData.map { it.toDouble() }.toDoubleArray()
                    val freq = findFrequency(doubleData)
                    callback(freq)
                } catch (e: Exception) {
                    Log.e(TAG, "Error: ${e.message}\n Stack: ${e.stackTraceToString()}")
                    callback(ERROR_FREQUENCY)
                }
            }
        }, "recordingThread")
        recordingThread?.start()
    }

    fun stop() {
        if (!isRecording) return
        Log.d(TAG, "stop: Stopping tuner")
        isRecording = false

        if (tunerPtr != 0L) {
            destroyTunerJNI(tunerPtr)
            tunerPtr = 0
        }

        try {
            recorder?.stop()
            recorder?.release()
        } catch (e: Exception) {
            Log.e(TAG, "stop: Error stopping recorder", e)
        }
        recorder = null

        try {
            recordingThread?.join()
        } catch (e: InterruptedException) {
            Log.e(TAG, "stop: Thread join interrupted", e)
        }
        recordingThread = null
    }

    companion object {
        const val TAG = "TUNER"
        const val SAMPLE_RATE = 16000
        const val RECORDER_CHANNELS = AudioFormat.CHANNEL_IN_MONO
        const val RECORDER_AUDIO_ENCODING = AudioFormat.ENCODING_PCM_16BIT
        const val ERROR_FREQUENCY = -1.0
        const val BUFFER_SIZE = 4096
        const val BYTES_PER_ELEMENT = 2

        init {
            System.loadLibrary("tuner")
        }
    }
}


/* ------------------------------------------------------------------------
   com.degastonapps.flutter_tuner.NoiseLevels object mirrors your dart noise_levels import.
   Replace values with your project's constants.
   ------------------------------------------------------------------------ */
object NoiseLevels {
    // Example value. Set to the constant you use in Dart.
    const val NOISE_SENSE_VERY_HIGH: Double = 25.0
}
