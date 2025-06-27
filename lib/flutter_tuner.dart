// You have generated a new plugin project without specifying the `--platforms`
// flag. A plugin project with no platform support was generated. To add a
// platform, run `flutter create -t plugin --platforms <platforms> .` under the
// same directory. You can also find a detailed instruction on how to add
// platforms in the `pubspec.yaml` at
// https://flutter.dev/to/pubspec-plugin-platforms.

import 'flutter_tuner_platform_interface.dart';

class FlutterTuner {
  /// Starts the frequency stream from the native platform.
  Future<void> startTuning() {
    return FlutterTunerPlatform.instance.startTuning();
  }

  /// Stops the frequency stream from the native platform.
  Future<void> stopTuning() {
    return FlutterTunerPlatform.instance.stopTuning();
  }

  /// Stream of frequency (in Hz) emitted by the native tuner.
  Stream<double> get frequencyStream {
    return FlutterTunerPlatform.instance.frequencyStream;
  }
}
