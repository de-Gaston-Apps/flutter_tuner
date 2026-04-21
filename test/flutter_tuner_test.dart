import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_tuner/flutter_tuner.dart';
import 'package:flutter_tuner/flutter_tuner_platform_interface.dart';
import 'package:flutter_tuner/flutter_tuner_method_channel.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

class MockFlutterTunerPlatform
    with MockPlatformInterfaceMixin
    implements FlutterTunerPlatform {
  bool startTuningCalled = false;
  bool stopTuningCalled = false;

  @override
  Future<void> startTuning() async {
    startTuningCalled = true;
  }

  @override
  Future<void> stopTuning() async {
    stopTuningCalled = true;
  }

  @override
  Stream<double> get frequencyStream => Stream.empty();
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
    expect(fakePlatform.startTuningCalled, true);
  });

  test('stopTuning', () async {
    FlutterTuner flutterTunerPlugin = FlutterTuner();
    MockFlutterTunerPlatform fakePlatform = MockFlutterTunerPlatform();
    FlutterTunerPlatform.instance = fakePlatform;

    await flutterTunerPlugin.stopTuning();
    expect(fakePlatform.stopTuningCalled, true);
  });
}
