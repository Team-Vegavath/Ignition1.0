class Ride {
  final int id;
  final DateTime startTime;
  final DateTime? endTime;
  final double distanceMeters;
  final double avgSpeedKmh;
  final int sampleCount;

  Ride({
    required this.id,
    required this.startTime,
    this.endTime,
    this.distanceMeters = 0.0,
    this.avgSpeedKmh = 0.0,
    this.sampleCount = 0,
  });

  factory Ride.fromMap(Map<String, dynamic> m) => Ride(
    id: m['id'] as int,
    startTime: DateTime.parse(m['start_time'] as String),
    endTime: m['end_time'] != null ? DateTime.parse(m['end_time'] as String) : null,
    distanceMeters: (m['distance_m'] as num?)?.toDouble() ?? 0,
    avgSpeedKmh: (m['avg_speed_kmh'] as num?)?.toDouble() ?? 0,
    sampleCount: (m['sample_count'] as int?) ?? 0,
  );
}
