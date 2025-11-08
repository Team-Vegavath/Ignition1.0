class Telemetry {
  final DateTime timestamp;
  final double accelX;
  final double accelY;
  final double accelZ;
  final double accelMag;
  final double speed;
  final double latitude;
  final double longitude;

  Telemetry({
    required this.timestamp,
    required this.accelX,
    required this.accelY,
    required this.accelZ,
    required this.accelMag,
    required this.speed,
    required this.latitude,
    required this.longitude,
  });

  Map<String, dynamic> toMap() {
    return {
      'timestamp': timestamp.toIso8601String(),
      'accelX': accelX,
      'accelY': accelY,
      'accelZ': accelZ,
      'accelMag': accelMag,
      'speed': speed,
      'latitude': latitude,
      'longitude': longitude,
    };
  }
}
