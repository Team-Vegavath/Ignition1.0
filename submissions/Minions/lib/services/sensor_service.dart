import 'dart:async';
import 'dart:math';
import 'package:vector_math/vector_math_64.dart' as vm;
import 'package:geolocator/geolocator.dart';
import 'package:sensors_plus/sensors_plus.dart';
import '../models/sample.dart';
import 'db_service.dart';

class SensorService {
  final DbService db;
  final _sampleCtrl = StreamController<Sample>.broadcast();
  Stream<Sample> get stream => _sampleCtrl.stream;

  StreamSubscription<AccelerometerEvent>? _accelSub;
  StreamSubscription<MagnetometerEvent>? _magSub;
  StreamSubscription<Position>? _gpsSub;

  vm.Vector3 _g = vm.Vector3(0, 0, 9.81); // gravity estimate
  vm.Vector3 _mag = vm.Vector3.zero(); // magnetic field
  vm.Vector3 _accelRaw = vm.Vector3.zero(); // latest accel reading

  final double _alpha = 0.8; // LPF smoothing for gravity

  double _speedKmh = 0.0;
  double _bearingDeg = 0.0;
  double _lat = 0.0, _lon = 0.0;
  double _lastSpeedKmh = 0.0;
  double _lastDecel = 0.0;
  DateTime _lastSpeedTime = DateTime.now();
  DateTime _lastGpsUpdate = DateTime.fromMillisecondsSinceEpoch(0);

  int? _currentRideId;

  SensorService(this.db);

  bool get isRunning => _currentRideId != null;

  Future<void> _ensureLocationPermission() async {
    bool serviceEnabled = await Geolocator.isLocationServiceEnabled();
    if (!serviceEnabled) await Geolocator.openLocationSettings();
    var permission = await Geolocator.checkPermission();
    if (permission == LocationPermission.denied) {
      permission = await Geolocator.requestPermission();
    }
    if (permission == LocationPermission.deniedForever) {
      await Geolocator.openAppSettings();
    }
  }

  Future<void> startRide() async {
    if (_currentRideId != null) return;
    await _ensureLocationPermission();

    try {
      final pos = await Geolocator.getCurrentPosition(
        desiredAccuracy: LocationAccuracy.bestForNavigation,
      );
      _lat = pos.latitude;
      _lon = pos.longitude;
      _speedKmh = pos.speed * 3.6;
      _bearingDeg = pos.heading.isNaN ? 0.0 : pos.heading;
      _lastGpsUpdate = DateTime.now();
      _lastSpeedKmh = _speedKmh;
      _lastSpeedTime = DateTime.now();
      print('📍 Initial GPS fix: $_lat,$_lon speed=$_speedKmh bearing=$_bearingDeg');
    } catch (_) {
      print('⚠️ No initial GPS fix yet');
    }

    _currentRideId = await db.createRide();

    // Accelerometer stream (includes gravity)
    _accelSub = accelerometerEvents.listen((e) {
      final raw = vm.Vector3(e.x, e.y, e.z);
      _g = vm.Vector3(
        _alpha * _g.x + (1 - _alpha) * raw.x,
        _alpha * _g.y + (1 - _alpha) * raw.y,
        _alpha * _g.z + (1 - _alpha) * raw.z,
      );
      _accelRaw = raw;
      _emitSampleIfPossible();
    });

    // Magnetometer (for heading correction)
    _magSub = magnetometerEvents.listen((e) {
      _mag = vm.Vector3(e.x, e.y, e.z);
    });

    // GPS continuous updates
    _gpsSub = Geolocator.getPositionStream(
      locationSettings: const LocationSettings(
        accuracy: LocationAccuracy.bestForNavigation,
        distanceFilter: 0,
      ),
    ).listen((pos) {
      final now = DateTime.now();
      final dt = max(1e-3, now.difference(_lastSpeedTime).inMilliseconds / 1000.0);
      final curSpeed = pos.speed * 3.6;

      // ✅ Compute deceleration from speed drop
      final decel = (_lastSpeedKmh - curSpeed) / 3.6 / dt;
      _lastDecel = decel > 0 ? decel : 0.0;

      _lat = pos.latitude;
      _lon = pos.longitude;
      _speedKmh = curSpeed;
      if (!pos.heading.isNaN) _bearingDeg = pos.heading;

      _lastSpeedKmh = curSpeed;
      _lastSpeedTime = now;
      _lastGpsUpdate = now;
    });
  }

  Future<void> stopRide() async {
    await _accelSub?.cancel();
    await _magSub?.cancel();
    await _gpsSub?.cancel();
    _accelSub = _magSub = _gpsSub = null;

    if (_currentRideId != null) {
      await db.endRide(_currentRideId!);
      _currentRideId = null;
    }
  }

  void dispose() {
    _sampleCtrl.close();
  }

  // -----------------------------------------------------------

  void _emitSampleIfPossible() {
    final rideId = _currentRideId;
    if (rideId == null) return;

    // Skip if GPS is stale
    if (DateTime.now().difference(_lastGpsUpdate).inSeconds > 5) return;

    // --- Compute world-aligned acceleration ---
    final gNorm = _g.normalized();
    final magNorm = _mag.normalized();

    // East-North-Up coordinate system
    final east = magNorm.cross(gNorm).normalized();
    final north = gNorm.cross(east).normalized();
    final rotMatrix = vm.Matrix3.columns(east, north, -gNorm); // device→world

    // Remove gravity from raw accel
    final accelNoGrav = _accelRaw - _g;

    // Rotate to world frame
    final accelWorld = rotMatrix.transposed() * accelNoGrav;

    // Clamp absurd spikes (due to jitter)
    double ax = accelWorld.x.clamp(-15.0, 15.0);
    double ay = accelWorld.y.clamp(-15.0, 15.0);
    double az = accelWorld.z.clamp(-15.0, 15.0);

    // Compute longitudinal, lateral, vertical relative to heading
    final headingRad = _bearingDeg * pi / 180.0;
    final forward = vm.Vector3(sin(headingRad), cos(headingRad), 0);
    final left = vm.Vector3(-cos(headingRad), sin(headingRad), 0);
    final up = vm.Vector3(0, 0, 1);

    final axLong = forward.dot(accelWorld);
    final ayLat = left.dot(accelWorld);
    final azUp = up.dot(accelWorld);

    // Downsample to one sample per second
    final now = DateTime.now();
    if (now.millisecond > 100) return; // only emit near full second

    final s = Sample(
      rideId: rideId,
      timestamp: now,
      axLong: axLong,
      ayLat: ayLat,
      azUp: azUp,
      ax: ax,
      ay: ay,
      az: az,
      speedKmh: _speedKmh,
      latitude: _lat,
      longitude: _lon,
      bearingDeg: _bearingDeg,
      decel: _lastDecel, // ✅ store computed deceleration
    );

    _sampleCtrl.add(s);
    db.insertSample(s);
  }
}
