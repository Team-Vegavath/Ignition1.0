#include <Wire.h>
#include <math.h>

#define MPU_ADDR 0x68
#define CAL_SAMPLES 200      // how many samples to average for bias
#define GRAVITY_LP_ALPHA 0.02f // gravity low-pass filter alpha (0..1). Lower = smoother/slower

// bias values (LSB)
float biasX = 0, biasY = 0, biasZ = 0;

// gravity estimate in g
float gX = 0, gY = 0, gZ = 0;

// chosen accel range LSB/g (default ±2g)
const float LSB_PER_G = 16384.0f;

void wakeMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);
  delay(20);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  delay(100);

  Serial.println();
  Serial.println("Keep the MPU6050 perfectly still and flat for calibration...");
  wakeMPU();

  // simple calibration: average raw readings while stationary
  long sumX = 0, sumY = 0, sumZ = 0;
  for (int i = 0; i < CAL_SAMPLES; ++i) {
    int16_t ax, ay, az;
    readRawAccel(ax, ay, az);
    sumX += ax;
    sumY += ay;
    sumZ += az;
    delay(10);
  }
  // convert to g and compute bias relative to expected gravity on Z (assuming flat)
  // Note: if module is not exactly flat, we compute raw bias only (we'll use gravity removal later)
  biasX = (float)sumX / CAL_SAMPLES;
  biasY = (float)sumY / CAL_SAMPLES;
  biasZ = (float)sumZ / CAL_SAMPLES;

  Serial.println("Calibration done (raw LSB offsets):");
  Serial.print(" biasX = "); Serial.println(biasX);
  Serial.print(" biasY = "); Serial.println(biasY);
  Serial.print(" biasZ = "); Serial.println(biasZ);
  Serial.println("Now streaming: Raw(g) | Gravity(g) | Linear(g)  (move sensor to see linear accel)");
  Serial.println();
  delay(200);
}

// helper to read raw accel into ints
void readRawAccel(int16_t &ax, int16_t &ay, int16_t &az) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, (uint8_t)6, (uint8_t)true);

  ax = (Wire.read() << 8) | Wire.read();
  ay = (Wire.read() << 8) | Wire.read();
  az = (Wire.read() << 8) | Wire.read();
}

// convert raw LSB to g using LSB_PER_G
inline float rawToG(float raw) { return raw / LSB_PER_G; }

void loop() {
  int16_t rawAx, rawAy, rawAz;
  readRawAccel(rawAx, rawAy, rawAz);

  // remove bias (raw)
  float ax = (float)rawAx - biasX;
  float ay = (float)rawAy - biasY;
  float az = (float)rawAz - biasZ;

  // convert to g
  float axg = rawToG(ax);
  float ayg = rawToG(ay);
  float azg = rawToG(az);

  // compute instantaneous gravity direction magnitude (we use accel measurement as rough gravity estimate)
  // normalize the measured accel vector to unit length then assume gravity magnitude = 1g
  float mag = sqrt(axg*axg + ayg*ayg + azg*azg);
  float nx = 0, ny = 0, nz = 0;
  if (mag > 0.001f) {
    nx = axg / mag;
    ny = ayg / mag;
    nz = azg / mag;
  }

  // target gravity vector (1g along measured direction)
  float gx_target = nx * 1.0f;
  float gy_target = ny * 1.0f;
  float gz_target = nz * 1.0f;

  // low-pass filter gravity estimate to avoid spikes (exponential smoothing)
  gX = (1.0f - GRAVITY_LP_ALPHA) * gX + GRAVITY_LP_ALPHA * gx_target;
  gY = (1.0f - GRAVITY_LP_ALPHA) * gY + GRAVITY_LP_ALPHA * gy_target;
  gZ = (1.0f - GRAVITY_LP_ALPHA) * gZ + GRAVITY_LP_ALPHA * gz_target;

  // linear acceleration = measured accel (g) - gravity estimate (g)
  float linX = axg - gX;
  float linY = ayg - gY;
  float linZ = azg - gZ;

  // print Raw, Gravity, Linear (g) with 3 decimals
  Serial.print("Raw(g): ");
  Serial.print(axg, 3); Serial.print(", ");
  Serial.print(ayg, 3); Serial.print(", ");
  Serial.print(azg, 3);
  Serial.print(" | Grav(g): ");
  Serial.print(gX, 3); Serial.print(", ");
  Serial.print(gY, 3); Serial.print(", ");
  Serial.print(gZ, 3);
  Serial.print(" | Lin(g): ");
  Serial.print(linX, 3); Serial.print(", ");
  Serial.print(linY, 3); Serial.print(", ");
  Serial.println(linZ, 3);

  delay(100);
}
