#include <Wire.h>
#include <MPU6050_tockn.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

MPU6050 mpu(Wire);
TinyGPSPlus gps;
SoftwareSerial gpsSerial(3, 4); // RX=3 (GPS TX), TX=4 (GPS RX)

unsigned long lastSend = 0;
bool gpsFix = false;

// Motion detection variables
float motionIntensity = 0.0;
float smoothSpeed = 0.0;
unsigned long lastMotion = 0;
bool isMoving = false;

// Sensitivity constants
const float MOVE_THRESHOLD = 0.12;   // Acceleration (g) threshold for movement
const float NOISE_LIMIT = 0.03;      // Ignore small vibrations
const unsigned long STOP_TIMEOUT = 1200; // ms before deciding stop
const float SPEED_GAIN = 9.5;        // Multiplier to scale motion intensity to speed (tune this empirically)

// Tilt/Posture thresholds
const float TILT_FORWARD = 10.0;     // degrees
const float TILT_BACKWARD = -10.0;   // degrees

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600);
  Wire.begin();

  Serial.println("=== Ride Assist: Motion + Tilt Detection (Fast Mode) ===");
  mpu.begin();
  mpu.calcGyroOffsets(true);
  delay(200);
  Serial.println("✅ MPU initialized");
  Serial.println("🚀 Starting motion tracking...");
}

void loop() {
  mpu.update();

  // GPS update
  while (gpsSerial.available()) gps.encode(gpsSerial.read());
  if (!gpsFix && gps.location.isValid()) {
    gpsFix = true;
    Serial.println("✅ GPS fix acquired");
  }

  // Read acceleration in g
  float ax = mpu.getAccX();
  float ay = mpu.getAccY();
  float az = mpu.getAccZ();

  // Posture detection (based on tilt angle)
  float tilt = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;
  String posture;
  if (tilt > TILT_FORWARD) posture = "Leaning Forward";
  else if (tilt < TILT_BACKWARD) posture = "Leaning Backward";
  else posture = "Upright";

  // Motion detection (separate from tilt)
  // Remove gravity effect (approx 1g on Z-axis)
  float linearAccZ = az - 1.0;
  float netAcc = sqrt(ax * ax + ay * ay + linearAccZ * linearAccZ);

  if (netAcc < NOISE_LIMIT) netAcc = 0;

  if (netAcc > MOVE_THRESHOLD) {
    motionIntensity = netAcc;
    lastMotion = millis();
    isMoving = true;
  } else if (millis() - lastMotion > STOP_TIMEOUT) {
    isMoving = false;
    motionIntensity = 0;
  }

  // Compute MPU-based speed (not from tilt)
  float mpuSpeed = isMoving ? motionIntensity * SPEED_GAIN : 0;

  // Smooth output (low-pass filter)
  smoothSpeed = 0.7 * smoothSpeed + 0.3 * mpuSpeed;

  // Transmit telemetry every 200ms
  if (millis() - lastSend > 200) {
    lastSend = millis();

    Serial.print("{\"ax\":"); Serial.print(ax, 2);
    Serial.print(",\"ay\":"); Serial.print(ay, 2);
    Serial.print(",\"az\":"); Serial.print(az, 2);
    Serial.print(",\"tilt\":"); Serial.print(tilt, 1);
    Serial.print(",\"posture\":\""); Serial.print(posture); Serial.print("\"");
    Serial.print(",\"motion\":"); Serial.print(motionIntensity, 2);
    Serial.print(",\"moving\":"); Serial.print(isMoving ? "true" : "false");
    Serial.print(",\"mpu_spd\":"); Serial.print(smoothSpeed, 2);

    if (gps.location.isValid()) {
      Serial.print(",\"lat\":"); Serial.print(gps.location.lat(), 6);
      Serial.print(",\"lon\":"); Serial.print(gps.location.lng(), 6);
      Serial.print(",\"gps_spd\":"); Serial.print(gps.speed.kmph(), 2);
      Serial.print(",\"sat\":"); Serial.print(gps.satellites.value());
    } else {
      Serial.print(",\"lat\":null,\"lon\":null,\"gps_spd\":0.0,\"sat\":0");
    }

    Serial.println("}");
  }
}