import 'dart:io';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';
import 'package:path_provider/path_provider.dart';
import 'package:printing/printing.dart';
import '../models/sample.dart';
import '../services/db_service.dart';
import '../theme/app_styles.dart';
import '../utils/pdf_generator.dart';

class AdvancedDataScreen extends StatelessWidget {
  final DbService db;
  final int rideId;
  final List<Sample> samples;

  const AdvancedDataScreen({
    super.key,
    required this.db,
    required this.rideId,
    required this.samples,
  });

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: AppColors.background,
      appBar: AppBar(
        title: Text(
          'Advanced Data – Ride #$rideId',
          style: const TextStyle(
            fontFamily: 'Orbitron',
            color: AppColors.accent,
            fontWeight: FontWeight.bold,
          ),
        ),
        centerTitle: true,
        backgroundColor: AppColors.background,
      ),
      body: Padding(
        padding: const EdgeInsets.all(12),
        child: samples.isEmpty
            ? const Center(
                child: Text(
                  'No data recorded for this ride.',
                  style: TextStyle(color: Colors.white70),
                ),
              )
            : ListView(
                children: [
                  _buildGraphCard(
                    title: 'Speed over Time',
                    color: Colors.amberAccent,
                    unit: 'km/h',
                    data: samples.map((s) => s.speedKmh).toList(),
                  ),
                  _buildGraphCard(
                    title: 'Longitudinal Acceleration (X)',
                    color: Colors.blueAccent,
                    unit: 'm/s²',
                    data: samples.map((s) => s.axLong).toList(),
                  ),
                  _buildGraphCard(
                    title: 'Lateral Acceleration (Y)',
                    color: Colors.greenAccent,
                    unit: 'm/s²',
                    data: samples.map((s) => s.ayLat).toList(),
                  ),
                  _buildGraphCard(
                    title: 'Vertical Acceleration (Z)',
                    color: Colors.purpleAccent,
                    unit: 'm/s²',
                    data: samples.map((s) => s.azUp).toList(),
                  ),
                  _buildGraphCard(
                    title: 'Deceleration',
                    color: Colors.redAccent,
                    unit: 'm/s²',
                    data: samples.map((s) => s.decel).toList(),
                  ),
                  const SizedBox(height: 30),

                  // ✅ PDF + CSV Buttons
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                    children: [
                      ElevatedButton.icon(
                        onPressed: () => _generatePdf(context),
                        icon: const Icon(Icons.picture_as_pdf, color: Colors.black),
                        label: const Text(
                          'Generate PDF',
                          style: TextStyle(
                            fontFamily: 'Orbitron',
                            fontWeight: FontWeight.bold,
                            color: Colors.black,
                          ),
                        ),
                        style: ElevatedButton.styleFrom(
                          backgroundColor: AppColors.accent,
                          padding: const EdgeInsets.symmetric(
                              horizontal: 18, vertical: 14),
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(10),
                          ),
                        ),
                      ),
                      ElevatedButton.icon(
                        onPressed: () => _exportCsv(context),
                        icon: const Icon(Icons.download, color: Colors.black),
                        label: const Text(
                          'Export CSV',
                          style: TextStyle(
                            fontFamily: 'Orbitron',
                            fontWeight: FontWeight.bold,
                            color: Colors.black,
                          ),
                        ),
                        style: ElevatedButton.styleFrom(
                          backgroundColor: Colors.amberAccent,
                          padding: const EdgeInsets.symmetric(
                              horizontal: 18, vertical: 14),
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(10),
                          ),
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 40),
                ],
              ),
      ),
    );
  }

  // ------------------------------------------------------------

  Widget _buildGraphCard({
    required String title,
    required Color color,
    required String unit,
    required List<double> data,
  }) {
    final chartData = <FlSpot>[
      for (int i = 0; i < data.length; i++) FlSpot(i.toDouble(), data[i])
    ];

    double minY = data.reduce((a, b) => a < b ? a : b);
    double maxY = data.reduce((a, b) => a > b ? a : b);
    if ((maxY - minY).abs() < 0.0001) {
      maxY += 1;
      minY -= 1;
    }
    final double interval = ((maxY - minY) / 4).clamp(0.5, 100.0);

    return Card(
      margin: const EdgeInsets.symmetric(vertical: 14),
      elevation: 6,
      color: const Color(0xFF121212),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              title,
              style: const TextStyle(
                fontSize: 17,
                fontWeight: FontWeight.w700,
                color: Colors.white70,
              ),
            ),
            const SizedBox(height: 10),
            SizedBox(
              height: 220,
              child: LineChart(
                LineChartData(
                  minY: minY,
                  maxY: maxY,
                  backgroundColor: Colors.black12,
                  gridData: FlGridData(
                    show: true,
                    horizontalInterval: interval,
                    getDrawingHorizontalLine: (value) => FlLine(
                      color: Colors.white12,
                      strokeWidth: 1,
                    ),
                  ),
                  borderData: FlBorderData(show: true),
                  titlesData: FlTitlesData(
                    leftTitles: AxisTitles(
                      axisNameWidget: Text(
                        'Y-axis: $unit',
                        style: const TextStyle(
                            fontSize: 11, color: Colors.white54),
                      ),
                      axisNameSize: 24,
                      sideTitles: SideTitles(
                        showTitles: true,
                        reservedSize: 42,
                        interval: interval,
                        getTitlesWidget: (value, meta) => Text(
                          value.toStringAsFixed(1),
                          style: const TextStyle(
                            color: Colors.white54,
                            fontSize: 10,
                          ),
                        ),
                      ),
                    ),
                    bottomTitles: AxisTitles(
                      axisNameWidget: const Text(
                        'Time (s)',
                        style: TextStyle(
                          fontSize: 11,
                          color: Colors.white54,
                        ),
                      ),
                      axisNameSize: 30,
                      sideTitles: SideTitles(
                        showTitles: true,
                        reservedSize: 30,
                        interval:
                            (chartData.length / 5).clamp(1, 50).toDouble(),
                        getTitlesWidget: (value, meta) => Text(
                          value.toInt().toString(),
                          style: const TextStyle(
                            color: Colors.white54,
                            fontSize: 10,
                          ),
                        ),
                      ),
                    ),
                    topTitles: const AxisTitles(
                        sideTitles: SideTitles(showTitles: false)),
                    rightTitles: const AxisTitles(
                        sideTitles: SideTitles(showTitles: false)),
                  ),
                  lineTouchData: LineTouchData(
                    enabled: true,
                    touchTooltipData: LineTouchTooltipData(
                      getTooltipColor: (_) => Colors.black87,
                      tooltipPadding: const EdgeInsets.all(8),
                      tooltipRoundedRadius: 8,
                      getTooltipItems: (touchedSpots) {
                        return touchedSpots.map((spot) {
                          return LineTooltipItem(
                            't=${spot.x.toInt()}s\n${spot.y.toStringAsFixed(2)} $unit',
                            const TextStyle(
                              color: Colors.white,
                              fontWeight: FontWeight.bold,
                            ),
                          );
                        }).toList();
                      },
                    ),
                  ),
                  lineBarsData: [
                    LineChartBarData(
                      isCurved: true,
                      color: color,
                      spots: chartData,
                      barWidth: 2,
                      dotData: const FlDotData(show: false),
                      belowBarData: BarAreaData(
                        show: true,
                        color: color.withOpacity(0.2),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Future<void> _exportCsv(BuildContext context) async {
    try {
      final buffer = StringBuffer();
      buffer.writeln(
          'timestamp,axLong,ayLat,azUp,ax,ay,az,speedKmh,latitude,longitude,bearingDeg,decel');

      for (final s in samples) {
        buffer.writeln(
            '${s.timestamp.toIso8601String()},'
            '${s.axLong.toStringAsFixed(3)},'
            '${s.ayLat.toStringAsFixed(3)},'
            '${s.azUp.toStringAsFixed(3)},'
            '${s.ax.toStringAsFixed(3)},'
            '${s.ay.toStringAsFixed(3)},'
            '${s.az.toStringAsFixed(3)},'
            '${s.speedKmh.toStringAsFixed(2)},'
            '${s.latitude.toStringAsFixed(6)},'
            '${s.longitude.toStringAsFixed(6)},'
            '${s.bearingDeg.toStringAsFixed(2)},'
            '${s.decel.toStringAsFixed(3)}');
      }

      final dir = await getApplicationDocumentsDirectory();
      final path = '${dir.path}/ride_${rideId}_data.csv';
      final file = File(path);
      await file.writeAsString(buffer.toString());

      if (context.mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('✅ CSV saved to: $path')),
        );
      }
    } catch (e) {
      if (context.mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('❌ Failed to export CSV: $e')),
        );
      }
    }
  }

  Future<void> _generatePdf(BuildContext context) async {
    try {
      double avgAccel = samples
              .where((s) => s.axLong > 0)
              .map((s) => s.axLong)
              .fold(0.0, (a, b) => a + b) /
          (samples.where((s) => s.axLong > 0).length + 1);
      double avgDecel = samples
              .where((s) => s.decel > 0)
              .map((s) => s.decel)
              .fold(0.0, (a, b) => a + b) /
          (samples.where((s) => s.decel > 0).length + 1);
      double avgSpeed = samples.map((s) => s.speedKmh).fold(0.0, (a, b) => a + b) / samples.length;

      final Uint8List pdfBytes = await PdfGenerator.generateRideReport(
        rideId: rideId,
        samples: samples,
        avgAccel: avgAccel,
        avgDecel: avgDecel,
        avgSpeed: avgSpeed,
      );

      await Printing.sharePdf(
        bytes: pdfBytes,
        filename: 'KINETIQ_Ride_${rideId}.pdf',
      );
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('❌ Failed to generate PDF: $e')),
      );
    }
  }
}
