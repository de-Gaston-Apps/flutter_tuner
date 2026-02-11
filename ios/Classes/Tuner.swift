import Foundation
import AVFoundation

class Tuner {
    static let bufferSize = 1024
    static let errorFrequency = -1.0
    
    private let fftSize = 4096
    private var fftBuffer = [Double]()

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

        // 1. Audio session FIRST
        let session = AVAudioSession.sharedInstance()

        try session.setCategory(
            .playAndRecord,
            mode: .measurement,
            options: [.allowBluetooth, .defaultToSpeaker, .mixWithOthers]
        )

        try session.setPreferredSampleRate(44100)
        try session.setActive(true)

        // 2. Engine setup
        let inputNode = audioEngine.inputNode

        // 3. Start engine BEFORE querying format
        try audioEngine.start()

        // 4. Now the hardware format is valid
        let hwFormat = inputNode.inputFormat(forBus: 0)

        guard hwFormat.sampleRate > 0, hwFormat.channelCount > 0 else {
            throw NSError(domain: "Tuner", code: -1)
        }

        sampleRate = hwFormat.sampleRate
        
        tuner = create_tuner(
            Int32(sampleRate),
            Int32(Tuner.bufferSize)
        )

        // 5. Install tap with format = nil
        inputNode.installTap(
            onBus: 0,
            bufferSize: AVAudioFrameCount(Tuner.bufferSize),
            format: nil
        ) { [weak self] buffer, _ in
            guard
                let self = self,
                self.isRunning,
                let tuner = self.tuner
            else { return }

            guard let channelData = buffer.floatChannelData?[0] else {
                self.callback(Tuner.errorFrequency)
                return
            }

            let frameLength = Int(buffer.frameLength)

            for i in 0..<frameLength {
                fftBuffer.append(Double(channelData[i]))
            }

            while fftBuffer.count >= fftSize {
                let window = Array(fftBuffer.prefix(fftSize))
                fftBuffer.removeFirst(fftSize)

                let frequency = find_frequency(
                    tuner,
                    window,
                    Int32(fftSize)
                )

                callback(frequency)
            }
        }

        isRunning = true
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
