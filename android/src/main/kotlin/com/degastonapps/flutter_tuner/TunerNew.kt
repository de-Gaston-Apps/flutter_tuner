package com.degastonapps.flutter_tuner

import android.annotation.SuppressLint
import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import android.util.Log
import kotlin.math.abs
import kotlin.math.max


class TunerNew(val callback: (Double) -> Unit) {

    companion object {
        const val TAG = "TUNER"
        const val SAMPLE_RATE = 16000
        const val RECORDER_CHANNELS = AudioFormat.CHANNEL_IN_MONO
        const val RECORDER_AUDIO_ENCODING = AudioFormat.ENCODING_PCM_16BIT

        const val ERROR_FREQUENCY = -1.0
        const val TOO_QUIET_FREQ = -2.0;

        // Want to record at least 0.25 seconds data for accurate output of low pitch instruments
        const val BUFFER_SIZE = 4096

        // 2 bytes in 16bit PCM format
        const val BYTES_PER_ELEMENT = 2

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


    private var prevFreq = ERROR_FREQUENCY
    private var prevFreq2 = ERROR_FREQUENCY

    private fun debounceFrequency(freq: Double): Double {
        val average = (prevFreq + prevFreq2 + freq) / 3.0
        val diffPrev = abs(prevFreq - average)
        val diffPrev2 = abs(prevFreq2 - average)
        val diffFreq = abs(freq - average)

        val freqToReturn = when {
            diffPrev <= diffPrev2 && diffPrev <= diffFreq -> prevFreq
            diffPrev2 <= diffPrev && diffPrev2 <= diffFreq -> prevFreq2
            else -> freq
        }

        prevFreq2 = prevFreq
        prevFreq = freq
        return freqToReturn
    }

    private fun collectData(): ShortArray {
        // Write the input audio in byte array
        val sData = ShortArray(BUFFER_SIZE)

        // Get the sound input from microphone
        val shortsRead = recorder!!.read(sData, 0, BUFFER_SIZE)

        if (shortsRead < 0) {
            Log.e(TAG, "collectData: Couldn't read from recorder! shortsRead=$shortsRead")
        }

        return sData
    }

    /**
     * Main frequency estimation method.
     */
    private fun findFrequency(sigIn: DoubleArray, noiseFloor: Double = NoiseLevels.NOISE_SENSE_VERY_HIGH): Double {
        val sig = sigIn.copyOf()
        if (sig.size < BUFFER_SIZE) {
            Log.e(TAG, "Not enough samples! Expecting > $BUFFER_SIZE. Got ${sig.size}")
            return ERROR_FREQUENCY
        }

        // Split into overlapping windows (Welch-style)
        val factor = 4 // must be multiple of 4 as in Dart
        val windowLength = sig.size / factor
        var startIndex = 0
        var windowsCalculated = 0

        // powers array length = windowLength/2 + 1
        val powers = DoubleArray(windowLength / 2 + 1)

        val ham = hamming(windowLength)

        // Perform factor*2 - 1 windows (overlap by half-window)
        while (windowsCalculated != factor * 2 - 1) {
            val windowed = DoubleArray(windowLength)
            for (i in 0 until windowLength) {
                windowed[i] = sig[startIndex + i] * ham[i]
            }

            val f = rfft(windowed) // Real FFT result
            val fAbs = arrayComplexAbs(f) // DoubleArray

            // powers += fAbs element-wise
            for (i in powers.indices) {
                powers[i] += fAbs.getOrElse(i) { 0.0 }
            }

            startIndex += windowLength / 2
            windowsCalculated++
        }

        var foundSignal = false
        val avgPower = mean(powers)

        // Find top NUM_PEAKS peaks
        val peaks = mutableListOf<Int>()
        val NUM_PEAKS = 5
        val START_FFT_INDEX = 2
        for (i in 0 until NUM_PEAKS) {
            var tmpMax = -1.0
            var tmpIdx = -1
            for (j in START_FFT_INDEX until powers.size) {
                if (powers[j] > tmpMax && !peaks.contains(j)) {
                    tmpIdx = j
                    tmpMax = powers[j]
                    if (tmpMax > avgPower * noiseFloor) {
                        foundSignal = true
                    }
                }
            }
            peaks.add(tmpIdx)
        }

        if (!foundSignal) {
            Log.d(TAG, "TOO QUIET! avgPower=$avgPower")
            return TOO_QUIET_FREQ
        }

        val maxIndex = scorePowers(powers, peaks)

        // Parabolic interpolation for sub-bin accuracy
        val logPowers = arrayLog(powers)
        val par = parabolic(logPowers, peaks[maxIndex])
        val trueI = par[0]

        val newFreq = SAMPLE_RATE * trueI / windowLength
        if (newFreq < 0.0) return ERROR_FREQUENCY

        return newFreq
    }

    private fun scorePowers(powers: DoubleArray, peaks: List<Int>): Int {
        // Harmonic Product Spectrum-inspired scoring
        val sums = mutableListOf<Double>()
        val factors = listOf(2.0, 3.0, 4.0)

        for (peak in peaks) {
            var sum = 0.0
            if (peak <= 0) {
                sums.add(sum)
                continue
            }

            for (fFactor in factors) {
                var newIndex = (peak * fFactor).toInt()
                if (newIndex < 2) newIndex = 2
                if (newIndex + 3 > powers.size) newIndex = powers.size - 3

                // include neighbors
//                sum += powers.getOrElse(newIndex - 2) { 0.0 } * powers[peak]
                sum += powers.getOrElse(newIndex - 1) { 0.0 } * powers[peak]
                sum += powers.getOrElse(newIndex) { 0.0 } * powers[peak]
                sum += powers.getOrElse(newIndex + 1) { 0.0 } * powers[peak]
//                sum += powers.getOrElse(newIndex + 2) { 0.0 } * powers[peak]
            }
            sums.add(sum)
        }

        var maxIndex = 0
        for (i in 1 until sums.size) {
            if (sums[i] > sums[maxIndex]) {
                maxIndex = i
            }
        }

        return maxIndex
    }


    /**
     * Check for permissions BEFORE calling this function
     */
    @SuppressLint("MissingPermission")
    fun start() {
        if (isRecording) return // Already running, ignore

        Log.d(TAG, "start: Starting tuner")

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
        if (!isRecording) return // Already stopped, ignore

        Log.d(TAG, "stop: Stopping tuner")
        isTuning = false
        isRecording = false

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

        recordingThread = null // Clear so a new one can be created
    }

}

/* ------------------------------------------------------------------------
   DSP helper placeholders. Implement these with a fast native/DSP library
   (JTransforms, FFTW wrapper, kissfft, or your own SIMD code).
   ------------------------------------------------------------------------ */
data class Complex(val re: Double, val im: Double) {
    operator fun plus(other: Complex) = Complex(re + other.re, im + other.im)
    operator fun minus(other: Complex) = Complex(re - other.re, im - other.im)
    operator fun times(other: Complex) =
        Complex(re * other.re - im * other.im, re * other.im + im * other.re)
    fun scale(f: Double) = Complex(re * f, im * f)
}

/**
 * Window function.
 * Returns length-sized Hamming window.
 */
fun hamming(length: Int): DoubleArray {
    val out = DoubleArray(length)
    for (n in 0 until length) {
        out[n] = 0.54 - 0.46 * kotlin.math.cos(2.0 * Math.PI * n / (length - 1))
    }
    return out
}


/**
 * Recursive Cooley–Tukey FFT
 */
private fun fft(input: Array<Complex>): Array<Complex> {
    val n = input.size
    if (n == 1) return arrayOf(input[0])

    if (n % 2 != 0) throw IllegalArgumentException("FFT input length must be power of 2")

    val even = fft(Array(n / 2) { input[2 * it] })
    val odd = fft(Array(n / 2) { input[2 * it + 1] })

    val out = Array(n) { Complex(0.0, 0.0) }
    for (k in 0 until n / 2) {
        val angle = -2.0 * Math.PI * k / n
        val wk = Complex(Math.cos(angle), Math.sin(angle)) * odd[k]
        out[k] = even[k] + wk
        out[k + n / 2] = even[k] - wk
    }
    return out
}

/**
 * Real FFT. Returns bins of length = windowLength/2 + 1
 */
fun rfft(windowed: DoubleArray): Array<Complex> {
    val n = windowed.size
    if (n and (n - 1) != 0) throw IllegalArgumentException("Input length must be power of 2")

    val complexInput = Array(n) { Complex(windowed[it], 0.0) }
    val spectrum = fft(complexInput)

    // Only keep positive frequencies [0..N/2]
    return spectrum.copyOfRange(0, n / 2 + 1)
}


/**
 * Compute magnitudes of complex array
 */
fun arrayComplexAbs(c: Array<Complex>): DoubleArray {
    return DoubleArray(c.size) { i ->
        kotlin.math.hypot(c[i].re, c[i].im)
    }
}

/**
 * Mean of an array
 */
fun mean(arr: DoubleArray): Double {
    if (arr.isEmpty()) return 0.0
    var sum = 0.0
    for (v in arr) sum += v
    return sum / arr.size
}

/**
 * Natural log of array elements. Returns new DoubleArray.
 */
fun arrayLog(arr: DoubleArray): DoubleArray {
    return DoubleArray(arr.size) { i ->
        kotlin.math.ln(max(arr[i], Double.MIN_VALUE))
    }
}

/**
 * Parabolic interpolation around index `i` on array `y` (log-powers).
 * Returns DoubleArray where index 0 = true_i (interpolated peak index),
 * index 1 = interpolated value (optional).
 *
 * Implementation expects `i` to be a valid index within y.
 * TODO: you can replace with a version that returns a richer structure.
 */
fun parabolic(y: DoubleArray, i: Int): DoubleArray {
    // i should have neighbors; clamp to safe range
    val idx = i.coerceIn(1, y.size - 2)
    val alpha = y[idx - 1]
    val beta = y[idx]
    val gamma = y[idx + 1]
    val p = 0.5 * (alpha - gamma) / (alpha - 2.0 * beta + gamma)
    val trueI = idx + p
    val value = beta - 0.25 * (alpha - gamma) * p
    return doubleArrayOf(trueI, value)
}

/* ------------------------------------------------------------------------
   com.degastonapps.flutter_tuner.NoiseLevels object mirrors your dart noise_levels import.
   Replace values with your project's constants.
   ------------------------------------------------------------------------ */
object NoiseLevels {
    // Example value. Set to the constant you use in Dart.
    const val NOISE_SENSE_VERY_HIGH: Double = 25.0
}
