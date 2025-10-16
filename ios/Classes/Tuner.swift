import Foundation
import AVFoundation

class Tuner {
    static let bufferSize = 4096
    static let errorFrequency = -1.0

    private var audioEngine = AVAudioEngine()
    private let callback: (Double) -> Void
    private var isRunning = false
    private var tuner: OpaquePointer?
    private var sampleRate: Double = 0.0

    init(callback: @escaping (Double) -> Void) {
        self.callback = callback
    }

    func start() throws {
        guard !isRunning else { return }
        isRunning = true

        let inputNode = audioEngine.inputNode
        let format = inputNode.inputFormat(forBus: 0)
        sampleRate = format.sampleRate

        tuner = create_tuner(Int32(sampleRate), Int32(Tuner.bufferSize))

        inputNode.installTap(onBus: 0, bufferSize: UInt32(Tuner.bufferSize), format: format) { (buffer, time) in
            guard self.isRunning, let tuner = self.tuner else { return }

            let channelData = buffer.floatChannelData?[0]
            let frameLength = Int(buffer.frameLength)

            guard let data = channelData else {
                self.callback(Tuner.errorFrequency)
                return
            }

            var doubleData = [Double](repeating: 0.0, count: frameLength)
            for i in 0..<frameLength {
                doubleData[i] = Double(data[i])
            }

            let frequency = find_frequency(tuner, doubleData, Int32(frameLength))
            self.callback(frequency)
        }

        do {
            try audioEngine.start()
        } catch {
            callback(Tuner.errorFrequency)
        }
    }

    func stop() throws {
        guard isRunning else { return }
        isRunning = false
        if let tuner = tuner {
            destroy_tuner(tuner)
            self.tuner = nil
        }
        audioEngine.inputNode.removeTap(onBus: 0)
        audioEngine.stop()
    }
}
