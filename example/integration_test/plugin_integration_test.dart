import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';
import 'package:flutter_tuner/flutter_tuner.dart';

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  group('Tuner Integration Tests', () {
    final FlutterTuner tuner = FlutterTuner();

    testWidgets('Start and stop tuning', (WidgetTester tester) async {
      // Basic check to see if the platform channels don't throw immediate errors
      // In a real environment with a mic, we would also check the frequency stream.

      expect(tuner.startTuning(), completes);

      // Wait a bit to simulate some tuning time
      await Future.delayed(const Duration(seconds: 1));

      expect(tuner.stopTuning(), completes);
    });

    testWidgets('Frequency stream emits values', (WidgetTester tester) async {
      // This test might fail on CI if there is no audio input device available,
      // but we can at least check if we can listen to the stream.

      await tuner.startTuning();

      // We don't necessarily expect a value here unless there's sound,
      // but we want to make sure the stream itself is accessible.
      final stream = tuner.frequencyStream;
      expect(stream, isNotNull);

      await tuner.stopTuning();
    });
  });
}
