import Foundation
import AVFoundation
import Accelerate

class TunerNew {

    // MARK: - Constants

    private static let sampleRate = 16000.0
    private static let bufferSize = 4096

    private static let errorFrequency = -1.0
    private static let tooQuietFreq = -2.0

    // This value is taken from the Android implementation's NoiseLevels object.
    private static let noiseSenseVeryHigh = 25.0

    // MARK: - Properties

    private let callback: (Double) -> Void
    private var audioEngine = AVAudioEngine()
    private var isRecording = false

    // MARK: - Initialization

    init(callback: @escaping (Double) -> Void) {
        self.callback = callback
    }

    // MARK: - Public Methods

    func start() throws {
        guard !isRecording else { return }

        // 1. Configure the audio session.
        let session = AVAudioSession.sharedInstance()
        try session.setCategory(.playAndRecord, mode: .measurement, options: .duckOthers)
        try session.setPreferredSampleRate(TunerNew.sampleRate)
        try session.setActive(true)

        // 2. Get the input node and its actual output format.
        let inputNode = audioEngine.inputNode
        let recordingFormat = inputNode.outputFormat(forBus: 0)

        // 3. Install a tap to process the audio buffer.
        inputNode.installTap(onBus: 0, bufferSize: AVAudioFrameCount(TunerNew.bufferSize), format: recordingFormat) { (buffer, time) in
            guard self.isRecording, let channelData = buffer.floatChannelData else { return }

            let frameLength = Int(buffer.frameLength)
            let samples = Array(UnsafeBufferPointer(start: channelData[0], count: frameLength))

            let doubleData = samples.map { Double($0) }
            let frequency = self.findFrequency(sigIn: doubleData)

            self.callback(frequency)
        }

        // 4. Prepare and start the audio engine.
        audioEngine.prepare()
        try audioEngine.start()
        isRecording = true
    }

    func stop() {
        guard isRecording else { return }
        isRecording = false
        audioEngine.stop()
        audioEngine.inputNode.removeTap(onBus: 0)

        do {
            try AVAudioSession.sharedInstance().setActive(false)
        } catch {
            print("Error deactivating audio session: \(error.localizedDescription)")
        }
    }

    // MARK: - Private DSP Methods

    private func findFrequency(sigIn: [Double], noiseFloor: Double = TunerNew.noiseSenseVeryHigh) -> Double {
        if sigIn.count < TunerNew.bufferSize {
            return TunerNew.errorFrequency
        }

        let factor = 4
        let windowLength = sigIn.count / factor
        var startIndex = 0
        var windowsCalculated = 0

        var powers = [Double](repeating: 0.0, count: windowLength / 2 + 1)
        let ham = hamming(length: windowLength)

        while windowsCalculated != factor * 2 - 1 {
            let windowed = (0..<windowLength).map { sigIn[startIndex + $0] * ham[$0] }

            if let f = rfft(windowed), f.count > 0 {
                let fAbs = arrayComplexAbs(f)
                for i in 0..<min(powers.count, fAbs.count) {
                    powers[i] += fAbs[i]
                }
            }

            startIndex += windowLength / 2
            windowsCalculated += 1
        }

        var foundSignal = false
        let avgPower = mean(arr: powers)

        var peaks: [Int] = []
        let numPeaks = 5
        let startFFTIndex = 2
        for _ in 0..<numPeaks {
            var tmpMax = -1.0
            var tmpIdx = -1
            for j in startFFTIndex..<powers.count {
                if powers[j] > tmpMax && !peaks.contains(j) {
                    tmpIdx = j
                    tmpMax = powers[j]
                    if tmpMax > avgPower * noiseFloor {
                        foundSignal = true
                    }
                }
            }
            if tmpIdx != -1 {
                peaks.append(tmpIdx)
            }
        }

        if !foundSignal || peaks.isEmpty {
            return TunerNew.tooQuietFreq
        }

        let maxIndex = scorePowers(powers: powers, peaks: peaks)

        let logPowers = arrayLog(arr: powers)
        let par = parabolic(y: logPowers, i: peaks[maxIndex])
        let trueI = par[0]

        let actualSampleRate = audioEngine.inputNode.outputFormat(forBus: 0).sampleRate
        let newFreq = actualSampleRate * trueI / Double(windowLength)
        return newFreq < 0.0 ? TunerNew.errorFrequency : newFreq
    }

    private func scorePowers(powers: [Double], peaks: [Int]) -> Int {
        var sums: [Double] = []
        let factors = [2.0, 3.0, 4.0]

        for peak in peaks {
            var sum = 0.0
            if peak <= 0 {
                sums.append(sum)
                continue
            }

            for fFactor in factors {
                var newIndex = Int(Double(peak) * fFactor)
                if newIndex < 2 { newIndex = 2 }
                if newIndex + 2 >= powers.count { newIndex = powers.count - 3 }

                sum += powers[newIndex - 1] * powers[peak]
                sum += powers[newIndex] * powers[peak]
                sum += powers[newIndex + 1] * powers[peak]
            }
            sums.append(sum)
        }

        var maxIndex = 0
        if let maxVal = sums.max(), let firstIndex = sums.firstIndex(of: maxVal) {
            maxIndex = firstIndex
        }

        return maxIndex
    }

    // MARK: - DSP Helper Functions

    private func hamming(length: Int) -> [Double] {
        return vDSP.window(ofType: Double.self, usingSequence: .hamming, count: length, isHalfWindow: false)
    }

    private func rfft(_ x: [Double]) -> [DSPDoubleComplex]? {
        let n = x.count
        guard n > 0, (n & (n - 1)) == 0 else {
            return nil
        }

        let log2n = vDSP_Length(log2(Double(n)))
        guard let fftSetup = vDSP_create_fftsetupD(log2n, FFTRadix(kFFTRadix2)) else {
            return nil
        }
        defer { vDSP_destroy_fftsetupD(fftSetup) }

        var real = [Double](repeating: 0, count: n / 2)
        var imag = [Double](repeating: 0, count: n / 2)

        x.withUnsafeBufferPointer { bufferPtr in
            var complex = DSPDoubleSplitComplex(realp: &real, imagp: &imag)
            vDSP_ctozD(bufferPtr.baseAddress!.withMemoryRebound(to: DSPDoubleComplex.self, capacity: x.count/2) { $0 }, 2, &complex, 1, vDSP_Length(n / 2))
        }

        var complex = DSPDoubleSplitComplex(realp: &real, imagp: &imag)
        vDSP_fft_zripD(fftSetup, &complex, 1, log2n, FFTDirection(kFFTDirection_Forward))

        let dc = complex.realp[0]
        let nyquist = complex.imagp[0]

        var result: [DSPDoubleComplex] = [DSPDoubleComplex(real: dc, imag: 0.0)]
        for i in 1..<(n/2) {
            result.append(DSPDoubleComplex(real: complex.realp[i], imag: complex.imagp[i]))
        }
        result.append(DSPDoubleComplex(real: nyquist, imag: 0.0))

        return result
    }

    private func arrayComplexAbs(_ c: [DSPDoubleComplex]) -> [Double] {
        var real = c.map { $0.real }
        var imag = c.map { $0.imag }
        var mags = [Double](repeating: 0.0, count: c.count)

        var splitComplex = DSPDoubleSplitComplex(realp: &real, imagp: &imag)
        vDSP_zvabsD(&splitComplex, 1, &mags, 1, vDSP_Length(c.count))

        return mags
    }

    private func mean(arr: [Double]) -> Double {
        if arr.isEmpty { return 0.0 }
        return vDSP.mean(arr)
    }

    private func arrayLog(arr: [Double]) -> [Double] {
        var results = [Double](repeating: 0.0, count: arr.count)
        var mutableArr = arr.map { max($0, Double.leastNormalMagnitude) }
        vvlog(&results, &mutableArr, [Int32(arr.count)])
        return results
    }

    private func parabolic(y: [Double], i: Int) -> [Double] {
        let idx = max(1, min(i, y.count - 2))
        let alpha = y[idx - 1]
        let beta = y[idx]
        let gamma = y[idx + 1]

        let denominator = alpha - 2.0 * beta + gamma
        if abs(denominator) < 1e-6 {
            return [Double(idx), beta]
        }

        let p = 0.5 * (alpha - gamma) / denominator
        let trueI = Double(idx) + p
        let value = beta - 0.25 * (alpha - gamma) * p

        return [trueI, value]
    }
}

enum TunerError: Error {
    case invalidFormat
    case audioSessionError(String)
}