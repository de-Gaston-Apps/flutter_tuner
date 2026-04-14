import AVFoundation
import Foundation

class Tuner {
    // Define a chunk size and how many chunks per FFT.
    // Adjust these two to tune latency vs. frequency resolution.
    private static let chunksPerFFT = 4        // FFT window spans this many chunks
    private static let chunkSize = 1024        // Tap chunk size in frames

    // Derived constants
    static let bufferSize: Int = chunkSize
    static let fftSize: Int = chunksPerFFT * chunkSize

    static let errorFrequency = -1.0

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

        // Check for microphone permissions or throw
        if #available(iOS 17.0, *) {
            if AVAudioApplication.shared.recordPermission != .granted {
                throw NSError(domain: "Tuner", code: 0, userInfo: [NSLocalizedDescriptionKey: "Microphone permission not granted"])
            }
        } else {
            if AVAudioSession.sharedInstance().recordPermission != .granted {
                throw NSError(domain: "Tuner", code: 0, userInfo: [NSLocalizedDescriptionKey: "Microphone permission not granted"])
            }
        }

        // Sanity check: enforce fftSize is a multiple of bufferSize
        precondition(Tuner.fftSize % Tuner.bufferSize == 0, "fftSize must be a multiple of bufferSize")

        // 1. Audio session FIRST
        let session = AVAudioSession.sharedInstance()

        try session.setCategory(
            .playAndRecord,
            mode: .voiceChat,
            options: [.defaultToSpeaker, .duckOthers]
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

        // Initialize native tuner with the actual analysis window size (fftSize)
        tuner = create_tuner(
            Int32(sampleRate),
            Int32(Tuner.fftSize)
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
                self.fftBuffer.append(Double(channelData[i]))
            }

            while self.fftBuffer.count >= Tuner.fftSize {
                let window = Array(self.fftBuffer.prefix(Tuner.fftSize))
                self.fftBuffer.removeFirst(Tuner.fftSize)

                let frequency = find_frequency(
                    tuner,
                    window,
                    Int32(Tuner.fftSize)
                )

                self.callback(frequency)
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
