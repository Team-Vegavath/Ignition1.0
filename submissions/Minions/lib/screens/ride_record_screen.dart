import 'dart:async';
import 'package:flutter/material.dart';
import '../models/sample.dart';
import '../services/db_service.dart';
import '../services/sensor_service.dart';
import '../theme/app_styles.dart';


class RideRecordScreen extends StatefulWidget {
  final DbService db;
  final SensorService sensor;
  const RideRecordScreen({super.key, required this.db, required this.sensor});

  @override
  State<RideRecordScreen> createState() => _RideRecordScreenState();
}

class _RideRecordScreenState extends State<RideRecordScreen> {
  StreamSubscription<Sample>? _sub;
  Sample? _last;
  bool _running = false;
  double _decel = 0.0; // m/s^2 (negative = braking)
  DateTime _lastT = DateTime.now();
  double _lastSpeed = 0.0;

  Future<void> _start() async {
    if (_running) return;
    await widget.sensor.startRide();
    _sub = widget.sensor.stream.listen((s) {
      final now = s.timestamp;
      final dt = (now.difference(_lastT).inMilliseconds / 1000.0).clamp(0.001, 60.0);
      final speedMs = s.speedKmh / 3.6;
      final lastMs = _lastSpeed / 3.6;
      _decel = (speedMs - lastMs) / dt; // negative => decel
      _lastT = now;
      _lastSpeed = s.speedKmh;
      setState(() => _last = s);
    });
    setState(() => _running = true);
  }

  Future<void> _stop() async {
    if (!_running) return;
    await _sub?.cancel();
    await widget.sensor.stopRide();
    setState(() {
      _running = false;
      _sub = null;
    });
    if (mounted) Navigator.of(context).pop(); // back to list, ride will appear there
  }

  @override
  void dispose() {
    _sub?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final s = _last;
    return Scaffold(
      appBar: AppBar(title: const Text('Record Ride')),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: ListView(
          children: [
            _tile('Accel Longitudinal (ax)', s?.axLong, suffix: 'm/s²'),
            _tile('Accel Lateral (ay)', s?.ayLat, suffix: 'm/s²'),
            _tile('Accel Vertical (az)', s?.azUp, suffix: 'm/s²'),
            _tile('Deceleration', _decel, suffix: 'm/s²'),
            const Divider(),
            _tile('Speed', s?.speedKmh, suffix: 'km/h'),
            _tile('Latitude', s?.latitude),
            _tile('Longitude', s?.longitude),
            _tile('Bearing', s?.bearingDeg, suffix: '°'),
            _tile('Timestamp', s?.timestamp.toIso8601String()),
            const SizedBox(height: 24),
            if (!_running)
              ElevatedButton.icon(
                style: ElevatedButton.styleFrom(backgroundColor: Colors.green),
                onPressed: _start,
                icon: const Icon(Icons.play_arrow),
                label: const Text('Start Ride'),
              ),
            if (_running)
              ElevatedButton.icon(
                style: ElevatedButton.styleFrom(backgroundColor: Colors.red),
                onPressed: _stop,
                icon: const Icon(Icons.stop),
                label: const Text('Stop Ride'),
              ),
          ],
        ),
      ),
    );
  }

  Widget _tile(String title, Object? value, {String suffix = ''}) {
    String text;
    if (value == null) {
      text = '--';
    } else if (value is num) {
      text = value.toStringAsFixed(3) + (suffix.isEmpty ? '' : ' $suffix');
    } else {
      text = value.toString();
    }
    return Card(
      margin: const EdgeInsets.symmetric(vertical: 8),
      child: ListTile(
        title: Text(title),
        trailing: Text(text, style: const TextStyle(fontWeight: FontWeight.bold)),
      ),
    );
  }
}
