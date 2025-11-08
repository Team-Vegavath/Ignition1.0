import 'package:flutter/material.dart';
import 'screens/home_screen.dart';
import 'services/db_service.dart';
import 'services/sensor_service.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  final db = DbService();
  await db.init();
  final sensor = SensorService(db);
  runApp(KinetiqApp(db: db, sensor: sensor));
}

class KinetiqApp extends StatelessWidget {
  final DbService db;
  final SensorService sensor;
  const KinetiqApp({super.key, required this.db, required this.sensor});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Kinetiq',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        fontFamily: 'Exo2',
        brightness: Brightness.dark,
        scaffoldBackgroundColor: const Color(0xFF0D0D0D),
        cardColor: const Color(0xFF141414),
        primaryColor: const Color(0xFFFF3B30),
        appBarTheme: const AppBarTheme(
          backgroundColor: Color(0xFF0D0D0D),
          elevation: 4,
          shadowColor: Color(0x44FF3B30),
          centerTitle: true,
          titleTextStyle: TextStyle(
            fontFamily: 'Orbitron',
            color: Colors.white,
            fontWeight: FontWeight.bold,
            fontSize: 20,
          ),
        ),
        elevatedButtonTheme: ElevatedButtonThemeData(
          style: ElevatedButton.styleFrom(
            backgroundColor: const Color(0xFFFF3B30),
            foregroundColor: Colors.white,
            textStyle: const TextStyle(
              fontWeight: FontWeight.bold,
              fontSize: 16,
              letterSpacing: 0.5,
            ),
            shape: RoundedRectangleBorder(
              borderRadius: BorderRadius.circular(12),
            ),
          ),
        ),
        textTheme: const TextTheme(
          bodyLarge: TextStyle(color: Colors.white, fontSize: 16),
          bodyMedium: TextStyle(color: Color(0xFFBBBBBB), fontSize: 14),
          titleLarge: TextStyle(
            fontFamily: 'Orbitron',
            color: Colors.white,
            fontWeight: FontWeight.w700,
          ),
        ),
      ),
      home: HomeScreen(db: db, sensor: sensor),
    );
  }
}
