import 'package:flutter/material.dart';
import '../services/db_service.dart';
import '../services/sensor_service.dart';
import '../models/ride.dart';
import 'ride_record_screen.dart';
import 'ride_detail_screen.dart';
import '../theme/app_styles.dart';

class RidesListScreen extends StatefulWidget {
  final DbService db;
  final SensorService sensor;
  const RidesListScreen({super.key, required this.db, required this.sensor});

  @override
  State<RidesListScreen> createState() => _RidesListScreenState();
}

class _RidesListScreenState extends State<RidesListScreen> {
  List<Ride> rides = [];
  bool loading = true;

  @override
  void initState() {
    super.initState();
    _loadRides();
  }

  Future<void> _loadRides() async {
    final data = await widget.db.listRides();
    setState(() {
      rides = data;
      loading = false;
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: AppColors.background,
      appBar: AppBar(
        backgroundColor: AppColors.background,
        elevation: 3,
        title: const Text(
          'My Rides',
          style: TextStyle(
            fontFamily: 'Orbitron',
            color: AppColors.accent,
            fontWeight: FontWeight.bold,
            fontSize: 22,
          ),
        ),
        centerTitle: true,
      ),
      body: loading
          ? const Center(
              child: CircularProgressIndicator(color: AppColors.accent))
          : rides.isEmpty
              ? const Center(
                  child: Text(
                    'No rides yet.',
                    style: TextStyle(
                        color: AppColors.textSecondary, fontSize: 16),
                  ),
                )
              : RefreshIndicator(
                  color: AppColors.accent,
                  onRefresh: _loadRides,
                  child: ListView.builder(
                    padding: const EdgeInsets.symmetric(
                        horizontal: 16, vertical: 12),
                    itemCount: rides.length,
                    itemBuilder: (context, i) {
                      final ride = rides[i];
                      final isActive = ride.endTime == null;

                      return GestureDetector(
                        onTap: () {
                          if (ride.endTime != null) {
                            Navigator.push(
                              context,
                              MaterialPageRoute(
                                builder: (_) => RideDetailScreen(
                                  db: widget.db,
                                  rideId: ride.id,
                                ),
                              ),
                            );
                          }
                        },
                        child: AnimatedContainer(
                          duration: const Duration(milliseconds: 250),
                          margin: const EdgeInsets.only(bottom: 14),
                          decoration: BoxDecoration(
                            color: AppColors.card.withOpacity(0.7),
                            borderRadius: BorderRadius.circular(14),
                            border: Border.all(
                              color: isActive
                                  ? AppColors.accent
                                  : AppColors.accent.withOpacity(0.3),
                              width: 1.2,
                            ),
                            boxShadow: [
                              BoxShadow(
                                color: AppColors.accent.withOpacity(0.2),
                                blurRadius: 6,
                                offset: const Offset(0, 3),
                              ),
                            ],
                          ),
                          child: Padding(
                            padding: const EdgeInsets.all(14),
                            child: Row(
                              mainAxisAlignment: MainAxisAlignment.spaceBetween,
                              children: [
                                Expanded(
                                  child: Column(
                                    crossAxisAlignment:
                                        CrossAxisAlignment.start,
                                    children: [
                                      Text(
                                        'Ride #${ride.id}${isActive ? " (Active)" : ""}',
                                        style: TextStyle(
                                          fontFamily: 'Orbitron',
                                          fontWeight: FontWeight.bold,
                                          fontSize: 18,
                                          color: AppColors.accent,
                                        ),
                                      ),
                                      const SizedBox(height: 6),
                                      Text(
                                        'Start: ${_formatTime(ride.startTime!)}',
                                        style: const TextStyle(
                                            color: AppColors.textSecondary,
                                            fontSize: 14),
                                      ),
                                      if (ride.endTime != null)
                                        Text(
                                          'End: ${_formatTime(ride.endTime!)}',
                                          style: const TextStyle(
                                              color: AppColors.textSecondary,
                                              fontSize: 14),
                                        ),
                                    ],
                                  ),
                                ),
                                Text(
                                  '${ride.sampleCount} samples',
                                  style: const TextStyle(
                                    fontSize: 13,
                                    color: AppColors.textSecondary,
                                  ),
                                ),
                              ],
                            ),
                          ),
                        ),
                      );
                    },
                  ),
                ),
      floatingActionButton: FloatingActionButton.extended(
        backgroundColor: AppColors.accent,
        foregroundColor: Colors.white,
        elevation: 10,
        label: const Text('New Ride'),
        icon: const Icon(Icons.add),
        onPressed: () async {
          await Navigator.push(
            context,
            MaterialPageRoute(
              builder: (_) =>
                  RideRecordScreen(sensor: widget.sensor, db: widget.db),
            ),
          );
          _loadRides();
        },
      ),
      floatingActionButtonLocation: FloatingActionButtonLocation.centerFloat,
    );
  }

  String _formatTime(DateTime t) {
    final h = t.hour.toString().padLeft(2, '0');
    final m = t.minute.toString().padLeft(2, '0');
    final s = t.second.toString().padLeft(2, '0');
    return '$h:$m:$s';
  }
}
