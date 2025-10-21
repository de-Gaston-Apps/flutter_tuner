Plan: test the algorithm in isolation on the host, test JNI/ObjC++ integration on device/simulator, then test Dart platform channels and UI. Do all three levels and one end-to-end run. Steps, examples, and CI plan below.

# What you need

1. **Algorithm unit tests (C++)**

   * Put gtest-based tests inside `flutter_tuner/cpp/tests` or similar.
   * Use CMake so you can build and run tests on the CI Linux runner (fast, deterministic).
   * This verifies algorithm correctness independent of mobile toolchains.

2. **Native integration tests (Android / iOS)**

   * Android: instrumentation tests or a native test runner that exercises JNI boundary and verifies C++ behavior when called from Java/Kotlin. Use an emulator in CI.
   * iOS: XCTest that calls into Obj-C++ bridge or Swift wrapper to exercise the same C++ code in the simulator. Requires macOS runner in CI.

3. **Flutter layer tests**

   * Unit tests that mock `MethodChannel` to verify Dart API behavior and serialization.
   * Integration tests (`integration_test` package) that run on real/simulated device and exercise the UI and platform calls end-to-end.

4. **End-to-end smoke**

   * A CI job that runs algorithm tests, native integration tests, and Flutter integration tests in sequence.

# Why test algorithm on host first

* Faster. No emulator overhead.
* Easier debugging.
* CI-friendly (Linux runners).
* Avoids toolchain complexity. You still test integration on device later.

# Minimal examples

## 1) C++ algorithm test with gtest (CMake)

`cpp/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.10)
project(flutter_tuner_native)

# build library
add_library(tuner_lib STATIC src/tuner_algorithm.cpp)
target_include_directories(tuner_lib PUBLIC include)

# GoogleTest (FetchContent)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
)
FetchContent_MakeAvailable(googletest)

# tests
add_executable(tuner_tests tests/test_tuner.cpp)
target_link_libraries(tuner_tests PRIVATE tuner_lib gtest_main)
enable_testing()
add_test(NAME tuner_tests COMMAND tuner_tests)
```

`cpp/tests/test_tuner.cpp`

```cpp
#include <gtest/gtest.h>
#include "tuner.h"

TEST(TunerAlgorithm, BasicPitch) {
  Tuner t;
  double out = t.processFrame(...); // call function
  EXPECT_NEAR(out, 440.0, 0.5);
}
```

## 2) Run C++ tests on CI (Linux)

In CI job on ubuntu:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
ctest --output-on-failure
```

This runs algorithm tests quickly without Android/iOS.

## 3) Android JNI integration test (Kotlin instrumentation)

Create an Android instrumentation test that calls your Java/Kotlin wrapper which calls JNI and then asserts results.
`androidTest/src/.../TunerInstrumentedTest.kt`

```kotlin
@RunWith(AndroidJUnit4::class)
class TunerInstrumentedTest {
  @Test
  fun nativeAlgorithmWorks() {
    val wrapper = TunerWrapper()
    val result = wrapper.processSample(sample)
    assertThat(result, closeTo(440.0, 0.5))
  }
}
```

CI: start emulator and run `./gradlew connectedAndroidTest` or run `./gradlew :app:installDebugAndroidTest :app:connectedDebugAndroidTest`.

## 4) iOS native integration (XCTest)

`ios/Tests/TunerTests.swift`

```swift
import XCTest
@testable import YourApp

class TunerTests: XCTestCase {
  func testNativeAlgorithm() {
    let wrapper = TunerWrapper()
    let pitch = wrapper.process(sample)
    XCTAssertEqual(pitch, 440.0, accuracy: 0.5)
  }
}
```

CI: use macOS runner and `xcodebuild test -scheme YourScheme -sdk iphonesimulator -destination 'platform=iOS Simulator,name=iPhone 14'`

## 5) Dart platform-channel unit test

```dart
void main() {
  TestWidgetsFlutterBinding.ensureInitialized();
  const MethodChannel channel = MethodChannel('tuner_channel');

  test('Dart calls native', () async {
    TestDefaultBinaryMessengerBinding.instance!.defaultBinaryMessenger
      .setMockMethodCallHandler(channel, (call) async {
        expect(call.method, 'processSample');
        return 440.0;
      });

    final res = await TunerApi.processSample([...]);
    expect(res, 440.0);
  });
}
```

## 6) Flutter integration test (UI + platform)

Use `integration_test` package and run on emulator.
`integration_test/tuner_e2e_test.dart`

```dart
void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('end-to-end tuning', (tester) async {
    await tester.pumpWidget(MyApp());
    // interact with UI that triggers tuning
    await tester.tap(find.byKey(Key('startButton')));
    await tester.pumpAndSettle(Duration(seconds:2));
    expect(find.textContaining('440.0'), findsOneWidget);
  });
}
```

Run on device: `flutter test integration_test/tuner_e2e_test.dart -d emulator-5554`

# CI job plan (GitHub Actions) — outline

1. Job `cpp-algo-tests` on `ubuntu-latest`

   * Checkout, install CMake & build essentials.
   * `cmake` build and `ctest` to run C++ gtests in `flutter_tuner`.
2. Job `flutter-analyze-test` on `ubuntu-latest`

   * Setup Flutter, `flutter pub get`, `flutter analyze`, `flutter test` (Dart layer unit tests).
3. Job `android-integration` on `ubuntu-latest`

   * Setup SDK, start emulator (use `reactivecircus/android-emulator-runner`), run `./gradlew connectedAndroidTest` and Flutter integration tests against emulator.
4. Job `ios-integration` on `macos-latest` (optional if you need iOS)

   * Build and run `xcodebuild test` and run Flutter integration tests on iOS simulator.

Sequence: `cpp-algo-tests` → `flutter-analyze-test` → `android-integration` → optionally `ios-integration`. Make later jobs depend on earlier success.

# Practical tips and gotchas

* Keep algorithm tests platform-independent. Test them on host. That avoids NDK complexity.
* For JNI/ObjC++ integration use device/sim tests. These validate bindings, memory handling, ABI issues.
* Use deterministic test vectors. Record inputs and expected outputs. Cover edge cases and numeric tolerances.
* CI: emulator startup can be flaky. Increase timeouts and snapshot the emulator for speed.
* iOS tests require macOS runner. Plan cost and parallelism accordingly.
* If C++ code uses SIMD or platform-specific optimizations, test on representative architectures or disable optimizations for host tests to ensure identical behavior when needed.
* Consider fuzz tests for the algorithm inputs if robustness matters.

# Minimal next actions I can produce now

* Provide a ready-to-run `CMakeLists.txt` and `test_tuner.cpp` tailored to your current `tuner` API.
* Provide a GitHub Actions YAML that implements the three CI jobs (host C++ tests, Flutter tests, Android instrumentation + emulator).
* Provide sample Android instrumentation test file and Gradle snippets to include native tests in the test APK.

Pick one and I will generate the exact files.
