package com.degastonapps.flutter_tuner

import android.annotation.SuppressLint
import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import android.util.Log
import kotlin.math.abs
import kotlin.math.ln


class Tuner(val callback: (Double) -> Unit) {

    companion object {
        const val TAG = "TUNER"
        const val RECORDER_SAMPLE_RATE = 44100
        const val RECORDER_CHANNELS = AudioFormat.CHANNEL_IN_MONO
        const val RECORDER_AUDIO_ENCODING = AudioFormat.ENCODING_PCM_16BIT

        const val ERROR_FREQUENCY = -1.0
        const val TOO_QUIET_FREQ = -2.0;

        // Want to record at least 0.3 seconds data for accurate output of low pitch instruments
        const val BUFFER_SIZE:Int = RECORDER_SAMPLE_RATE / 3

        // 2 bytes in 16bit format
        const val BYTES_PER_ELEMENT = 2

        // Record at least 1/4 second of data
        const val MIN_SIGNAL_LEN = RECORDER_SAMPLE_RATE / 4;

        // Tuner specific values:
        const val ATTENUATION = 0.80

        // TODO: Fix the noise levels.
        // Sometimes it picks up frequencies around ~60Hz or ~120Hz
        const val NOISE_SENSE_LOW = 5;
        const val NOISE_SENSE_MED = 10;
        const val NOISE_SENSE_HIGH = 15;
        const val NOISE_SENSE_VERY_HIGH = 40;
    }


    private var isRecording = false
    private var isTuning = false
    private var recorder: AudioRecord? = null
    private var recordingThread: Thread? = null


    private fun collectData(): ShortArray {
        // Write the input audio in byte array
        val sData = ShortArray(BUFFER_SIZE)
        // Get the voice input from microphone
        val shortsRead = recorder!!.read(sData, 0, BUFFER_SIZE)

        if (shortsRead < 0) {
//            Log.e(TAG, "collectData: Couldn't read from recorder! shortsRead=$shortsRead")
        }

        return sData
    }


    private fun findFrequency(sData: ShortArray): Double {
        if (sData.size < BUFFER_SIZE) {
//            Log.d(TAG, "findFreq: BufferSize too small")
            return ERROR_FREQUENCY
        }
        val startIndex = 1000
        val windowSize = 1200

        //take every value in the window and square it and add to autoXSum
        var autoXSum = 0.0
        for (j in startIndex until startIndex + windowSize) autoXSum += (sData[j] * sData[j]).toDouble()

        //if the average piece of data was less than 4.0
//        Log.d(TAG, "AutoXSum FOUND: $autoXSum")
        if (autoXSum < 4 * windowSize) {
//            Log.d(TAG, "Too Quiet $autoXSum")
            return ERROR_FREQUENCY
        }
        var xSum: Double
        var foundPeak = false
        var peakIndex = -1
        var peakHeight = -1.0
        run {
            var k = 30
            while (k < 1102) {
                if (k > 400) k++ //speed things up a bit after
                xSum = 0.0
                for (j in startIndex until startIndex + windowSize) {
                    xSum += (sData[j] * sData[j + k]).toDouble()
                }
                if (foundPeak) {
                    if (xSum > peakHeight) {
//                        Log.d(TAG, "Higher Peak: $xSum")
                        peakIndex = k
                        peakHeight = xSum
                    } else {
                        break
                    }
                } else {  // !foundPeak
                    if (xSum * 1.0 > autoXSum * ATTENUATION) {
                        peakIndex = k
                        peakHeight = xSum
                        foundPeak = true
//                        Log.d(TAG, "Found Peak: $xSum")
                    } else {
//                        Log.d(TAG,"Did Not Find Peak Value: $k: $xSum:$autoXSum")
                        if (xSum < 0) k += 2
                    }
                }
                k++
            }
        }
        if (!foundPeak) {
//            Log.d(TAG, "findFreqCorr: No peak found")
            return ERROR_FREQUENCY
        }
//        Log.d(TAG,"PeakFreq W/O Precision: " + 1.0 * RECORDER_SAMPLE_RATE / peakIndex)
        var precisionPeak = peakIndex
        var precision = 1
        if (peakIndex < 350) {
            precision = 20
            precisionPeak = peakIndex * precision
        } else if (peakIndex < 700) {
            precision = 10
            precisionPeak = peakIndex * precision
        } else if (peakIndex < 1400) {
            precision = 5
            precisionPeak = peakIndex * precision
        } else if (peakIndex < 3500) {
            precision = 2
            precisionPeak = peakIndex * precision
        } else {
            //return (SR *1.0) / peakIndex;
        }
        var pXSum: Double
        var max = 0.0
        val searchSpan = 2 + precision
        val pStart = precisionPeak - searchSpan
        val pStop = precisionPeak + searchSpan
        for (k in pStart until pStop) {
            pXSum = 0.0
            for (j in startIndex until startIndex + windowSize) {
                pXSum += (sData[j] * sData[j + k] / 512).toDouble()
            }
            if (pXSum > max) {
                max = pXSum
                precisionPeak = k
            }
        }
        return 1.0 * RECORDER_SAMPLE_RATE * precision / precisionPeak
    }


    /**
     * Check for permissions BEFORE calling this function
     */
    @SuppressLint("MissingPermission")
    fun start() {
        if (isRecording) return // Already running, ignore

//        Log.d(TAG, "start: Starting tuner")

        recorder = AudioRecord(
            MediaRecorder.AudioSource.MIC,
            RECORDER_SAMPLE_RATE,
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
                    val freq = findFrequency(sData)
                    callback(freq)
                } catch (e: Exception) {
//                    Log.e(TAG, "Error: ${e.message}\n Stack: ${e.stackTraceToString()}")
                    callback(ERROR_FREQUENCY)
                }
            }
        }, "recordingThread")

        recordingThread?.start()
    }

    fun stop() {
        if (!isRecording) return // Already stopped, ignore

//        Log.d(TAG, "stop: Stopping tuner")
        isTuning = false
        isRecording = false

        try {
            recorder?.stop()
            recorder?.release()
        } catch (e: Exception) {
//            Log.e(TAG, "stop: Error stopping recorder", e)
        }

        recorder = null

        try {
            recordingThread?.join()
        } catch (e: InterruptedException) {
//            Log.e(TAG, "stop: Thread join interrupted", e)
        }

        recordingThread = null // Clear so a new one can be created
    }

}
