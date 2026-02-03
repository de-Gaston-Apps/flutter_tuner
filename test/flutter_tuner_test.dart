import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_tuner/flutter_tuner.dart';
import 'package:flutter_tuner/flutter_tuner_platform_interface.dart';
import 'package:flutter_tuner/flutter_tuner_method_channel.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

class MockFlutterTunerPlatform
    with MockPlatformInterfaceMixin
    implements FlutterTunerPlatform {

  @override
  Future<void> startTuning() => Future.value();

  @override
  Future<void> stopTuning() => Future.value();

  @override
  Stream<double> get frequencyStream => Stream.value(440.0);
}

void main() {
  final FlutterTunerPlatform initialPlatform = FlutterTunerPlatform.instance;

  test('$MethodChannelFlutterTuner is the default instance', () {
    expect(initialPlatform, isInstanceOf<MethodChannelFlutterTuner>());
  });

  test('startTuning', () async {
    FlutterTuner flutterTunerPlugin = FlutterTuner();
    MockFlutterTunerPlatform fakePlatform = MockFlutterTunerPlatform();
    FlutterTunerPlatform.instance = fakePlatform;

    await flutterTunerPlugin.startTuning();
  });
}
