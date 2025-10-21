import XCTest

class TunerTests: XCTestCase {
    func testSineWave() throws {
        let sampleRate = 44100
        let bufferSize = 4096
        let frequency = 440.0

        // Generate a sine wave
        var audioData = [Double](repeating: 0.0, count: bufferSize)
        for i in 0..<bufferSize {
            audioData[i] = sin(2 * .pi * frequency * Double(i) / Double(sampleRate))
        }

        // Initialize the tuner
        let tuner = create_tuner(Int32(sampleRate), Int32(bufferSize))

        // Find the frequency
        let detectedFrequency = find_frequency(tuner, &audioData, Int32(bufferSize))

        // Check if the detected frequency is close to the actual frequency
        XCTAssertEqual(detectedFrequency, frequency, accuracy: 1.0)

        // Clean up
        destroy_tuner(tuner)
    }
}
