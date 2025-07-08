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

    private var prevFreq = ERROR_FREQUENCY
    private var prevFreq2 = ERROR_FREQUENCY

    private fun debounceFrequency(freq: Double): Double {
        val average = (prevFreq + prevFreq2 + freq) / 3
        val diffPrev = abs(prevFreq - average)
        val diffPrev2 = abs(prevFreq2 - average)
        val diffFreq = abs(freq - average)

        var freqToReturn = ERROR_FREQUENCY
        if (diffPrev <= diffPrev2 && diffPrev <= diffFreq) {
            freqToReturn = prevFreq
        } else if (diffPrev2 <= diffPrev && diffPrev2 <= diffFreq) {
            freqToReturn = prevFreq2
        } else {
            freqToReturn = freq
        }

        prevFreq2 = prevFreq
        prevFreq = freq
        return freqToReturn
    }

    private fun findFrequencyNew(sData: ShortArray): Double {
        return try {
            val freq = findFundFreqWelchHPS(sData)
            debounceFrequency(freq)
        } catch (e: Exception) {
//            Log.e(TAG, "Error: ${e.message}\n Stack: ${e.stackTraceToString()}")
            ERROR_FREQUENCY
        }
    }

    /**
     * Estimates SNR
     */
    private fun hasSignal(data: ShortArray, windowSize: Int): Boolean {
        val startIndex = 1000

        // take every value in the window and square it and add to autoXSum
        var autoXSum = 0.0
        for (j in startIndex until startIndex + windowSize) {
            autoXSum += (data[j] * data[j]).toDouble()
        }

//        Log.d(TAG, "hasSignal: autoXSum = $autoXSum")

        return autoXSum > 4 * windowSize
    }

    /**
     * Computes autocorrelation powers and top N peaks for a signal segment.
     *
     * @param sData The input signal as DoubleArray.
     * @param startIndex The starting index of the analysis window.
     * @param windowSize The size of the window to analyze.
     * @param maxLag The maximum lag to consider in autocorrelation.
     * @param numPeaks Number of top peaks to return.
     * @return Pair of (peaks: List<Int>, powers: List<Double>)
     */
    private fun autocorrPowersAndPeaks(
        sData: ShortArray,
        startIndex: Int,
        windowSize: Int,
        numPeaks: Int = 5
    ): Pair<List<Int>, List<Double>> {
        val powers = MutableList(sData.size) { 0.0 }

        // Compute autocorrelation power for each lag
        for (k in sData.indices) {
            var sum = 0.0
            val limit = startIndex + windowSize
            for (i in startIndex until limit) {
                val j = i + k
                if (j >= sData.size) break
                sum += sData[i] * sData[j]
            }
            powers[k] = sum
        }

        // Find local maxima for peak detection
        val peaks = powers
            .asSequence()
            .withIndex()                     // Pair each value with its index
            .sortedByDescending { it.value } // Sort by value, descending
            .take(numPeaks)                         // Take top 5
            .map { it.index }               // Extract the indices
            .toMutableList()


        return Pair(peaks, powers)
    }


    // Computes the natural logarithm of each element in the list
    private fun arrayLog(a: List<Double>): List<Double> {
        return a.map { ln(it) }
    }


    // Performs parabolic interpolation around index x in list y
    // Returns Pair(xv, yv) where xv is the interpolated x (fractional index) and yv the interpolated y value
    private fun parabolic(y: List<Double>, x: Int): Pair<Double, Double> {
        require(x > 0 && x < y.lastIndex) { "x must be within the valid range (1..y.size-2)" }
        val numerator = y[x - 1] - y[x + 1]
        val denominator = y[x - 1] - 2 * y[x] + y[x + 1]
        val xv = (0.5 * numerator / denominator) + x
        val yv = y[x] - 0.25 * numerator * (xv - x)
        return Pair(xv, yv)
    }


    /// Estimate frequency from FFT using the Welch method
    /// and "Harmonic Product Spectrum Theory"
    /// [sig] should be of length (multiple of 2)
    private fun findFundFreqWelchHPS(
        signal: ShortArray,
        noiseFloor: Int = NOISE_SENSE_VERY_HIGH
    ): Double {

        // Benchmark on an old Samsung phone took 45-50ms on average
//         val start = System.nanoTime()


        // Check that there are enough samples
        if (signal.size < MIN_SIGNAL_LEN) {
//            Log.e(TAG, "Not enough samples! Expecting > $MIN_SIGNAL_LEN. Got ${signal.size}")
            return ERROR_FREQUENCY
        }

        // TODO: Speed up the alg by checking that the signal is a power of 2?
        // if (!isPowerOf2(sig.length)) {
        //     logger.d("WARNING: signal length is not a power of 2")
        // }

        val windowLength = 1200

        // Get the signal peaks and powers at every element
        val (peaks, powers) = autocorrPowersAndPeaks(signal, 2, windowLength)


//        Log.d(TAG, "Peaks = $peaks")


        // Do some "Harmonic Product Spectrum Theory"
        val sums = mutableListOf<Double>()
        val factors = listOf(2.0, 3.0, 4.0)
        for (peak in peaks) {
            var sum = 0.0
            if (peak == 0) {
                sums.add(sum)
                continue
            }
            for (f in factors) {
                var newIndex = (peak * f).toInt()
                if (newIndex < 2) newIndex = 2
                if (newIndex + 3 > powers.size) newIndex = powers.size - 3
                sum += powers[newIndex - 2] * powers[peak]
                sum += powers[newIndex - 1] * powers[peak]
                sum += powers[newIndex + 0] * powers[peak]
                sum += powers[newIndex + 1] * powers[peak]
                sum += powers[newIndex + 2] * powers[peak]
            }
            sums.add(sum)
        }


        var maxIndex = 0
        for (i in 1 until sums.size) {
            if (sums[i] > sums[maxIndex]) {
                maxIndex = i
            }
        }


        // Filter out signals that are too quiet to accurately process.
        if (powers[maxIndex] < powers.average() * noiseFloor) {
            return TOO_QUIET_FREQ
        }


        val trueIndex = parabolic(arrayLog(powers), peaks[maxIndex]).first
        val newFreq = RECORDER_SAMPLE_RATE / trueIndex
        if (newFreq < 0) return ERROR_FREQUENCY


//        val end = System.nanoTime()
//        val durationMs = (end - start) / 1_000_000.0
//        Log.d(TAG, "Took $durationMs ms")

//        Log.d(TAG, "Old frequency would be =${RECORDER_SAMPLE_RATE / maxIndex}")
        return newFreq
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
                    // TODO: Make the new alg work
                    // val freq = findFrequencyNew(sData)
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
