import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';
import 'dart:math';
import 'package:flutter/material.dart';
import 'package:flutter_bluetooth_serial/flutter_bluetooth_serial.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:shared_preferences/shared_preferences.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await _requestPermissions();
  runApp(const RiderTelemetryApp());
}

Future<void> _requestPermissions() async {
  await [
    Permission.bluetooth,
    Permission.bluetoothConnect,
    Permission.bluetoothScan,
    Permission.location,
  ].request();
}

class RiderTelemetryApp extends StatelessWidget {
  const RiderTelemetryApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: "Rider Telemetry Dashboard",
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        brightness: Brightness.dark,
        scaffoldBackgroundColor: const Color(0xFF0F1724),
        cardColor: const Color(0xFF1E293B),
        colorScheme: const ColorScheme.dark(primary: Color(0xFF00B4D8)),
      ),
      home: const TelemetryHomePage(),
    );
  }
}

class TelemetryHomePage extends StatefulWidget {
  const TelemetryHomePage({super.key});

  @override
  State<TelemetryHomePage> createState() => _TelemetryHomePageState();
}

class _TelemetryHomePageState extends State<TelemetryHomePage> {
  BluetoothConnection? _connection;
  bool _isConnected = false;
  final String _deviceName = "RiderTelemetry";

  // Telemetry data
  String state = "Idle";
  String activity = "Standing";
  String speed = "0.00";
  String signal = "None";
  String source = "MPU";
  String lean = "0.0";
  String ax = "0.00";
  String ay = "0.00";
  String az = "0.00";
  String lat = "NoFix";
  String lon = "NoFix";
  String sats = "0";

  // Numeric state
  double lastSpeedVal = 0.0;
  double _leanOffset = 0.0;
  double currentLeanVal = 0.0;
  final List<double> _leanSamples = [];
  final int _maxSamples = 15;

  List<String> _dataHistory = [];
  Timer? _logTimer;

  final double gpsMovingThreshold = 1.5;
  final double mpuAccThreshold = 0.3;

  @override
  void initState() {
    super.initState();
    _loadHistory();
    _connectToDevice();
    _startLoggingTimer();
  }

  Future<void> _loadHistory() async {
    final prefs = await SharedPreferences.getInstance();
    setState(() {
      _dataHistory = prefs.getStringList("telemetry_history") ?? [];
    });
  }

  void _startLoggingTimer() {
    _logTimer = Timer.periodic(const Duration(seconds: 5), (_) async {
      final prefs = await SharedPreferences.getInstance();
      final entry =
          "[${DateTime.now().toIso8601String()}] State:$state Activity:$activity Speed:$speed Lean:$lean° Signal:$signal Source:$source X:$ax Y:$ay Z:$az Lat:$lat Lon:$lon Sat:$sats";
      _dataHistory.add(entry);
      if (_dataHistory.length > 500) _dataHistory.removeAt(0);
      await prefs.setStringList("telemetry_history", _dataHistory);
    });
  }

  Future<void> _connectToDevice() async {
    try {
      List<BluetoothDevice> devices =
          await FlutterBluetoothSerial.instance.getBondedDevices();

      if (devices.isEmpty) {
        debugPrint("No bonded devices found.");
        Future.delayed(const Duration(seconds: 5), _connectToDevice);
        return;
      }

      BluetoothDevice? target;
      for (var d in devices) {
        if (d.name != null && d.name!.contains(_deviceName)) {
          target = d;
          break;
        }
      }
      target ??= devices.first;

      BluetoothConnection connection =
          await BluetoothConnection.toAddress(target.address);

      setState(() {
        _connection = connection;
        _isConnected = true;
      });

      connection.input?.listen((Uint8List rawData) {
        final chunk = utf8.decode(rawData);
        for (final line in chunk.split(RegExp(r'[\r\n]+'))) {
          final msg = line.trim();
          if (msg.isNotEmpty) _processIncomingLine(msg);
        }
      }).onDone(() {
        setState(() => _isConnected = false);
        Future.delayed(const Duration(seconds: 3), _connectToDevice);
      });
    } catch (e) {
      debugPrint("Bluetooth connection error: $e");
      Future.delayed(const Duration(seconds: 5), _connectToDevice);
    }
  }

  void _processIncomingLine(String msg) {
    final speedMatch = RegExp(r'Speed:(\d+(\.\d+)?)').firstMatch(msg);
    final sigMatch = RegExp(r'Sig:(\w+)').firstMatch(msg);
    final srcMatch = RegExp(r'Source:(\w+)').firstMatch(msg);
    final leanMatch = RegExp(r'Lean:([-]?\d+(\.\d+)?)').firstMatch(msg);
    final axMatch = RegExp(r'X:([-]?\d+(\.\d+)?)').firstMatch(msg);
    final ayMatch = RegExp(r'Y:([-]?\d+(\.\d+)?)').firstMatch(msg);
    final azMatch = RegExp(r'Z:([-]?\d+(\.\d+)?)').firstMatch(msg);
    final latMatch = RegExp(r'Lat:([-]?\d+(\.\d+)?)').firstMatch(msg);
    final lonMatch = RegExp(r'Lon:([-]?\d+(\.\d+)?)').firstMatch(msg);
    final satMatch = RegExp(r'Sat:(\d+)').firstMatch(msg);

    setState(() {
      if (speedMatch != null) {
        speed = speedMatch.group(1)!;
        lastSpeedVal = double.tryParse(speed) ?? lastSpeedVal;
      }
      if (sigMatch != null) signal = sigMatch.group(1)!;
      if (srcMatch != null) source = srcMatch.group(1)!;
      if (leanMatch != null) {
        lean = leanMatch.group(1)!;
        currentLeanVal = double.tryParse(lean) ?? 0.0;
        _updateLean(currentLeanVal);
      }
      if (axMatch != null) ax = axMatch.group(1)!;
      if (ayMatch != null) ay = ayMatch.group(1)!;
      if (azMatch != null) az = azMatch.group(1)!;
      if (latMatch != null) lat = latMatch.group(1)!;
      if (lonMatch != null) lon = lonMatch.group(1)!;
      if (satMatch != null) sats = satMatch.group(1)!;
    });

    // Determine motion
    final dx = double.tryParse(ax) ?? 0.0;
    final dy = double.tryParse(ay) ?? 0.0;
    final dz = double.tryParse(az) ?? 0.0;
    final totalAcc = sqrt(dx * dx + dy * dy + dz * dz);
    final accWithoutG = (totalAcc - 1.0).abs();

    final bool moving = (lastSpeedVal > gpsMovingThreshold) || (accWithoutG > mpuAccThreshold);
    setState(() => state = moving ? "Moving" : "Idle");
  }

  void _updateLean(double rawLean) {
    if (lastSpeedVal < 2.0) {
      _leanOffset = (_leanOffset * 0.9) + (rawLean * 0.1);
    }

    double corrected = rawLean - _leanOffset;
    lean = corrected.toStringAsFixed(1);

    _leanSamples.add(corrected);
    if (_leanSamples.length > _maxSamples) _leanSamples.removeAt(0);

    double mean = _leanSamples.reduce((a, b) => a + b) / _leanSamples.length;
    double variance = _leanSamples
            .map((v) => pow(v - mean, 2))
            .reduce((a, b) => a + b) /
        _leanSamples.length;
    double stdDev = sqrt(variance);

    setState(() {
      if (lastSpeedVal < 1.5) {
        activity = "Standing";
      } else if (lastSpeedVal < 10.0) {
        activity = "Walking";
      } else {
        activity = stdDev > 10 ? "Motorcycle" : "Scooter";
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text("Rider Telemetry Dashboard"),
        backgroundColor: const Color(0xFF00B4D8),
        actions: [
          IconButton(
            icon: const Icon(Icons.history),
            onPressed: () async {
              final prefs = await SharedPreferences.getInstance();
              final stored = prefs.getStringList("telemetry_history") ?? [];
              Navigator.push(
                context,
                MaterialPageRoute(
                    builder: (_) => HistoryPage(dataHistory: stored)),
              );
            },
          ),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 12),
            child: Icon(
              _isConnected
                  ? Icons.bluetooth_connected
                  : Icons.bluetooth_disabled,
              color: _isConnected ? Colors.greenAccent : Colors.redAccent,
            ),
          ),
        ],
      ),
      body: _buildDashboard(),
    );
  }

  Widget _buildDashboard() {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          _buildDataTile(Icons.power_settings_new, "State", state),
          _buildDataTile(Icons.directions_walk, "Activity", activity),
          _buildDataTile(Icons.speed, "Speed", "$speed km/h"),
          _buildDataTile(Icons.refresh, "Lean Angle", "$lean°"),
          _buildDataTile(Icons.wifi, "Signal", signal),
          _buildDataTile(Icons.sensors, "Source", source),
          const SizedBox(height: 12),
          const Text("Acceleration (g)",
              style: TextStyle(
                  color: Color(0xFF00B4D8),
                  fontSize: 18,
                  fontWeight: FontWeight.bold)),
          const SizedBox(height: 8),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceAround,
            children: [
              _buildAccelBox("X", ax),
              _buildAccelBox("Y", ay),
              _buildAccelBox("Z", az),
            ],
          ),
          const SizedBox(height: 16),
          const Text("GPS Data",
              style: TextStyle(
                  color: Color(0xFF00B4D8),
                  fontSize: 18,
                  fontWeight: FontWeight.bold)),
          const SizedBox(height: 8),
          _buildGPSRow("Latitude", lat),
          _buildGPSRow("Longitude", lon),
          _buildGPSRow("Satellites", sats),
        ],
      ),
    );
  }

  Widget _buildDataTile(IconData icon, String label, String value) => Container(
        margin: const EdgeInsets.symmetric(vertical: 6),
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 14),
        decoration: BoxDecoration(
          color: const Color(0xFF1E293B),
          borderRadius: BorderRadius.circular(12),
        ),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Row(
              children: [
                Icon(icon, color: const Color(0xFF00B4D8), size: 22),
                const SizedBox(width: 12),
                Text(label,
                    style: const TextStyle(fontSize: 16, color: Colors.white70)),
              ],
            ),
            Text(value,
                style: const TextStyle(
                    fontSize: 16,
                    color: Colors.white,
                    fontWeight: FontWeight.w600)),
          ],
        ),
      );

  Widget _buildAccelBox(String label, String value) => Container(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
        decoration: BoxDecoration(
            color: const Color(0xFF1E293B),
            borderRadius: BorderRadius.circular(8)),
        child: Text("$label: $value",
            style: const TextStyle(color: Colors.white70, fontSize: 15)),
      );

  Widget _buildGPSRow(String label, String value) => Padding(
        padding: const EdgeInsets.symmetric(vertical: 4),
        child: Text("$label: $value",
            style: const TextStyle(color: Colors.white70, fontSize: 15)),
      );

  @override
  void dispose() {
    _logTimer?.cancel();
    _connection?.dispose();
    super.dispose();
  }
}

class HistoryPage extends StatelessWidget {
  final List<String> dataHistory;
  const HistoryPage({super.key, required this.dataHistory});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text("Telemetry History"),
        backgroundColor: const Color(0xFF00B4D8),
        actions: [
          IconButton(
            icon: const Icon(Icons.delete),
            onPressed: () async {
              final prefs = await SharedPreferences.getInstance();
              await prefs.remove("telemetry_history");
              Navigator.pop(context);
            },
          )
        ],
      ),
      body: dataHistory.isEmpty
          ? const Center(
              child: Text("No telemetry data logged yet",
                  style: TextStyle(color: Colors.white54)))
          : ListView.builder(
              itemCount: dataHistory.length,
              itemBuilder: (_, i) => ListTile(
                title: Text(dataHistory[dataHistory.length - 1 - i],
                    style: const TextStyle(color: Colors.white70)),
              ),
            ),
    );
  }
}
