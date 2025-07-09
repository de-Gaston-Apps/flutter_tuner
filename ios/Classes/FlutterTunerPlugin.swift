import Flutter
import UIKit

public class FlutterTunerPlugin: NSObject, FlutterPlugin, FlutterStreamHandler {
  private var eventSink: FlutterEventSink?
  private var tuner: Tuner?

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
          try startTuning()
          result(nil)
        case "stopTuning":
          try stopTuning()
          result(nil)
        default:
          result(FlutterMethodNotImplemented)
        }
      } catch {
        result(FlutterError(code: "TUNER_ERROR", message: error.localizedDescription, details: nil))
      }
    }

    private func startTuning() throws {
      guard tuner == nil else { return } // prevent multiple tuners

      tuner = Tuner { [weak self] frequency in
        DispatchQueue.main.async {
          self?.eventSink?(frequency)
        }
      }

      try tuner?.start()
    }

    private func stopTuning() throws {
      try tuner?.stop()
      tuner = nil
    }
    
    
  public func onListen(withArguments arguments: Any?, eventSink events: @escaping FlutterEventSink) -> FlutterError? {
    self.eventSink = events
    return nil
  }

  public func onCancel(withArguments arguments: Any?) -> FlutterError? {
      do {
          try stopTuning()
      }
      catch {
          // Nothing to do here
      }
      
    self.eventSink = nil
    return nil
  }
}
