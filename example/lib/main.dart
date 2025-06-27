import 'dart:async';

import 'package:flutter_tuner/flutter_tuner.dart';
import 'package:flutter/material.dart';

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
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            StreamBuilder<double>(
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
                  icon: Icon(Icons.play_arrow),
                ),
                IconButton(
                  onPressed: () {
                    flutterTuner.stopTuning();
                  },
                  icon: Icon(Icons.stop),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
