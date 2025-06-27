import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'flutter_tuner_method_channel.dart';

abstract class FlutterTunerPlatform extends PlatformInterface {
  /// Constructs a FlutterTunerPlatform.
  FlutterTunerPlatform() : super(token: _token);

  static final Object _token = Object();

  static FlutterTunerPlatform _instance = MethodChannelFlutterTuner();

  /// The default instance of [FlutterTunerPlatform] to use.
  ///
  /// Defaults to [MethodChannelFlutterTuner].
  static FlutterTunerPlatform get instance => _instance;

  /// Platform-specific implementations should set this with their own
  /// platform-specific class that extends [FlutterTunerPlatform] when
  /// they register themselves.
  static set instance(FlutterTunerPlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  /// Start the native tuner.
  Future<void> startTuning() {
    throw UnimplementedError('startTuning() has not been implemented.');
  }

  /// Stop the native tuner.
  Future<void> stopTuning() {
    throw UnimplementedError('stopTuning() has not been implemented.');
  }

  /// Stream of frequency updates (in Hz).
  Stream<double> get frequencyStream {
    throw UnimplementedError('frequencyStream has not been implemented.');
  }
}
