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

    // Cap the buffered samples to avoid unbounded growth if producer outpaces consumer.
    // Keep at most this many FFT windows worth of samples.
    private static let maxBufferedWindows = 4
    private static let maxBufferSamples = fftSize * maxBufferedWindows

    static let errorFrequency = -1.0

    private var fftBuffer = [Double]()

    private var tunerBridge: TunerBridge?
    private var audioEngine = AVAudioEngine()
    private let callback: (Double) -> Void
    private var isRunning = false
    private var sampleRate: Double = 0.0

    // Synchronization for tap callback vs. stop()/teardown
    private let tapLock = NSLock()

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
        self.tunerBridge = TunerBridge(
            sampleRate: Int32(sampleRate),
            bufferSize: Int32(Tuner.fftSize)
        )

        // 5. Install tap with format = nil
        inputNode.installTap(
            onBus: 0,
            bufferSize: AVAudioFrameCount(Tuner.bufferSize),
            format: nil
        ) { [weak self] buffer, _ in
            guard let self = self else { return }

            // Synchronize access to isRunning and tuner
            self.tapLock.lock()
            defer { self.tapLock.unlock() }

            guard self.isRunning, let bridge = self.tunerBridge else {
                return
            }

            guard let channelData = buffer.floatChannelData?[0] else {
                self.callback(Tuner.errorFrequency)
                return
            }

            let frameLength = Int(buffer.frameLength)

            // Append new samples
            for i in 0..<frameLength {
                self.fftBuffer.append(Double(channelData[i]))
            }

            // Cap growth: if buffer exceeds maxBufferSamples, drop oldest samples
            if self.fftBuffer.count > Tuner.maxBufferSamples {
                // Keep at least one FFT window to allow immediate processing
                let targetCount = max(Tuner.fftSize, Tuner.maxBufferSamples)
                let dropCount = self.fftBuffer.count - targetCount
                if dropCount > 0 {
                    self.fftBuffer.removeFirst(dropCount)
                }
            }

            // Process as many full windows as are available
            while self.fftBuffer.count >= Tuner.fftSize {
                let window = Array(self.fftBuffer.prefix(Tuner.fftSize))
                self.fftBuffer.removeFirst(Tuner.fftSize)

                let frequency = self.tunerBridge?.findFrequency(
                    window,
                    length: Int32(Tuner.fftSize)
                )

                self.callback(frequency ?? Tuner.errorFrequency)
            }
        }

        isRunning = true
    }

    func stop() throws {
        // Remove the tap FIRST so no more data comes in
        audioEngine.inputNode.removeTap(onBus: 0)
        audioEngine.stop()

        tapLock.lock()
        isRunning = false
        tunerBridge?.dispose()
        tunerBridge = nil
        tapLock.unlock()

        fftBuffer.removeAll()
    }
}
