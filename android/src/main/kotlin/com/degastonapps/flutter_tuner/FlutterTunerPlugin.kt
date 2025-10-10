package com.degastonapps.flutter_tuner

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.os.Handler
import android.os.Looper
import android.util.Log
import androidx.core.content.ContextCompat
import io.flutter.embedding.engine.plugins.FlutterPlugin
import io.flutter.plugin.common.EventChannel
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import io.flutter.plugin.common.MethodChannel.MethodCallHandler
import io.flutter.plugin.common.MethodChannel.Result

class FlutterTunerPlugin : FlutterPlugin, MethodCallHandler, EventChannel.StreamHandler {
  private lateinit var methodChannel: MethodChannel
  private lateinit var eventChannel: EventChannel
  private lateinit var context: Context
  private val logTag = "Flutter_Tuner"
  private var eventSink: EventChannel.EventSink? = null


  /*
   * This is the function that gets called when the tuner frequency changes.
   */
  private val tunerCallback: (Double) -> Unit = { freq ->
    Handler(Looper.getMainLooper()).post {
      eventSink?.success(freq)
    }
  }

//  private val tuner = Tuner(tunerCallback)
  private val tuner = TunerNew(tunerCallback)

  override fun onAttachedToEngine(binding: FlutterPlugin.FlutterPluginBinding) {
    context = binding.applicationContext
    methodChannel = MethodChannel(binding.binaryMessenger, "flutter_tuner")
    eventChannel = EventChannel(binding.binaryMessenger, "flutter_tuner_stream")

    methodChannel.setMethodCallHandler(this)
    eventChannel.setStreamHandler(this)
  }

  override fun onMethodCall(call: MethodCall, result: Result) {
      try {
          when (call.method) {
            "startTuning" -> {
              if (ContextCompat.checkSelfPermission(context, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
                result.error("401", "Microphone permission not granted", null)
                return
              }
              startTuning()
              result.success(null)
            }
            "stopTuning" -> {
              stopTuning()
              result.success(null)
            }
            else -> result.notImplemented()
          }
      } catch (e: Exception) {
        Log.e(logTag, "Unexpected exception", e)
        result.error(
          "317",
          "Unexpected exception", e.localizedMessage
        )
      }
  }

  private fun startTuning() {
    tuner.start()
  }

  private fun stopTuning() {
    tuner.stop()
  }


  override fun onListen(arguments: Any?, events: EventChannel.EventSink?) {
    eventSink = events
  }

  override fun onCancel(arguments: Any?) {
    eventSink = null
    stopTuning()
  }

  override fun onDetachedFromEngine(binding: FlutterPlugin.FlutterPluginBinding) {
    methodChannel.setMethodCallHandler(null)
    eventChannel.setStreamHandler(null)
  }
}