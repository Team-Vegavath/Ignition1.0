/* 
  ESP32 + MPU6050 demo
  - posture calibration at startup (3s)
  - walking vs riding classification (IMU window + optional GPS speed over Bluetooth SPP)
  - crooked posture detection
  - imminent-impact / hard-decel detection (warning)
  
  Notes:
   - If a phone sends "SPEED:NNN" over Bluetooth SPP (classic), the code will parse it and use it (m/s).
   - If no GPS is provided, classification falls back to IMU-only heuristics.
*/

#include <Wire.h>
#include "MPU6050.h"
#include "BluetoothSerial.h"

MPU6050 mpu;
BluetoothSerial BTSerial;

const int SAMPLE_HZ = 100;
const int sampleDelayMs = 1000 / SAMPLE_HZ;

// rolling buffer for accel magnitude
const int WINDOW_S = 2; // seconds
const int WINDOW_N = SAMPLE_HZ * WINDOW_S;
float accBuffer[WINDOW_N];
int accIndex = 0;
bool bufferFilled = false;

unsigned long lastSampleMillis = 0;

// posture baseline (calibrated at startup)
float baselinePitch = 0.0;
float baselineRoll  = 0.0;
bool calibrated = false;

// thresholds (tune these)
const float postureThresholdDeg = 18.0; // deg deviation -> crooked
const unsigned long postureHoldMs = 1500; // must hold for this long
const float accelStdThresholdWalking = 0.6; // higher = walking; lower = riding (tune)
const float gpsSpeedRidingThreshold = 3.0; // m/s ~ 10.8 km/h
const float impactAccelThreshold_g = 3.0; // g's -> immediate impact
const float jerkThreshold_gps = 4.0; // sudden jerk in g (approx)

unsigned long crookedSince = 0;
bool crookedReported = false;

float lastAccMag = 0;
unsigned long lastImpactReport = 0;
const unsigned long impactCooldownMs = 3000; // avoid multiple triggers

// optional GPS speed from phone (in m/s). -1 if none.
float phoneGpsSpeed = -1.0;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Serial.println("ESP32 + MPU6050 demo starting...");

  // Init Bluetooth SPP (Classic)
  if (!BTSerial.begin("ESP32-Helmet")) {
    Serial.println("Failed to start Bluetooth");
  } else {
    Serial.println("Bluetooth started. Pair phone to 'ESP32-Helmet' and send 'SPEED:xx' lines (m/s).");
  }

  // init MPU6050
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed. Check wiring (SDA,SCL,GND,VCC).");
    while (1) delay(1000);
  }
  Serial.println("MPU6050 OK");

  // calibration of baseline posture: ask user to keep helmet upright for 3 seconds
  Serial.println("Calibrating posture baseline for 3 seconds. Keep helmet upright and steady...");
  delay(1000);
  calibrateBaseline(3000);
  calibrated = true;
  Serial.printf("Baseline pitch=%.2f roll=%.2f\n", baselinePitch, baselineRoll);
  lastSampleMillis = millis();
}

void loop() {
  // handle BT incoming lines (optional phone speed)
  handleBluetooth();

  unsigned long now = millis();
  if (now - lastSampleMillis < sampleDelayMs) return;
  lastSampleMillis = now;

  // read IMU
  int16_t axRaw, ayRaw, azRaw;
  int16_t gxRaw, gyRaw, gzRaw;
  mpu.getAcceleration(&axRaw, &ayRaw, &azRaw);
  mpu.getRotation(&gxRaw, &gyRaw, &gzRaw);

  // convert to g and deg/s
  float ax = axRaw / 16384.0;
  float ay = ayRaw / 16384.0;
  float az = azRaw / 16384.0;
  float gx = gxRaw / 131.0;
  float gy = gyRaw / 131.0;
  float gz = gzRaw / 131.0;

  // magnitude
  float accMag = sqrt(ax*ax + ay*ay + az*az);

  // store in rolling buffer
  accBuffer[accIndex++] = accMag;
  if (accIndex >= WINDOW_N) { accIndex = 0; bufferFilled = true; }

  // compute stddev if buffer has enough samples
  float accStd = 0;
  if (bufferFilled) {
    float mean = 0;
    for (int i=0;i<WINDOW_N;i++) mean += accBuffer[i];
    mean /= WINDOW_N;
    float var = 0;
    for (int i=0;i<WINDOW_N;i++) {
      float d = accBuffer[i] - mean;
      var += d*d;
    }
    var /= WINDOW_N;
    accStd = sqrt(var);
  }

  // compute pitch/roll (simple)
  float pitch = atan2(ax, sqrt(ay*ay + az*az)) * 180.0 / PI;
  float roll  = atan2(ay, az) * 180.0 / PI;

  // Posture check (crooked)
  float dPitch = fabs(pitch - baselinePitch);
  float dRoll  = fabs(roll  - baselineRoll);
  bool crookedNow = (dPitch > postureThresholdDeg) || (dRoll > postureThresholdDeg);
  if (crookedNow) {
    if (crookedSince == 0) crookedSince = now;
    if (!crookedReported && (now - crookedSince) > postureHoldMs) {
      crookedReported = true;
      reportCrooked(dPitch, dRoll);
    }
  } else {
    crookedSince = 0;
    crookedReported = false;
  }

  // Riding vs Walking classification
  bool isRiding = false;
  if (phoneGpsSpeed >= 0) {
    // If phone speed available, use it
    isRiding = (phoneGpsSpeed > gpsSpeedRidingThreshold);
  } else if (bufferFilled) {
    // IMU-only fallback
    // Lower stddev -> smoother ride (riding). Higher stddev -> walking.
    isRiding = (accStd < accelStdThresholdWalking);
  }
  reportMotion(isRiding, accStd, phoneGpsSpeed);

  // Impact / imminent crash detection with IMU (jerk & big accel changes)
  float jerk = fabs(accMag - lastAccMag) * SAMPLE_HZ; // approx g/s
  if ((accMag > impactAccelThreshold_g || jerk > jerkThreshold_gps) && (now - lastImpactReport > impactCooldownMs)) {
    lastImpactReport = now;
    reportImpact(accMag, jerk);
  }
  lastAccMag = accMag;

  // minimal serial status
  if (bufferFilled) {
    Serial.printf("P:%.1f R:%.1f | accStd: %.3f | %s | gps:%.2f m/s\n",
                  pitch, roll, accStd, (isRiding ? "RIDING":"WALKING/IDLE"), phoneGpsSpeed);
  }
}

// ---------- helpers ----------

void calibrateBaseline(unsigned long ms) {
  unsigned long start = millis();
  int count = 0;
  float sumPitch = 0, sumRoll = 0;
  while (millis() - start < ms) {
    int16_t axRaw, ayRaw, azRaw;
    mpu.getAcceleration(&axRaw, &ayRaw, &azRaw);
    float ax = axRaw / 16384.0;
    float ay = ayRaw / 16384.0;
    float az = azRaw / 16384.0;
    float pitch = atan2(ax, sqrt(ay*ay + az*az)) * 180.0 / PI;
    float roll  = atan2(ay, az) * 180.0 / PI;
    sumPitch += pitch;
    sumRoll  += roll;
    count++;
    delay(20);
  }
  baselinePitch = sumPitch / count;
  baselineRoll  = sumRoll  / count;
}

void reportCrooked(float dPitch, float dRoll) {
  Serial.printf("*** CROOKED POSTURE DETECTED (dP=%.1f dR=%.1f) ***\n", dPitch, dRoll);
  // TODO: add buzzer / vibration pin or BLE alert
}

void reportMotion(bool isRiding, float accStd, float gpsSpeed) {
  // optional: publish over BT or LED
  static unsigned long lastReport = 0;
  unsigned long now = millis();
  if (now - lastReport < 800) return;
  lastReport = now;
  if (isRiding) {
    Serial.println("==> Classified: RIDING");
  } else {
    Serial.println("==> Classified: WALKING/IDLE");
  }
}

void reportImpact(float accMag, float jerk) {
  Serial.printf("!!! IMPACT/HEAVY BREAK DETECTED: acc=%.2fg jerk=%.1f g/s !!!\n", accMag, jerk);
  // TODO: immediate alert: LED/buzzer/Bluetooth message
  // Example: send alert string over BT
  if (BTSerial.connected()) {
    BTSerial.printf("ALERT:IMPACT acc=%.2f jerk=%.1f\n", accMag, jerk);
  }
}

// Very simple Bluetooth line handling for "SPEED:xx" incoming
String btLine = "";
void handleBluetooth() {
  while (BTSerial.available()) {
    char c = BTSerial.read();
    if (c == '\n' || c == '\r') {
      if (btLine.length() > 0) {
        parseBTLine(btLine);
        btLine = "";
      }
    } else {
      btLine += c;
      if (btLine.length() > 200) btLine = ""; // safety
    }
  }
}

void parseBTLine(const String &ln) {
  String s = ln;
  s.trim();
  // Expect lines like: SPEED:3.5
  if (s.startsWith("SPEED:")) {
    String num = s.substring(6);
    num.trim();
    float v = num.toFloat();
    if (v > 0.001) {
      phoneGpsSpeed = v;
      Serial.printf("Phone GPS speed updated: %.2f m/s\n", phoneGpsSpeed);
      return;
    }
  }
  // allow reset
  if (s == "GPS:OFF" || s == "SPEED:-1") {
    phoneGpsSpeed = -1.0;
    Serial.println("Phone GPS supply disabled.");
    return;
  }
  // echo any other bt text for debugging
  Serial.printf("BT RX: %s\n", s.c_str());
}
