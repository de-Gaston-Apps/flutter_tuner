import Flutter
import UIKit

public class FlutterTunerPlugin: NSObject, FlutterPlugin, FlutterStreamHandler {
  private var eventSink: FlutterEventSink?
  private var timer: Timer?

  public static func register(with registrar: FlutterPluginRegistrar) {
    let methodChannel = FlutterMethodChannel(name: "flutter_tuner", binaryMessenger: registrar.messenger())
    let eventChannel = FlutterEventChannel(name: "flutter_tuner_stream", binaryMessenger: registrar.messenger())

    let instance = FlutterTunerPlugin()
    registrar.addMethodCallDelegate(instance, channel: methodChannel)
    eventChannel.setStreamHandler(instance)
  }

  public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
    do {
      switch call.method {
      case "startTuning":
        startSendingFakeData()
        result(nil)
      case "stopTuning":
        stopSendingFakeData()
        result(nil)
      default:
        result(FlutterMethodNotImplemented)
      }
    } catch {
      result(FlutterError(code: "PLUGIN_ERROR", message: "Failed to handle method: \(call.method)", details: error.localizedDescription))
    }
  }


  private func startSendingFakeData() {
    stopSendingFakeData() // prevent multiple timers

    timer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { _ in
      let fakeFrequency = 440.0 + Double.random(in: -3...3)
      self.eventSink?(fakeFrequency)
    }
  }

  private func stopSendingFakeData() {
    timer?.invalidate()
    timer = nil
  }

  public func onListen(withArguments arguments: Any?, eventSink events: @escaping FlutterEventSink) -> FlutterError? {
    self.eventSink = events
    return nil
  }

  public func onCancel(withArguments arguments: Any?) -> FlutterError? {
    stopSendingFakeData()
    self.eventSink = nil
    return nil
  }
}
