import 'dart:math';
import 'package:flutter/material.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:latlong2/latlong.dart';
import '../models/sample.dart';
import '../theme/app_styles.dart';

class OverviewScreen extends StatefulWidget {
  final int rideId;
  final List<Sample> samples;

  const OverviewScreen({
    super.key,
    required this.rideId,
    required this.samples,
  });

  @override
  State<OverviewScreen> createState() => _OverviewScreenState();
}

class _OverviewScreenState extends State<OverviewScreen> {
  int currentIndex = 0;
  String _currentMode = "Walking";

  // Tilt thresholds (degrees from vertical)
  static const double walkScooterThresh = 10.0; // <10° => Walking
  static const double scooterBikeThresh = 22.0; // 10–22° => Scooter, >22° => Bike
  static const double hysteresis = 2.0;         // prevent flicker

  static const double windowSec = 5.0;          // 5-second steady window
  static const double defaultHz = 10.0;         // fallback sampling rate

  @override
  Widget build(BuildContext context) {
    final samples = widget.samples;

    if (samples.isEmpty) {
      return Scaffold(
        appBar: AppBar(
          title: Text('Overview – Ride #${widget.rideId}'),
          centerTitle: true,
        ),
        body: const Center(
          child: Text(
            'No GPS data recorded for this ride.',
            style: TextStyle(color: Colors.white70),
          ),
        ),
      );
    }

    final s = samples[currentIndex];
    final pos = LatLng(s.latitude, s.longitude);
    final mode = _classifySteady(samples, currentIndex);
    final moving = s.speedKmh > 1.0;
    final routePoints = _smoothTrack(samples);

    return Scaffold(
      backgroundColor: AppColors.background,
      appBar: AppBar(
        backgroundColor: AppColors.background,
        title: Text(
          'Overview – Ride #${widget.rideId}',
          style: const TextStyle(
            fontFamily: 'Orbitron',
            fontWeight: FontWeight.bold,
            color: AppColors.accent,
          ),
        ),
        centerTitle: true,
      ),
      body: Column(
        children: [
          Expanded(
            child: FlutterMap(
              options: MapOptions(
                initialCenter: pos,
                initialZoom: 17.5,
                maxZoom: 19,
                minZoom: 12,
                interactionOptions: const InteractionOptions(
                  flags: InteractiveFlag.all & ~InteractiveFlag.rotate,
                ),
              ),
              children: [
                TileLayer(
                  urlTemplate: 'https://tile.openstreetmap.org/{z}/{x}/{y}.png',
                  userAgentPackageName: 'rider_telemetry_app',
                ),
                PolylineLayer(
                  polylines: [
                    Polyline(
                      points: routePoints,
                      strokeWidth: 3.5,
                      color: AppColors.accent.withOpacity(0.8),
                    ),
                  ],
                ),
                MarkerLayer(
                  markers: [
                    Marker(
                      point: pos,
                      width: 100,
                      height: 100,
                      child: Stack(
                        alignment: Alignment.center,
                        children: [
                          const Icon(Icons.navigation,
                              size: 42, color: AppColors.accent),
                          Positioned(
                            top: 4,
                            child: Container(
                              padding: const EdgeInsets.symmetric(
                                  horizontal: 8, vertical: 4),
                              decoration: BoxDecoration(
                                color: Colors.black.withOpacity(0.75),
                                borderRadius: BorderRadius.circular(8),
                                border: Border.all(
                                  color: AppColors.accent.withOpacity(0.7),
                                ),
                              ),
                              child: Text(
                                'Speed: ${s.speedKmh.toStringAsFixed(1)} km/h\n'
                                '${moving ? "Moving" : "Stopped"}\n'
                                'Mode: $mode',
                                textAlign: TextAlign.center,
                                style: const TextStyle(
                                  color: Colors.white,
                                  fontSize: 10,
                                ),
                              ),
                            ),
                          ),
                        ],
                      ),
                    ),
                  ],
                ),
              ],
            ),
          ),
          Container(
            padding: const EdgeInsets.all(12),
            color: Colors.black.withOpacity(0.55),
            child: Column(
              children: [
                const Text(
                  'Progress through ride',
                  style: TextStyle(
                    color: AppColors.accent,
                    fontWeight: FontWeight.w600,
                    fontFamily: 'Orbitron',
                  ),
                ),
                Slider(
                  value: currentIndex.toDouble(),
                  min: 0,
                  max: (samples.length - 1).toDouble(),
                  divisions: samples.length - 1,
                  activeColor: AppColors.accent,
                  inactiveColor: Colors.white24,
                  label: 'Sample ${currentIndex + 1}/${samples.length}',
                  onChanged: (v) {
                    setState(() => currentIndex = v.toInt());
                  },
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  // --- GPS smoothing for route ---
  List<LatLng> _smoothTrack(List<Sample> samples) {
    if (samples.length < 3) {
      return samples.map((s) => LatLng(s.latitude, s.longitude)).toList();
    }
    final out = <LatLng>[];
    for (int i = 1; i < samples.length - 1; i++) {
      final a = samples[i - 1];
      final b = samples[i];
      final c = samples[i + 1];
      final avgLat = (a.latitude + b.latitude + c.latitude) / 3;
      final avgLon = (a.longitude + b.longitude + c.longitude) / 3;
      out.add(LatLng(avgLat, avgLon));
    }
    return out;
  }

  // --- Tilt-based mode classification (steady over 5s) ---
  String _classifySteady(List<Sample> samples, int idx) {
    // Estimate sampling rate
    double hz = defaultHz;
    if (samples.length > 5) {
      final dt = samples.last.timestamp
          .difference(samples.first.timestamp)
          .inMilliseconds /
          1000.0;
      if (dt > 0) hz = samples.length / dt;
    }

    final windowSize = max(1, (hz * windowSec).round());
    final start = max(0, idx - windowSize ~/ 2);
    final end = min(samples.length, start + windowSize);
    final window = samples.sublist(start, end);

    // Compute mean tilt in this 5s window
    double meanTilt = 0.0;
    for (final s in window) {
      meanTilt += s.tiltDeg;
    }
    meanTilt /= window.length;

    // Apply thresholds with hysteresis
    final walkUpper = walkScooterThresh - hysteresis;   // ≤8° = Walking
    final scooterLower = walkScooterThresh + hysteresis; // ≥12° start Scooter
    final scooterUpper = scooterBikeThresh - hysteresis; // ≤20° = Scooter
    final bikeLower = scooterBikeThresh + hysteresis;    // ≥24° = Bike

    String newMode = _currentMode;

    if (meanTilt <= walkUpper) {
      newMode = "Walking";
    } else if (meanTilt >= bikeLower) {
      newMode = "Motorbike";
    } else if (meanTilt >= scooterLower && meanTilt <= scooterUpper) {
      newMode = "Scooter";
    } else {
      // Between boundaries — hold previous mode
      newMode = _currentMode;
    }

    _currentMode = newMode;
    return _currentMode;
  }
}
