import 'package:path/path.dart' as p;
import 'package:path_provider/path_provider.dart';
import 'package:sqflite/sqflite.dart';
import '../models/ride.dart';
import '../models/sample.dart';

class DbService {
  Database? _db;

  Future<void> init() async {
    final dir = await getApplicationDocumentsDirectory();
    final dbPath = p.join(dir.path, 'telemetry.db');
    _db = await openDatabase(
      dbPath,
      version: 3, // ✅ bumped to v3
      onCreate: (db, _) async {
        await db.execute('PRAGMA foreign_keys = ON;');

        await db.execute('''
          CREATE TABLE rides(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            start_time TEXT NOT NULL,
            end_time TEXT,
            distance_m REAL DEFAULT 0,
            avg_speed_kmh REAL DEFAULT 0,
            sample_count INTEGER DEFAULT 0
          );
        ''');

        await db.execute('''
          CREATE TABLE samples(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            ride_id INTEGER NOT NULL,
            timestamp TEXT NOT NULL,
            ax_long REAL,
            ay_lat REAL,
            az_up REAL,
            ax REAL,
            ay REAL,
            az REAL,
            speed_kmh REAL,
            lat REAL,
            lon REAL,
            bearing_deg REAL,
            decel REAL,  -- ✅ new deceleration column
            FOREIGN KEY(ride_id) REFERENCES rides(id) ON DELETE CASCADE
          );
        ''');

        await db.execute('CREATE INDEX idx_samples_ride ON samples(ride_id);');
      },
      onUpgrade: (db, oldV, newV) async {
        await db.execute('PRAGMA foreign_keys = ON;');

        if (oldV < 2) {
          // legacy cleanup if from v1
          await db.execute('DROP TABLE IF EXISTS telemetry;');
        }

        if (oldV < 3) {
          // ✅ add decel column if upgrading from older schema
          try {
            await db.execute('ALTER TABLE samples ADD COLUMN decel REAL;');
          } catch (_) {
            // If column already exists or table recreated, ignore
          }
        }
      },
    );
  }

  Future<int> createRide() async {
    final id = await _db!.insert('rides', {
      'start_time': DateTime.now().toIso8601String(),
      'sample_count': 0,
    });
    return id;
  }

  Future<void> endRide(int rideId) async {
    await _db!.update(
      'rides',
      {'end_time': DateTime.now().toIso8601String()},
      where: 'id = ?',
      whereArgs: [rideId],
    );
  }

  Future<void> insertSample(Sample s) async {
    await _db!.insert('samples', s.toMap());
    await _db!.rawUpdate(
      'UPDATE rides SET sample_count = sample_count + 1 WHERE id = ?',
      [s.rideId],
    );
  }

  Future<List<Ride>> listRides() async {
    final rows = await _db!.rawQuery('''
      SELECT id, start_time, end_time, distance_m, avg_speed_kmh, sample_count
      FROM rides
      ORDER BY COALESCE(end_time, start_time) DESC
    ''');
    return rows.map((e) => Ride.fromMap(e)).toList();
  }

  Future<List<Sample>> samplesForRide(int rideId, {int limit = 500}) async {
    final rows = await _db!.query(
      'samples',
      where: 'ride_id = ?',
      whereArgs: [rideId],
      orderBy: 'timestamp ASC',
      limit: limit,
    );
    return rows.map((e) => Sample.fromMap(e)).toList();
  }
}
