#include <Wire.h>
#include <TinyGPSPlus.h>
#include "BluetoothSerial.h"
#include <Preferences.h>
#include <math.h>

// ===== Objects =====
BluetoothSerial SerialBT;
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
Preferences prefs;

// ===== MPU Constants =====
const uint8_t MPU_ADDR = 0x68;
const uint8_t PWR_MGMT_1 = 0x6B;
const uint8_t ACCEL_XOUT_H = 0x3B;

// ===== Config =====
const int SPEED_BUFFER = 10;
float speedBuf[SPEED_BUFFER];
int speedIdx = 0;
bool bufInit = false;

const float MOVE_THRESHOLD = 1.5f;
const int MIN_SATS_FOR_RELIABLE = 4;
const float MAX_JUMP_PER_SEC = 10.0f;
const int N_MOVING_CONS = 4;
const unsigned long UPDATE_INTERVAL = 400; // ms

const int LED_PIN = 2;

// ===== MPU / Calibration =====
float offsetX = 0, offsetY = 0, offsetZ = 0;
bool mpuReady = false;

// ===== States =====
bool gpsLocked = false;
String gpsSignal = "None";
String motionMode = "Idle";

// ===== Crash / Braking Detection =====
int crashCount = 0;
const int CRASH_THRESHOLD = 5;
const float CRASH_ACC_THRESHOLD = 1.0;

float lastFx = 0.0;
int mpuBrakeCount = 0;
const int MPU_BRAKE_COUNT_THRESHOLD = 4;
const float MPU_BRAKE_DELTA_G = 0.6f;

int brakeCount = 0;
const int BRAKE_THRESHOLD_COUNT = 3;
const float BRAKE_SPEED_DROP_KMH = 6.0f;
float lastSmoothSpeed = 0.0f;
unsigned long lastBrakeCheckTime = 0;

// ===== GPS / Speed Tracking =====
float lastAcceptedSpeed = 0.0;
unsigned long lastSpeedTime = 0;
int consecutiveMoving = 0;

// ===== Timing =====
unsigned long lastUpdate = 0;

// ===== Function Prototypes =====
bool initMPU();
bool readMPU(float &fx, float &fy, float &fz);
void calibrateMPU();
void saveCalibration();
void loadCalibration();
void processGPS(unsigned long now);
void processMPU(float fx, float fy, float fz);
void updateMotionMode();
void publishUnified();
String getDateTimeString();
void checkBTCommands();

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  SerialBT.begin("RiderTelemetry");
  gpsSerial.begin(9600, SERIAL_8N1, 4, 2);
  Wire.begin(21, 22);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("\n🏍 Rider Telemetry System v5.3-Lite Starting...");
  SerialBT.println("🏍 Rider Telemetry System v5.3-Lite Starting...");

  // Load calibration
  loadCalibration();

  if (initMPU()) {
    mpuReady = true;
    if (fabs(offsetX) < 1e-5 && fabs(offsetY) < 1e-5 && fabs(offsetZ) < 1e-5)
      calibrateMPU();
    else {
      Serial.printf("Loaded calibration X=%.3f Y=%.3f Z=%.3f\n", offsetX, offsetY, offsetZ);
      SerialBT.printf("Loaded calibration X=%.3f Y=%.3f Z=%.3f\n", offsetX, offsetY, offsetZ);
    }
  } else {
    Serial.println("❌ MPU6050 not detected!");
    SerialBT.println("❌ MPU6050 not detected!");
  }

  Serial.println("✅ System Ready!");
  SerialBT.println("✅ System Ready!");
}

// ===== Main Loop =====
void loop() {
  while (gpsSerial.available()) gps.encode(gpsSerial.read());
  unsigned long now = millis();

  if (now - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = now;
    if (gps.speed.isUpdated() || gps.location.isUpdated()) processGPS(now);

    if (mpuReady) {
      float fx, fy, fz;
      if (readMPU(fx, fy, fz)) processMPU(fx, fy, fz);
    }

    updateMotionMode();
    digitalWrite(LED_PIN, gpsLocked ? HIGH : LOW);
    publishUnified();
  }

  checkBTCommands();
  delay(5);
}

// ===== GPS Processing =====
void processGPS(unsigned long now) {
  float rawSpeed = gps.speed.kmph();
  bool validSpeed = gps.speed.isValid();

  int sats = gps.satellites.value();
  gpsLocked = (sats >= MIN_SATS_FOR_RELIABLE);
  if (!gpsLocked) validSpeed = false;

  float candidate = validSpeed ? rawSpeed : 0.0f;
  float dt_s = max(0.001f, (now - lastSpeedTime) / 1000.0f);
  float allowed_jump = MAX_JUMP_PER_SEC * dt_s;

  if (abs(candidate - lastAcceptedSpeed) > allowed_jump && lastSpeedTime != 0)
    candidate = lastAcceptedSpeed;
  else {
    lastAcceptedSpeed = candidate;
    lastSpeedTime = now;
  }

  // Smoothing buffer
  speedBuf[speedIdx] = candidate;
  speedIdx = (speedIdx + 1) % SPEED_BUFFER;

  float sum = 0;
  for (int i = 0; i < SPEED_BUFFER; i++) sum += speedBuf[i];
  float smoothSpeed = sum / SPEED_BUFFER;
  bufInit = true;

  // GPS signal quality
  if (sats == 0) gpsSignal = "None";
  else if (sats < 4) gpsSignal = "Weak";
  else if (sats < 7) gpsSignal = "Medium";
  else gpsSignal = "Strong";

  // Braking detection (GPS)
  unsigned long tnow = millis();
  if (tnow - lastBrakeCheckTime > 800) {
    float drop = lastSmoothSpeed - smoothSpeed;
    if (drop >= BRAKE_SPEED_DROP_KMH) brakeCount++;
    else brakeCount = 0;
    lastSmoothSpeed = smoothSpeed;
    lastBrakeCheckTime = tnow;
  }

  if (brakeCount >= BRAKE_THRESHOLD_COUNT) {
    SerialBT.println("🛑 Harsh Braking (GPS)!");
    brakeCount = 0;
  }

  String gpsOutput = String("[") + getDateTimeString() + "] 📡 " +
                     "Lat:" + String(gps.location.isValid() ? gps.location.lat() : 0, 6) +
                     " | Lon:" + String(gps.location.isValid() ? gps.location.lng() : 0, 6) +
                     " | Spd:" + String(smoothSpeed, 2) + " km/h" +
                     " | Sat:" + String(sats) +
                     " | Sig:" + gpsSignal;

  Serial.println(gpsOutput);
  SerialBT.println(gpsOutput);
}

// ===== MPU =====
bool initMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(PWR_MGMT_1);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) return false;
  delay(100);
  Serial.println("✅ MPU6050 initialized!");
  SerialBT.println("✅ MPU6050 initialized!");
  return true;
}

bool readMPU(float &fx, float &fy, float &fz) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(ACCEL_XOUT_H);
  if (Wire.endTransmission(false) != 0) return false;

  Wire.requestFrom((int)MPU_ADDR, 6, (uint8_t)true);
  if (Wire.available() < 6) return false;

  int16_t ax = (Wire.read() << 8) | Wire.read();
  int16_t ay = (Wire.read() << 8) | Wire.read();
  int16_t az = (Wire.read() << 8) | Wire.read();

  fx = ax / 16384.0 - offsetX;
  fy = ay / 16384.0 - offsetY;
  fz = -(az / 16384.0 - offsetZ);
  return true;
}

void calibrateMPU() {
  Serial.println("⚙ Calibrating MPU6050... Keep still...");
  SerialBT.println("⚙ Calibrating MPU6050... Keep still...");
  long sx = 0, sy = 0, sz = 0;
  for (int i = 0; i < 200; i++) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(ACCEL_XOUT_H);
    Wire.endTransmission(false);
    Wire.requestFrom((int)MPU_ADDR, 6, (uint8_t)true);
    int16_t ax = (Wire.read() << 8) | Wire.read();
    int16_t ay = (Wire.read() << 8) | Wire.read();
    int16_t az = (Wire.read() << 8) | Wire.read();
    sx += ax; sy += ay; sz += az;
    delay(5);
  }
  offsetX = sx / (16384.0 * 200);
  offsetY = sy / (16384.0 * 200);
  offsetZ = sz / (16384.0 * 200);
  saveCalibration();
  Serial.printf("✅ Calibrated: X=%.3f Y=%.3f Z=%.3f\n", offsetX, offsetY, offsetZ);
  SerialBT.printf("✅ Calibrated: X=%.3f Y=%.3f Z=%.3f\n", offsetX, offsetY, offsetZ);
}

// ===== Crash / Braking Detection =====
void processMPU(float fx, float fy, float fz) {
  float totalAcc = sqrt(fx*fx + fy*fy + fz*fz);
  float accWithoutG = fabs(totalAcc - 1.0);
  float leanAngle = atan2(fy, fz) * 180.0 / M_PI;

  if (accWithoutG > CRASH_ACC_THRESHOLD) crashCount++;
  else crashCount = 0;

  if (crashCount >= CRASH_THRESHOLD) {
    SerialBT.println("⚠ Crash Detected!");
    crashCount = 0;
  }

  float deltaFx = lastFx - fx;
  lastFx = fx;
  if (deltaFx > MPU_BRAKE_DELTA_G) mpuBrakeCount++;
  else mpuBrakeCount = 0;

  if (mpuBrakeCount >= MPU_BRAKE_COUNT_THRESHOLD) {
    SerialBT.println("🛑 Harsh Braking (MPU)!");
    mpuBrakeCount = 0;
  }

  String mpuOut = String("[") + getDateTimeString() + "] " +
                  "Accel X:" + String(fx, 2) +
                  " Y:" + String(fy, 2) +
                  " Z:" + String(fz, 2) +
                  " | Lean:" + String(leanAngle, 1) + "°";
  Serial.println(mpuOut);
  SerialBT.println(mpuOut);
}

// ===== Motion Mode =====
void updateMotionMode() {
  if (gpsLocked && bufInit) {
    float avg = 0;
    for (int i = 0; i < SPEED_BUFFER; i++) avg += speedBuf[i];
    avg /= SPEED_BUFFER;
    if (avg < 1.5f) motionMode = "Idle";
    else if (avg < 10.0f) motionMode = "Walking";
    else motionMode = "Scooter";
  }
}

// ===== Output =====
void publishUnified() {
  float avg = 0;
  for (int i = 0; i < SPEED_BUFFER; i++) avg += speedBuf[i];
  avg /= SPEED_BUFFER;

  String unified = String("[") + getDateTimeString() + "] 🚦 Mode:" + motionMode +
                   " | Source:" + String(gpsLocked ? "GPS" : "MPU") +
                   " | Speed:" + String(avg, 2) + " km/h" +
                   " | Sig:" + gpsSignal;
  Serial.println(unified);
  SerialBT.println(unified);
}

// ===== Time =====
String getDateTimeString() {
  if (gps.date.isValid() && gps.time.isValid()) {
    char buf[20];
    sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
            gps.date.year(), gps.date.month(), gps.date.day(),
            gps.time.hour(), gps.time.minute(), gps.time.second());
    return String(buf);
  }
  return String("no-time");
}

// ===== Bluetooth Commands =====
void checkBTCommands() {
  if (SerialBT.available()) {
    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim();
    if (cmd.equalsIgnoreCase("#RECAL")) {
      calibrateMPU();
      SerialBT.println("⚙ Recalibration complete.");
    } else {
      SerialBT.println("❓ Unknown command.");
    }
  }
}

// ===== Calibration Save/Load =====
void saveCalibration() {
  prefs.begin("rider", false);
  prefs.putFloat("offX", offsetX);
  prefs.putFloat("offY", offsetY);
  prefs.putFloat("offZ", offsetZ);
  prefs.end();
}

void loadCalibration() {
  prefs.begin("rider", true);
  offsetX = prefs.getFloat("offX", 0.0f);
  offsetY = prefs.getFloat("offY", 0.0f);
  offsetZ = prefs.getFloat("offZ", 0.0f);
  prefs.end();
}void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
