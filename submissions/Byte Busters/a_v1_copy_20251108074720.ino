#include <Wire.h>

const int MPU = 0x68;  // I2C address of MPU6050

float AccX, AccY, AccZ;
float GyroX, GyroY, GyroZ;
float Temp;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Initializing MPU6050...");

  // Start I2C communication
  Wire.begin(21, 22);  // SDA = 21, SCL = 22 for ESP32

  // Wake up MPU6050 (it starts in sleep mode)
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);  // Power management register
  Wire.write(0);
  Wire.endTransmission(true);

  // Check device connection
  Wire.beginTransmission(MPU);
  if (Wire.endTransmission() == 0) {
    Serial.println("✅ MPU6050 Connected Successfully!");
  } else {
    Serial.println("❌ MPU6050 Not Found! Check wiring.");
    while (1);
  }
}

void loop() {
  // Request 14 bytes of data from MPU6050
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);  // Starting register for accelerometer data
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 14, true);

  // Read accelerometer data
  AccX = (Wire.read() << 8 | Wire.read()) / 16384.0;
  AccY = (Wire.read() << 8 | Wire.read()) / 16384.0;
  AccZ = (Wire.read() << 8 | Wire.read()) / 16384.0;

  // Read temperature data
  Temp = (Wire.read() << 8 | Wire.read());
  Temp = (Temp / 340.00) + 36.53;

  // Read gyroscope data
  GyroX = (Wire.read() << 8 | Wire.read()) / 131.0;
  GyroY = (Wire.read() << 8 | Wire.read()) / 131.0;
  GyroZ = (Wire.read() << 8 | Wire.read()) / 131.0;

  // Print all sensor values
  Serial.println("\n📊 Sensor Readings:");
  Serial.print("Acceleration (g): X="); Serial.print(AccX);
  Serial.print(" | Y="); Serial.print(AccY);
  Serial.print(" | Z="); Serial.println(AccZ);

  Serial.print("Gyroscope (°/s): X="); Serial.print(GyroX);
  Serial.print(" | Y="); Serial.print(GyroY);
  Serial.print(" | Z="); Serial.println(GyroZ);

  Serial.print("Temperature: "); Serial.print(Temp); Serial.println(" °C");
  Serial.println("----------------------------------");

  delay(500);
}
