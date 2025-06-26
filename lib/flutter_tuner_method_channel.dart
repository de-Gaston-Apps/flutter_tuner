import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'flutter_tuner_platform_interface.dart';

/// An implementation of [FlutterTunerPlatform] that uses method channels.
class MethodChannelFlutterTuner extends FlutterTunerPlatform {
  /// The method channel used to interact with the native platform.
  @visibleForTesting
  final methodChannel = const MethodChannel('flutter_tuner');

  @override
  Future<String?> getPlatformVersion() async {
    final version = await methodChannel.invokeMethod<String>('getPlatformVersion');
    return version;
  }
}
