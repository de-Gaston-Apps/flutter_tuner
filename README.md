# Flutter Tuner

[![CI](https://github.com/Jonny5-5/flutter_tuner/actions/workflows/ci.yml/badge.svg)](https://github.com/Jonny5-5/flutter_tuner/actions/workflows/ci.yml)

flutter_tuner is a high-performance Flutter plugin for real-time audio frequency detection and tuning. It utilizes a shared C++ core implementation based on the YINFFT algorithm to provide exceptional accuracy and performance across both Android and iOS platforms. This project was developed to provide a robust, cross-platform solution for building musical instrument tuners and other audio analysis applications that require reliable pitch estimation with minimal overhead.

## Developer Pages

- [Google Play Store](https://play.google.com/store/apps/dev?id=8230997084127446105)
- [Apple App Store](https://apps.apple.com/us/developer/jonathan-de-gaston/id1656029142)

## Usage

Check out the [example code](https://github.com/Jonny5-5/flutter_tuner/blob/main/example/lib/main.dart) for a full implementation.

```dart
final flutterTuner = FlutterTuner();

// Start tuning
await flutterTuner.startTuning();

// Listen to frequency updates
flutterTuner.frequencyStream.listen((frequency) {
  print('Detected frequency: $frequency Hz');
});

// Stop tuning
await flutterTuner.stopTuning();
```
