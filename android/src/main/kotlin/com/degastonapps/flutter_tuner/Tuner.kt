package com.degastonapps.flutter_tuner

import android.annotation.SuppressLint
import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import android.os.Handler
import android.os.HandlerThread
import android.util.Log
import java.util.concurrent.locks.ReentrantLock
import kotlin.concurrent.withLock


class Tuner(private val callback: (Double) -> Unit) {

    private var recorder: AudioRecord? = null
    private var tunerPtr: Long = 0
    private var handlerThread: HandlerThread? = null
    private var handler: Handler? = null
    private val lock = ReentrantLock()

    private external fun createTunerJNI(sampleRate: Int, bufferSize: Int): Long
    private external fun destroyTunerJNI(tunerPtr: Long)
    private external fun findFrequencyJNI(tunerPtr: Long, audioData: ShortArray): Double

    @SuppressLint("MissingPermission")
    fun start() {
        lock.withLock {
            if (recorder != null) return
            Log.d(TAG, "start: Starting tuner")

            tunerPtr = createTunerJNI(SAMPLE_RATE, BUFFER_SIZE)
            if (tunerPtr == 0L) {
                Log.e(TAG, "Failed to create tuner instance")
                return
            }

            val audioRecord = AudioRecord(
                MediaRecorder.AudioSource.MIC,
                SAMPLE_RATE,
                RECORDER_CHANNELS,
                RECORDER_AUDIO_ENCODING,
                BUFFER_SIZE * BYTES_PER_ELEMENT * 2 // Buffer double the size for safety
            )

            if (audioRecord.state != AudioRecord.STATE_INITIALIZED) {
                Log.e(TAG, "AudioRecord initialization failed")
                destroyTunerJNI(tunerPtr)
                tunerPtr = 0
                audioRecord.release()
                return
            }

            handlerThread = HandlerThread("TunerThread")
            handlerThread?.start()
            handler = Handler(handlerThread!!.looper)

            audioRecord.setRecordPositionUpdateListener(object : AudioRecord.OnRecordPositionUpdateListener {
                override fun onMarkerReached(recorder: AudioRecord?) {}

                override fun onPeriodicNotification(recorder: AudioRecord?) {
                    processAudio()
                }
            }, handler)

            audioRecord.setPositionNotificationPeriod(BUFFER_SIZE)
            audioRecord.startRecording()
            recorder = audioRecord
        }
    }

    private fun processAudio() {
        lock.withLock {
            val currentRecorder = recorder
            val currentTunerPtr = tunerPtr
            if (currentRecorder == null || currentTunerPtr == 0L) return

            try {
                val sData = ShortArray(BUFFER_SIZE)
                val shortsRead = currentRecorder.read(sData, 0, BUFFER_SIZE)
                if (shortsRead > 0) {
                    val freq = findFrequencyJNI(currentTunerPtr, sData)
                    callback(freq)
                }
            } catch (e: Exception) {
                Log.e(TAG, "Error in processAudio: ${e.message}")
                callback(ERROR_FREQUENCY)
            }
        }
    }

    fun stop() {
        lock.withLock {
            if (recorder == null) return
            Log.d(TAG, "stop: Stopping tuner")

            try {
                recorder?.stop()
                recorder?.setRecordPositionUpdateListener(null)
                recorder?.release()
            } catch (e: Exception) {
                Log.e(TAG, "stop: Error stopping recorder", e)
            }
            recorder = null

            handlerThread?.quitSafely()
            handlerThread = null
            handler = null

            if (tunerPtr != 0L) {
                destroyTunerJNI(tunerPtr)
                tunerPtr = 0
            }
        }
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
