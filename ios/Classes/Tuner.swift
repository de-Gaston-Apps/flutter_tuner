import Foundation
import AVFoundation

class Tuner {
    static let sampleRate: Double = 44100
    static let bufferSize = Int(sampleRate / 3)
    static let bytesPerElement = 2
    static let minSignalLen = Int(sampleRate / 4)
    static let attenuation = 0.80

    static let errorFrequency = -1.0
    static let tooQuietFrequency = -2.0

    private var audioEngine = AVAudioEngine()
    private let callback: (Double) -> Void
    private var isRunning = false

    init(callback: @escaping (Double) -> Void) {
        self.callback = callback
    }

    func start() {
        guard !isRunning else { return }
        isRunning = true

        let inputNode = audioEngine.inputNode
        let format = inputNode.inputFormat(forBus: 0)
        let frameCount = UInt32(Tuner.bufferSize)

        inputNode.installTap(onBus: 0, bufferSize: frameCount, format: format) { (buffer, time) in
            guard self.isRunning else { return }

            let channelData = buffer.floatChannelData?[0]
            let frameLength = Int(buffer.frameLength)

            guard let data = channelData else {
                self.callback(Tuner.errorFrequency)
                return
            }

            var shorts = [Int16](repeating: 0, count: frameLength)
            for i in 0..<frameLength {
                shorts[i] = Int16(data[i] * Float(Int16.max))
            }

            let frequency = self.findFrequency(sData: shorts)
            self.callback(frequency)
        }

        do {
            try audioEngine.start()
        } catch {
            callback(Tuner.errorFrequency)
        }
    }

    func stop() {
        guard isRunning else { return }
        isRunning = false
        audioEngine.inputNode.removeTap(onBus: 0)
        audioEngine.stop()
    }

    private func findFrequency(sData: [Int16]) -> Double {
        guard sData.count >= Tuner.bufferSize else { return Tuner.errorFrequency }

        let startIndex = 1000
        let windowSize = 1200

        var autoXSum = 0.0
        for j in startIndex..<(startIndex + windowSize) {
            autoXSum += Double(sData[j]) * Double(sData[j])
        }

        if autoXSum < 4 * Double(windowSize) {
            return Tuner.errorFrequency
        }

        var xSum: Double
        var foundPeak = false
        var peakIndex = -1
        var peakHeight = -1.0

        var k = 30
        while k < 1102 {
            if k > 400 { k += 1 }
            xSum = 0.0
            for j in startIndex..<(startIndex + windowSize) {
                xSum += Double(sData[j]) * Double(sData[j + k])
            }

            if foundPeak {
                if xSum > peakHeight {
                    peakIndex = k
                    peakHeight = xSum
                } else {
                    break
                }
            } else {
                if xSum > autoXSum * Tuner.attenuation {
                    peakIndex = k
                    peakHeight = xSum
                    foundPeak = true
                } else {
                    if xSum < 0 { k += 2 }
                }
            }
            k += 1
        }

        guard foundPeak else { return Tuner.errorFrequency }

        var precision = 1
        var precisionPeak = peakIndex
        if peakIndex < 350 {
            precision = 20
        } else if peakIndex < 700 {
            precision = 10
        } else if peakIndex < 1400 {
            precision = 5
        } else if peakIndex < 3500 {
            precision = 2
        }
        precisionPeak = peakIndex * precision

        var maxVal = 0.0
        let searchSpan = 2 + precision
        let pStart = precisionPeak - searchSpan
        let pStop = precisionPeak + searchSpan

        for k in pStart..<pStop {
            var pXSum = 0.0
            for j in startIndex..<(startIndex + windowSize) {
                pXSum += Double(sData[j]) * Double(sData[j + k]) / 512.0
            }
            if pXSum > maxVal {
                maxVal = pXSum
                precisionPeak = k
            }
        }

        return Tuner.sampleRate * Double(precision) / Double(precisionPeak)
    }
}
