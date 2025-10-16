import Flutter
import UIKit
import AVFoundation

public class FlutterTunerPlugin: NSObject, FlutterPlugin, FlutterStreamHandler {
  private var eventSink: FlutterEventSink?
  private var tuner: TunerNew?

  public static func register(with registrar: FlutterPluginRegistrar) {
    let methodChannel = FlutterMethodChannel(name: "flutter_tuner", binaryMessenger: registrar.messenger())
    let eventChannel = FlutterEventChannel(name: "flutter_tuner_stream", binaryMessenger: registrar.messenger())

    let instance = FlutterTunerPlugin()
    registrar.addMethodCallDelegate(instance, channel: methodChannel)
    eventChannel.setStreamHandler(instance)
  }
    
    public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
        switch call.method {
        case "startTuning":
            AVAudioSession.sharedInstance().requestRecordPermission { granted in
                if granted {
                    do {
                        try self.startTuning()
                        result(nil)
                    } catch {
                        result(FlutterError(code: "TUNER_ERROR", message: error.localizedDescription, details: nil))
                    }
                } else {
                    result(FlutterError(code: "401", message: "Microphone permission not granted", details: nil))
                }
            }
        case "stopTuning":
            self.stopTuning()
            result(nil)
        default:
            result(FlutterMethodNotImplemented)
        }
    }

    private func startTuning() throws {
        guard tuner == nil else { return }

        tuner = TunerNew { [weak self] frequency in
            DispatchQueue.main.async {
                self?.eventSink?(frequency)
            }
        }

        try tuner?.start()
    }

    private func stopTuning() {
        tuner?.stop()
        tuner = nil
    }
    
  public func onListen(withArguments arguments: Any?, eventSink events: @escaping FlutterEventSink) -> FlutterError? {
    self.eventSink = events
    return nil
  }

  public func onCancel(withArguments arguments: Any?) -> FlutterError? {
    stopTuning()
    self.eventSink = nil
    return nil
  }
}
