package com.example.ascend

import android.Manifest
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothSocket
import android.content.pm.PackageManager
import android.os.Build
import android.util.Log
import androidx.core.app.ActivityCompat
import io.flutter.embedding.android.FlutterActivity
import io.flutter.plugin.common.EventChannel
import kotlinx.coroutines.*
import java.io.InputStream
import java.util.*

class MainActivity : FlutterActivity() {
    private val CHANNEL = "ascend.telemetry/stream"
    private val btAdapter: BluetoothAdapter? = BluetoothAdapter.getDefaultAdapter()
    private var socket: BluetoothSocket? = null
    private var readJob: Job? = null

    override fun configureFlutterEngine(flutterEngine: io.flutter.embedding.engine.FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)

        EventChannel(flutterEngine.dartExecutor.binaryMessenger, CHANNEL)
            .setStreamHandler(object : EventChannel.StreamHandler {
                override fun onListen(arguments: Any?, events: EventChannel.EventSink?) {
                    Log.i("BT", "Listening for RiderTelemetry...")
                    connectToESP32(events)
                }

                override fun onCancel(arguments: Any?) {
                    readJob?.cancel()
                }
            })
    }

    private fun connectToESP32(events: EventChannel.EventSink?) {
        if (btAdapter == null) {
            events?.error("BT_UNSUPPORTED", "Bluetooth not supported", null)
            return
        }

        if (!btAdapter.isEnabled) {
            events?.error("BT_DISABLED", "Bluetooth is off", null)
            return
        }

        // ✅ Permission check for Android 12+
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.BLUETOOTH_CONNECT), 1)
                return
            }
        }

        val device: BluetoothDevice? = btAdapter.bondedDevices.firstOrNull {
            it.name == "RiderTelemetry"
        }

        if (device == null) {
            events?.error("DEVICE_NOT_FOUND", "RiderTelemetry not paired", null)
            Log.e("BT", "RiderTelemetry not found among paired devices.")
            return
        }

        val uuid = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB")

        try {
            socket = device.createRfcommSocketToServiceRecord(uuid)
            btAdapter.cancelDiscovery()
            socket!!.connect()
            Log.i("BT", "✅ Connected to RiderTelemetry")
            events?.success("Connected to RiderTelemetry")
            startReading(socket!!.inputStream, events)
        } catch (e: Exception) {
            Log.e("BT", "Connection failed: ${e.message}")
            events?.error("CONNECT_FAIL", e.message, null)
        }
    }

    private fun startReading(input: InputStream, events: EventChannel.EventSink?) {
        readJob?.cancel()
        readJob = CoroutineScope(Dispatchers.IO).launch {
            val buffer = ByteArray(1024)
            while (isActive) {
                try {
                    val bytes = input.read(buffer)
                    if (bytes > 0) {
                        val raw = String(buffer, 0, bytes).trim()
                        Log.i("BT_DATA", raw)
                        events?.success(raw)
                    }
                } catch (e: Exception) {
                    Log.e("BT_READ", "Error: ${e.message}")
                    events?.error("READ_FAIL", e.message, null)
                    break
                }
            }
        }
    }
}
