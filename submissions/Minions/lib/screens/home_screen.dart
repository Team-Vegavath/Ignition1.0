import 'dart:ui';
import 'package:flutter/material.dart';
import '../services/db_service.dart';
import '../services/sensor_service.dart';
import 'rides_list_screen.dart';
import '../theme/app_styles.dart';

class HomeScreen extends StatelessWidget {
  final DbService db;
  final SensorService sensor;
  const HomeScreen({super.key, required this.db, required this.sensor});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: AppColors.background,
      body: Stack(
        fit: StackFit.expand,
        children: [
          // 🔴 subtle red gradient background
          Container(
            decoration: const BoxDecoration(
              gradient: LinearGradient(
                colors: [Color(0xFF0D0D0D), Color(0xFF1A0000)],
                begin: Alignment.topCenter,
                end: Alignment.bottomCenter,
              ),
            ),
          ),

          // 🔴 soft red glow
          Positioned(
            top: 180,
            left: 100,
            child: Container(
              width: 200,
              height: 200,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                gradient: RadialGradient(
                  colors: [
                    AppColors.accent.withOpacity(0.3),
                    Colors.transparent,
                  ],
                ),
              ),
            ),
          ),

          // 🌐 Main content
          SafeArea(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                const Spacer(),

                // 🔥 App name
                Text(
                  'KINETIQ',
                  style: TextStyle(
                    fontFamily: 'Orbitron',
                    fontWeight: FontWeight.w900,
                    fontSize: 42,
                    letterSpacing: 2,
                    color: AppColors.accent,
                    shadows: [
                      Shadow(
                        color: AppColors.accent.withOpacity(0.5),
                        blurRadius: 10,
                      ),
                    ],
                  ),
                ),
                const SizedBox(height: 16),

                // Subtitle tagline
                Text(
                  'Ride Telemetry Reimagined',
                  style: TextStyle(
                    color: AppColors.textSecondary.withOpacity(0.8),
                    fontSize: 16,
                    fontStyle: FontStyle.italic,
                  ),
                ),

                const Spacer(),

                // ⚡ Glassy “My Rides” button
                Padding(
                  padding: const EdgeInsets.symmetric(horizontal: 40),
                  child: ClipRRect(
                    borderRadius: BorderRadius.circular(18),
                    child: BackdropFilter(
                      filter: ImageFilter.blur(sigmaX: 10, sigmaY: 10),
                      child: Container(
                        width: double.infinity,
                        padding: const EdgeInsets.symmetric(vertical: 16),
                        decoration: BoxDecoration(
                          borderRadius: BorderRadius.circular(18),
                          border: Border.all(
                            color: AppColors.accent.withOpacity(0.6),
                            width: 1.5,
                          ),
                          gradient: LinearGradient(
                            colors: [
                              Colors.red.withOpacity(0.25),
                              Colors.redAccent.withOpacity(0.1),
                            ],
                            begin: Alignment.topLeft,
                            end: Alignment.bottomRight,
                          ),
                        ),
                        child: InkWell(
                          onTap: () {
                            Navigator.push(
                              context,
                              MaterialPageRoute(
                                builder: (_) => RidesListScreen(
                                  db: db,
                                  sensor: sensor,
                                ),
                              ),
                            );
                          },
                          child: Center(
                            child: Text(
                              'My Rides',
                              style: TextStyle(
                                fontFamily: 'Orbitron',
                                fontWeight: FontWeight.bold,
                                fontSize: 20,
                                color: AppColors.accent,
                                letterSpacing: 1.2,
                              ),
                            ),
                          ),
                        ),
                      ),
                    ),
                  ),
                ),

                const Spacer(),
                // ⚙️ Removed “Powered by HSP” footer for cleaner look
              ],
            ),
          ),
        ],
      ),
    );
  }
}
