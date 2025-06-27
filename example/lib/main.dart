import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_tuner/flutter_tuner.dart';
import 'package:permission_handler/permission_handler.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  final flutterTuner = FlutterTuner();
  late final Stream<double> _stream = flutterTuner.frequencyStream;
  bool _hasMicPermission = false;

  @override
  void initState() {
    super.initState();
    _requestMicrophonePermission();
  }

  Future<void> _requestMicrophonePermission() async {
    final status = await Permission.microphone.request();
    setState(() {
      _hasMicPermission = status == PermissionStatus.granted;
    });
  }

  @override
  void dispose() {
    flutterTuner.stopTuning();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(title: const Text('Tuner')),
        body: Column(
          children: [
            !_hasMicPermission
                ? const Center(
                    child: Text('Microphone permission is required.'),
                  )
                : StreamBuilder<double>(
                    stream: _stream,
                    builder: (context, snapshot) {
                      if (snapshot.connectionState == ConnectionState.waiting) {
                        return const Text('Listening...');
                      }
                      if (!snapshot.hasData) {
                        return const Text('No frequency detected.');
                      }
                      return Text(
                        '${snapshot.data!.toStringAsFixed(2)} Hz',
                        style: const TextStyle(fontSize: 32),
                      );
                    },
                  ),

            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                IconButton(
                  onPressed: () {
                    flutterTuner.startTuning();
                  },
                  icon: const Icon(Icons.play_arrow),
                ),
                IconButton(
                  onPressed: () {
                    flutterTuner.stopTuning();
                  },
                  icon: const Icon(Icons.stop),
                ),
              ],
            ),

            IconButton(
              onPressed: () {
                _requestMicrophonePermission();
              },
              icon: const Icon(Icons.mic),
            ),
          ],
        ),
      ),
    );
  }
}
