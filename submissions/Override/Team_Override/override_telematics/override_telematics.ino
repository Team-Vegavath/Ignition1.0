/*
  ESP32 + MPU6050 - motion/brake/accel/decel/posture/mode detection (with Bluetooth)
  SDA -> 21, SCL -> 22, MPU_ADDR 0x68 (AD0 = GND)
  Now transmits over Bluetooth using BluetoothSerial in addition to normal Serial.
*/

#include <Wire.h>
#include <math.h>
#include "BluetoothSerial.h"

BluetoothSerial BTSerial;  // Create Bluetooth serial object

const int MPU_ADDR = 0x68;
const int SDA_PIN = 21;
const int SCL_PIN = 22;

// ----- Thresholds -----
const float MOTION_THRESHOLD_G = 0.12;
const float ACCEL_THRESHOLD_G  = 0.30;
const float BRAKE_THRESHOLD_G  = -0.30;
const float DECEL_THRESHOLD_G  = -0.15;
const float VIBRATION_VAR_HIGH = 0.05;
const float VIBRATION_VAR_LOW  = 0.01;
const float PITCH_BIKE_DEG     = 25.0;
const float PITCH_SCOOTER_DEG  = 20.0;
const float WALKING_MAX_SPEED_KMPH = 6.0;

// ----- Sliding window -----
const int WINDOW_SIZE = 40;
float magWindow[WINDOW_SIZE];
int magIndex = 0;
int magCount = 0;

// ----- Variables -----
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;
unsigned long lastRead = 0;
float gpsSpeedKmph = -1.0;

// Debounce / time gating
unsigned long lastAccelEvent = 0;
unsigned long lastBrakeEvent = 0;
unsigned long lastMotionChange = 0;
const unsigned long EVENT_DEBOUNCE_MS = 300;

// Complementary filters
float pitchFiltered = 0.0;
float forwardGFiltered = 0.0;
float magVarFiltered = 0.0;
const float PITCH_ALPHA = 0.1;
const float SMOOTH_ALPHA = 0.2;

// Mode memory
String lastMode = "Walking";
unsigned long lastModeChange = 0;

// helpers
float accelToG(int16_t raw) { return raw / 16384.0; }
float gyroToDps(int16_t raw) { return raw / 131.0; }

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(100);

  // Initialize Bluetooth
  BTSerial.begin("ESP32_Telemetry"); // device name visible for pairing
  Serial.println("✅ Bluetooth started! Pair with 'ESP32_Telemetry'");

  // Wake MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  for (int i = 0; i < WINDOW_SIZE; i++) magWindow[i] = 1.0;
  Serial.println("MPU6050 telemetry logic starting...");
  BTSerial.println("MPU6050 telemetry logic starting...");
  lastRead = millis();
}

void loop() {
  readGPSFromSerial();

  if (millis() - lastRead >= 50) {
    lastRead = millis();
    readMPU6050();
    processSensors();
  }

  delay(400);  // 👈 small delay for readable output
}

void readMPU6050() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);

  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();
  Tmp = Wire.read() << 8 | Wire.read();
  GyX = Wire.read() << 8 | Wire.read();
  GyY = Wire.read() << 8 | Wire.read();
  GyZ = Wire.read() << 8 | Wire.read();
}

void processSensors() {
  float ax_g = accelToG(AcX);
  float ay_g = accelToG(AcY);
  float az_g = accelToG(AcZ);
  float gx_dps = gyroToDps(GyX);
  float gy_dps = gyroToDps(GyY);
  float gz_dps = gyroToDps(GyZ);
  float tempC = Tmp / 340.0 + 36.53;

  float mag = sqrt(ax_g * ax_g + ay_g * ay_g + az_g * az_g);

  magWindow[magIndex] = mag;
  magIndex = (magIndex + 1) % WINDOW_SIZE;
  if (magCount < WINDOW_SIZE) magCount++;

  float magMean = 0.0, magVar = 0.0;
  for (int i = 0; i < magCount; i++) magMean += magWindow[i];
  magMean /= magCount;

  for (int i = 0; i < magCount; i++) {
    float d = magWindow[i] - magMean;
    magVar += d * d;
  }
  magVar /= max(1, magCount);

  magVarFiltered = (1.0 - SMOOTH_ALPHA) * magVarFiltered + SMOOTH_ALPHA * magVar;

  bool motionDetected = (fabs(mag - 1.0) > MOTION_THRESHOLD_G);

  float pitch = atan2(ay_g, sqrt(ax_g * ax_g + az_g * az_g)) * 180.0 / M_PI;
  pitchFiltered = (1.0 - PITCH_ALPHA) * pitchFiltered + PITCH_ALPHA * pitch;
  forwardGFiltered = 0.85 * forwardGFiltered + 0.15 * ax_g;

  bool accelerating = false, braking = false, decelerating = false;
  unsigned long now = millis();

  if (motionDetected) {
    if (forwardGFiltered > ACCEL_THRESHOLD_G && (now - lastAccelEvent) > EVENT_DEBOUNCE_MS) {
      accelerating = true;
      lastAccelEvent = now;
    }
    if (forwardGFiltered < BRAKE_THRESHOLD_G && (now - lastBrakeEvent) > EVENT_DEBOUNCE_MS) {
      braking = true;
      lastBrakeEvent = now;
    }
    if (forwardGFiltered < DECEL_THRESHOLD_G && forwardGFiltered >= BRAKE_THRESHOLD_G) {
      decelerating = true;
    }
  }

  String posture = "Neutral";
  if (pitchFiltered >= PITCH_BIKE_DEG) posture = "Bike (bent)";
  else if (pitchFiltered <= PITCH_SCOOTER_DEG) posture = "Scooter (upright)";

  String mode = "Walking";
  static bool walkingSticky = false;

  if (gpsSpeedKmph < WALKING_MAX_SPEED_KMPH && motionDetected && magVarFiltered > 0.015 && magVarFiltered < 0.06 && magMean < 1.25) {
    walkingSticky = true;
    mode = "Walking";
  } else if (walkingSticky && gpsSpeedKmph < WALKING_MAX_SPEED_KMPH && !accelerating && !braking) {
    mode = "Walking";
  } else {
    walkingSticky = false;
  }

  if (gpsSpeedKmph >= WALKING_MAX_SPEED_KMPH || accelerating || braking) {
    if (pitchFiltered >= PITCH_BIKE_DEG && magVarFiltered > VIBRATION_VAR_HIGH) {
      mode = "Bike";
      walkingSticky = false;
    } else if (pitchFiltered < PITCH_BIKE_DEG && (magVarFiltered < VIBRATION_VAR_HIGH || accelerating || decelerating)) {
      mode = "Scooter";
      walkingSticky = false;
    }
  }

  if (!motionDetected) mode = "Walking";

  // same output to both Serial and Bluetooth
  String msg = "";
  msg += "Mag(g):" + String(mag, 3);
  msg += " Var:" + String(magVarFiltered, 4);
  msg += " | Ax(g):" + String(ax_g, 3);
  msg += " Ay:" + String(ay_g, 3);
  msg += " Az:" + String(az_g, 3);
  msg += " | Pitch:" + String(pitchFiltered, 2);
  msg += " | FwdG:" + String(forwardGFiltered, 3);
  msg += " | Mode:" + mode;
  msg += " | Posture:" + posture;
  msg += " | Accel:" + String(accelerating ? 1 : 0);
  msg += " Brk:" + String(braking ? 1 : 0);
  msg += " Decel:" + String(decelerating ? 1 : 0);
  if (gpsSpeedKmph >= 0.0) msg += " | GPS(km/h):" + String(gpsSpeedKmph, 2);
  msg += "\n";

  Serial.print(msg);
  BTSerial.print(msg);
}

void readGPSFromSerial() {
  static String buffer = "";
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (buffer.length() > 0) {
        buffer.trim();
        if (buffer.startsWith("GPS:")) {
          String val = buffer.substring(4);
          float s = val.toFloat();
          if (s > 0.0) gpsSpeedKmph = s;
          else gpsSpeedKmph = -1.0;
        } else if (buffer == "GPS:NA") {
          gpsSpeedKmph = -1.0;
        }
        buffer = "";
      }
    } else buffer += c;
    if (buffer.length() > 64) buffer = buffer.substring(buffer.length() - 64);
  }
}
