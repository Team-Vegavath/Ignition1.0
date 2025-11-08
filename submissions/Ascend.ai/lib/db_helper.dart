import 'package:sqflite/sqflite.dart';
import 'package:path/path.dart';
import 'package:path_provider/path_provider.dart';
import 'dart:io';

class DBHelper {
  static Database? _db;

  Future<Database> get database async {
    if (_db != null) return _db!;
    _db = await initDB();
    return _db!;
  }

  Future<Database> initDB() async {
    Directory dir = await getApplicationDocumentsDirectory();
    String path = join(dir.path, 'rider_logs.db');
    return await openDatabase(path, version: 1, onCreate: (db, version) async {
      await db.execute('''
        CREATE TABLE logs(
          id INTEGER PRIMARY KEY AUTOINCREMENT,
          timestamp TEXT,
          accelX REAL,
          accelY REAL,
          accelZ REAL,
          lean REAL,
          mode TEXT,
          source TEXT,
          speed REAL,
          signal TEXT,
          latitude TEXT,
          longitude TEXT,
          satellites INTEGER
        )
      ''');
    });
  }

  Future<void> insertData(Map<String, dynamic> data) async {
    final db = await database;
    await db.insert('logs', data);
  }

  Future<List<Map<String, dynamic>>> getAllLogs() async {
    final db = await database;
    return await db.query('logs', orderBy: 'id DESC');
  }

  Future<void> clearLogs() async {
    final db = await database;
    await db.delete('logs');
  }
}

