import 'package:flutter/material.dart';

class AppColors {
  static const Color background = Color(0xFF0D0D0D);
  static const Color card = Color(0xFF141414);
  static const Color accent = Color(0xFFFF3B30);
  static const Color textPrimary = Colors.white;
  static const Color textSecondary = Color(0xFFBBBBBB);
}

class AppStyles {
  static ButtonStyle redOutlineButton = OutlinedButton.styleFrom(
    side: const BorderSide(color: AppColors.accent, width: 1.5),
    foregroundColor: AppColors.accent,
    shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
    padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 8),
  );

  static ButtonStyle solidRedButton = ElevatedButton.styleFrom(
    backgroundColor: AppColors.accent,
    foregroundColor: Colors.white,
    textStyle: const TextStyle(fontWeight: FontWeight.bold),
    shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
  );
}
