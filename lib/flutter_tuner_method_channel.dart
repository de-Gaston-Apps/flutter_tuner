import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'flutter_tuner_platform_interface.dart';

/// An implementation of [FlutterTunerPlatform] that uses method channels.
class MethodChannelFlutterTuner implements FlutterTunerPlatform {
  @visibleForTesting
  static const methodChannel = MethodChannel('flutter_tuner');

  @visibleForTesting
  static const eventChannel = EventChannel('flutter_tuner_stream');

  @override
  Future<void> startTuning() => methodChannel.invokeMethod('startTuning');

  @override
  Future<void> stopTuning() => methodChannel.invokeMethod('stopTuning');

  @override
  Stream<double> get frequencyStream =>
      eventChannel.receiveBroadcastStream().map((event) => event as double);
}
