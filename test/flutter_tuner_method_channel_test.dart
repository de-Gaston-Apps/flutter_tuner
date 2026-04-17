import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_tuner/flutter_tuner_method_channel.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  MethodChannelFlutterTuner platform = MethodChannelFlutterTuner();
  const MethodChannel channel = MethodChannel('flutter_tuner');

  final List<MethodCall> log = <MethodCall>[];

  setUp(() {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger.setMockMethodCallHandler(
      channel,
      (MethodCall methodCall) async {
        log.add(methodCall);
        return null;
      },
    );
  });

  tearDown(() {
    log.clear();
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger.setMockMethodCallHandler(channel, null);
  });

  test('startTuning', () async {
    await platform.startTuning();
    expect(
      log,
      <Matcher>[
        isMethodCall('startTuning', arguments: null),
      ],
    );
  });

  test('stopTuning', () async {
    await platform.stopTuning();
    expect(
      log,
      <Matcher>[
        isMethodCall('stopTuning', arguments: null),
      ],
    );
  });
}
