import 'package:flutter/material.dart';
import '../models/sample.dart';
import '../services/db_service.dart';
import 'advanced_data_screen.dart';
import 'overview_screen.dart';
import '../theme/app_styles.dart';

class RideDetailScreen extends StatefulWidget {
  final DbService db;
  final int rideId;

  const RideDetailScreen({
    super.key,
    required this.db,
    required this.rideId,
  });

  @override
  State<RideDetailScreen> createState() => _RideDetailScreenState();
}

class _RideDetailScreenState extends State<RideDetailScreen> {
  List<Sample> _samples = [];
  List<Sample> _filtered = [];
  bool _loading = true;

  double avgAccel = 0.0;
  double avgDecel = 0.0;
  double avgSpeed = 0.0;
  double? startLat, startLon, endLat, endLon;

  @override
  void initState() {
    super.initState();
    _loadSamples();
  }

  Future<void> _loadSamples() async {
    final samples = await widget.db.samplesForRide(widget.rideId, limit: 5000);
    if (samples.isEmpty) {
      setState(() => _loading = false);
      return;
    }

    double accelSum = 0, decelSum = 0, speedSum = 0;
    int accelCount = 0, decelCount = 0;

    for (var s in samples) {
      if (s.axLong > 0.2) {
        accelSum += s.axLong;
        accelCount++;
      } else if (s.axLong < -0.2) {
        decelSum += s.axLong.abs(); // ✅ Correct deceleration magnitude
        decelCount++;
      }
      speedSum += s.speedKmh;
    }

    avgAccel = accelCount > 0 ? accelSum / accelCount : 0.0;
    avgDecel = decelCount > 0 ? decelSum / decelCount : 0.0;
    avgSpeed = samples.isNotEmpty ? speedSum / samples.length : 0.0;

    startLat = samples.first.latitude;
    startLon = samples.first.longitude;
    endLat = samples.last.latitude;
    endLon = samples.last.longitude;

    setState(() {
      _samples = samples;
      _filtered = samples;
      _loading = false;
    });
  }

  void _filterSamples(String query) {
    final trimmed = query.trim().toLowerCase();
    if (trimmed.isEmpty) {
      setState(() => _filtered = _samples);
      return;
    }
    setState(() {
      _filtered = _samples
          .where((s) =>
              s.timestamp.toIso8601String().toLowerCase().contains(trimmed))
          .toList();
    });
  }

  void _openAdvancedData() {
    Navigator.push(
      context,
      MaterialPageRoute(
        builder: (_) => AdvancedDataScreen(
          db: widget.db,
          rideId: widget.rideId,
          samples: _samples,
        ),
      ),
    );
  }

  void _openOverview() {
    Navigator.push(
      context,
      MaterialPageRoute(
        builder: (_) => OverviewScreen(
          rideId: widget.rideId,
          samples: _samples,
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final accent = AppColors.accent;

    return Scaffold(
      backgroundColor: AppColors.background,
      appBar: AppBar(
        automaticallyImplyLeading: true,
        centerTitle: false, // ✅ Left-aligned as requested
        titleSpacing: 16,
        title: Text(
          'Ride #${widget.rideId}',
          style: const TextStyle(
            fontFamily: 'Orbitron',
            fontWeight: FontWeight.bold,
            fontSize: 20,
            color: AppColors.accent,
          ),
        ),
        actions: [
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 4),
            child: OutlinedButton(
              style: AppStyles.redOutlineButton,
              onPressed: _openOverview,
              child: const Text('Overview'),
            ),
          ),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 4),
            child: OutlinedButton(
              style: AppStyles.redOutlineButton,
              onPressed: _openAdvancedData,
              child: const Text('Advanced'),
            ),
          ),
          const SizedBox(width: 10),
        ],
      ),
      body: _loading
          ? const Center(
              child: CircularProgressIndicator(color: AppColors.accent),
            )
          : _samples.isEmpty
              ? const Center(
                  child: Text(
                    'No data recorded for this ride.',
                    style: TextStyle(color: AppColors.textSecondary),
                  ),
                )
              : Column(
                  children: [
                    _buildDashboard(accent),
                    const SizedBox(height: 10),
                    Padding(
                      padding: const EdgeInsets.symmetric(
                          horizontal: 16, vertical: 8),
                      child: TextField(
                        decoration: InputDecoration(
                          hintText: 'Search timestamp (HH:MM:SS)',
                          hintStyle: const TextStyle(
                              color: AppColors.textSecondary),
                          prefixIcon: const Icon(Icons.search,
                              color: AppColors.accent),
                          filled: true,
                          fillColor: const Color(0x22FF3B30),
                          border: OutlineInputBorder(
                            borderRadius: BorderRadius.circular(12),
                            borderSide:
                                const BorderSide(color: AppColors.accent),
                          ),
                        ),
                        style: const TextStyle(color: Colors.white),
                        onChanged: _filterSamples,
                      ),
                    ),
                    Expanded(
                      child: ListView.builder(
                        itemCount: _filtered.length,
                        itemBuilder: (context, i) {
                          final s = _filtered[i];
                          final t = TimeOfDay.fromDateTime(s.timestamp);
                          final time =
                              '${t.hour.toString().padLeft(2, '0')}:${t.minute.toString().padLeft(2, '0')}:${s.timestamp.second.toString().padLeft(2, '0')}';
                          return AnimatedContainer(
                            duration: const Duration(milliseconds: 250),
                            margin: const EdgeInsets.symmetric(
                                horizontal: 12, vertical: 6),
                            decoration: BoxDecoration(
                              color: AppColors.card.withOpacity(0.75),
                              borderRadius: BorderRadius.circular(14),
                              border: Border.all(
                                color: AppColors.accent.withOpacity(0.4),
                                width: 1.2,
                              ),
                              boxShadow: [
                                BoxShadow(
                                  color: AppColors.accent.withOpacity(0.15),
                                  blurRadius: 6,
                                  offset: const Offset(0, 3),
                                ),
                              ],
                            ),
                            child: Padding(
                              padding: const EdgeInsets.all(14),
                              child: Column(
                                crossAxisAlignment: CrossAxisAlignment.start,
                                children: [
                                  Text(
                                    time,
                                    style: const TextStyle(
                                      fontSize: 16,
                                      fontFamily: 'Orbitron',
                                      fontWeight: FontWeight.w600,
                                      color: AppColors.accent,
                                    ),
                                  ),
                                  const SizedBox(height: 6),
                                  Text(
                                    'Speed: ${s.speedKmh.toStringAsFixed(1)} km/h\n'
                                    'Accel: ${s.axLong.toStringAsFixed(2)} m/s² | '
                                    'Decel: ${s.axLong < -0.2 ? s.axLong.abs().toStringAsFixed(2) : "0.00"} m/s²\n'
                                    'Tilt: ${s.tiltDeg.toStringAsFixed(1)}°\n'
                                    'Lat: ${s.latitude.toStringAsFixed(5)}, Lon: ${s.longitude.toStringAsFixed(5)}\n'
                                    'Bearing: ${s.bearingDeg.toStringAsFixed(0)}°',
                                    style: const TextStyle(
                                      fontSize: 13,
                                      color: AppColors.textSecondary,
                                    ),
                                  ),
                                ],
                              ),
                            ),
                          );
                        },
                      ),
                    ),
                  ],
                ),
    );
  }

  Widget _buildDashboard(Color accent) {
    return Padding(
      padding: const EdgeInsets.all(16),
      child: Card(
        color: AppColors.card,
        elevation: 6,
        shadowColor: accent.withOpacity(0.4),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(14),
          side: BorderSide(color: accent.withOpacity(0.3)),
        ),
        child: Padding(
          padding: const EdgeInsets.all(18),
          child: Column(
            children: [
              Text(
                'Ride Summary',
                style: TextStyle(
                  fontFamily: 'Orbitron',
                  fontSize: 22,
                  fontWeight: FontWeight.bold,
                  color: accent,
                ),
              ),
              const SizedBox(height: 14),
              Divider(color: accent.withOpacity(0.4), thickness: 1),
              const SizedBox(height: 14),
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceAround,
                children: [
                  _metric('Avg Accel', '${avgAccel.toStringAsFixed(2)} m/s²'),
                  _metric('Avg Decel', '${avgDecel.toStringAsFixed(2)} m/s²'),
                  _metric('Avg Speed', '${avgSpeed.toStringAsFixed(2)} km/h'),
                ],
              ),
              const SizedBox(height: 16),
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                children: [
                  _metric(
                    'Start',
                    startLat != null && startLon != null
                        ? '${startLat!.toStringAsFixed(4)}, ${startLon!.toStringAsFixed(4)}'
                        : null,
                  ),
                  _metric(
                    'End',
                    endLat != null && endLon != null
                        ? '${endLat!.toStringAsFixed(4)}, ${endLon!.toStringAsFixed(4)}'
                        : null,
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _metric(String label, String? value) {
    return Column(
      children: [
        Text(
          label,
          style: const TextStyle(
            fontWeight: FontWeight.w500,
            fontSize: 14,
            color: AppColors.textSecondary,
          ),
        ),
        const SizedBox(height: 4),
        Text(
          value ?? '—',
          style: const TextStyle(
            fontFamily: 'Orbitron',
            fontWeight: FontWeight.bold,
            fontSize: 15,
            color: AppColors.accent,
          ),
        ),
      ],
    );
  }
}
