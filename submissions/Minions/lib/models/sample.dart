class Sample {
  final int? id;
  final int rideId;
  final DateTime timestamp;

  // Vehicle/world-aligned acceleration (m/s²)
  final double axLong; // forward/back
  final double ayLat;  // left/right
  final double azUp;   // up/down

  // Raw device linear acceleration (optional debug)
  final double ax;
  final double ay;
  final double az;

  // GPS
  final double speedKmh;
  final double latitude;
  final double longitude;
  final double bearingDeg;

  // Derived values
  final double decel;   // Deceleration magnitude
  final double tiltDeg; // Tilt angle (degrees from vertical)

  Sample({
    this.id,
    required this.rideId,
    required this.timestamp,
    required this.axLong,
    required this.ayLat,
    required this.azUp,
    required this.ax,
    required this.ay,
    required this.az,
    required this.speedKmh,
    required this.latitude,
    required this.longitude,
    required this.bearingDeg,
    this.decel = 0.0,
    this.tiltDeg = 0.0,
  });

  Map<String, dynamic> toMap() => {
        'id': id,
        'ride_id': rideId,
        'timestamp': timestamp.toIso8601String(),
        'ax_long': axLong,
        'ay_lat': ayLat,
        'az_up': azUp,
        'ax': ax,
        'ay': ay,
        'az': az,
        'speed_kmh': speedKmh,
        'lat': latitude,
        'lon': longitude,
        'bearing_deg': bearingDeg,
        'decel': decel,
        'tilt_deg': tiltDeg, // ✅ added
      };

  factory Sample.fromMap(Map<String, dynamic> m) => Sample(
        id: m['id'] as int?,
        rideId: m['ride_id'] as int,
        timestamp: DateTime.parse(m['timestamp'] as String),
        axLong: (m['ax_long'] ?? 0).toDouble(),
        ayLat: (m['ay_lat'] ?? 0).toDouble(),
        azUp: (m['az_up'] ?? 0).toDouble(),
        ax: (m['ax'] ?? 0).toDouble(),
        ay: (m['ay'] ?? 0).toDouble(),
        az: (m['az'] ?? 0).toDouble(),
        speedKmh: (m['speed_kmh'] ?? 0).toDouble(),
        latitude: (m['lat'] ?? 0).toDouble(),
        longitude: (m['lon'] ?? 0).toDouble(),
        bearingDeg: (m['bearing_deg'] ?? 0).toDouble(),
        decel: (m['decel'] ?? 0).toDouble(),
        tiltDeg: (m['tilt_deg'] ?? 0).toDouble(), // ✅ added
      );
}
