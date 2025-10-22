import XCTest
@testable import flutter_tuner

class TunerTests: XCTestCase {
    func testExample() {
        let sampleRate = 44100
        let bufferSize = 4096

        let tuner = TunerWrapper(sampleRate: Int32(sampleRate), bufferSize: Int32(bufferSize))
        XCTAssertNotNil(tuner)

        let audioData: [Double] = [Double](repeating: 0.0, count: bufferSize)
        let frequency = tuner.findFrequency(audioData, dataSize: Int32(bufferSize))
        XCTAssertNotEqual(frequency, -1.0)
    }
}
