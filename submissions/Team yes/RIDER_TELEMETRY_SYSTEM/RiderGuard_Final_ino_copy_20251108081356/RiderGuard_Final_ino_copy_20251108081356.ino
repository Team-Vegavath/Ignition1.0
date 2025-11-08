/*
 * RIDER TELEMETRY SYSTEM
 * ESP32 + MPU6050 + GPS Module
 * 
 * Connections:
 * MPU6050:  VCC->3.3V, GND->GND, SDA->GPIO21, SCL->GPIO22
 * GPS:      VCC->3.3V, GND->GND, TX->GPIO16, RX->GPIO17
 * 
 * Libraries needed:
 * - TinyGPS++ (install from Library Manager)
 * - Adafruit MPU6050 (install from Library Manager)
 * - Adafruit Unified Sensor (install from Library Manager)
 * - ESP32 BLE Arduino (built-in with ESP32 board)
 */

#include <Wire.h>
#include <TinyGPSPlus.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// BLE UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

// MPU6050
Adafruit_MPU6050 mpu;

// GPS
TinyGPSPlus gps;
HardwareSerial GPS_Serial(2);

// Pins
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17
#define LED_PIN 2

// Telemetry Data
struct TelemetryData {
  float accelX, accelY, accelZ;
  float gyroX, gyroY, gyroZ;
  float speed;
  float deceleration;
  double latitude, longitude;
  float heading;
  unsigned long timestamp;
  int rideState;
  int gpsQuality;
  float leanAngle;
  bool harshBraking;
} data;

// Variables
float prevSpeed = 0;
float prevHeading = 0;
unsigned long lastGPSUpdate = 0;
unsigned long lastDataSend = 0;
float gyroOffsetX = 0, gyroOffsetY = 0, gyroOffsetZ = 0;

// Data logging
#define MAX_LOG_SIZE 2000
struct LogEntry {
  unsigned long timestamp;
  float speed;
  float accelX;
  float gyroY;
  double lat, lon;
  int state;
};
LogEntry dataLog[MAX_LOG_SIZE];
int logIndex = 0;

// BLE Callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device connected");
      digitalWrite(LED_PIN, HIGH);
    }
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device disconnected");
      digitalWrite(LED_PIN, LOW);
      BLEDevice::startAdvertising();
    }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  
  Serial.println("\n================================");
  Serial.println("RIDER TELEMETRY SYSTEM");
  Serial.println("MPU6050 + GPS Configuration");
  Serial.println("================================\n");
  
  // I2C Init
  Wire.begin(21, 22);
  Wire.setClock(400000);
  
  // GPS Init
  GPS_Serial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("GPS initialized (RX=16, TX=17)");
  
  // MPU6050 Init
  Serial.println("\nInitializing MPU6050...");
  if (!mpu.begin()) {
    Serial.println("ERROR: MPU6050 NOT FOUND!");
    Serial.println("Check: VCC->3.3V, GND->GND, SDA->21, SCL->22");
    while(1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(200);
    }
  }
  Serial.println("MPU6050 connected!");
  
  // Configure MPU6050
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  
  Serial.println("MPU6050 Range: ±8G, ±2000°/s");
  
  // Calibration
  Serial.println("\nCALIBRATING - Keep STILL!");
  Serial.println("Starting in 2 seconds...");
  delay(2000);
  calibrateGyro();
  Serial.println("Calibration done!\n");
  
  // BLE Init
  Serial.println("Starting Bluetooth...");
  BLEDevice::init("RiderTelemetry");
  
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE: RiderTelemetry");
  Serial.println("\nSYSTEM READY!");
  Serial.println("Waiting for GPS lock...\n");
  
  // Ready blink
  for(int i=0; i<3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
}

void loop() {
  unsigned long currentTime = millis();
  
  // Read MPU6050
  readMPU6050();
  
  // Read GPS
  bool newGPSData = false;
  while (GPS_Serial.available() > 0) {
    if (gps.encode(GPS_Serial.read())) {
      if (gps.location.isValid()) {
        data.latitude = gps.location.lat();
        data.longitude = gps.location.lng();
        data.speed = gps.speed.kmph();
        data.heading = gps.course.deg();
        data.gpsQuality = gps.satellites.value();
        
        // Calculate acceleration
        float deltaTime = (currentTime - lastGPSUpdate) / 1000.0;
        if (deltaTime > 0 && lastGPSUpdate > 0) {
          float speedChangeKmh = data.speed - prevSpeed;
          float speedChangeMs = speedChangeKmh / 3.6;
          
          data.accelX = speedChangeMs / deltaTime;
          data.deceleration = -data.accelX;
          data.harshBraking = (data.deceleration > 3.0);
          
          // Lateral accel from heading change
          float headingChange = data.heading - prevHeading;
          if (headingChange > 180) headingChange -= 360;
          if (headingChange < -180) headingChange += 360;
          
          if (data.speed > 5.0) {
            float turnRate = abs(headingChange) / deltaTime;
            float speedMs = data.speed / 3.6;
            data.accelY = (turnRate * 0.01745) * speedMs;
          }
        }
        
        prevSpeed = data.speed;
        prevHeading = data.heading;
        lastGPSUpdate = currentTime;
        newGPSData = true;
      }
    }
  }
  
  // Lean angle estimation
  static float estimatedRoll = 0;
  estimatedRoll += data.gyroY * 0.02;
  estimatedRoll *= 0.98;
  data.leanAngle = estimatedRoll;
  
  // Detect ride state
  data.rideState = detectRideState();
  data.timestamp = currentTime;
  
  // Log data
  if (newGPSData) {
    dataLog[logIndex].timestamp = data.timestamp;
    dataLog[logIndex].speed = data.speed;
    dataLog[logIndex].accelX = data.accelX;
    dataLog[logIndex].gyroY = data.gyroY;
    dataLog[logIndex].lat = data.latitude;
    dataLog[logIndex].lon = data.longitude;
    dataLog[logIndex].state = data.rideState;
    logIndex = (logIndex + 1) % MAX_LOG_SIZE;
  }
  
  // Send BLE data at 10Hz
  if (deviceConnected && (currentTime - lastDataSend >= 100)) {
    String dataString = createDataPacket();
    pCharacteristic->setValue(dataString.c_str());
    pCharacteristic->notify();
    lastDataSend = currentTime;
  }
  
  // Print status every second
  static unsigned long lastPrint = 0;
  if (currentTime - lastPrint >= 1000) {
    printSensorData();
    lastPrint = currentTime;
  }
  
  delay(20);
}

void calibrateGyro() {
  const int samples = 150;
  float sumX = 0, sumY = 0, sumZ = 0;
  
  Serial.print("Calibrating");
  for (int i = 0; i < samples; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    
    sumX += g.gyro.x * 57.2958; // Convert to deg/s
    sumY += g.gyro.y * 57.2958;
    sumZ += g.gyro.z * 57.2958;
    
    if (i % 30 == 0) Serial.print(".");
    delay(10);
  }
  Serial.println();
  
  gyroOffsetX = sumX / samples;
  gyroOffsetY = sumY / samples;
  gyroOffsetZ = sumZ / samples;
  
  Serial.print("Offsets: X=");
  Serial.print(gyroOffsetX, 2);
  Serial.print(" Y=");
  Serial.print(gyroOffsetY, 2);
  Serial.print(" Z=");
  Serial.println(gyroOffsetZ, 2);
}

void readMPU6050() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  
  // Convert gyro from rad/s to deg/s and apply calibration
  data.gyroX = (g.gyro.x * 57.2958) - gyroOffsetX;
  data.gyroY = (g.gyro.y * 57.2958) - gyroOffsetY;
  data.gyroZ = (g.gyro.z * 57.2958) - gyroOffsetZ;
  
  // Store accelerometer data (m/s²)
  data.accelZ = a.acceleration.z;
}

int detectRideState() {
  float gyroMag = sqrt(data.gyroX*data.gyroX + data.gyroY*data.gyroY + data.gyroZ*data.gyroZ);
  
  // Walking
  if (data.speed < 8.0 && gyroMag < 80.0) return 0;
  
  // Scooter
  if (data.speed >= 8.0 && data.speed < 35.0) {
    if (abs(data.gyroY) < 60.0) return 1;
  }
  
  // Motorcycle
  if (data.speed >= 35.0) return 2;
  if (data.speed >= 15.0 && abs(data.gyroY) > 60.0) return 2;
  
  return 0;
}

String createDataPacket() {
  String packet = "";
  packet += String(data.timestamp) + ",";
  packet += String(data.accelX, 3) + ",";
  packet += String(data.accelY, 3) + ",";
  packet += String(data.accelZ, 3) + ",";
  packet += String(data.gyroX, 2) + ",";
  packet += String(data.gyroY, 2) + ",";
  packet += String(data.gyroZ, 2) + ",";
  packet += String(data.speed, 2) + ",";
  packet += String(data.latitude, 6) + ",";
  packet += String(data.longitude, 6) + ",";
  packet += String(data.rideState) + ",";
  packet += String(data.gpsQuality) + ",";
  packet += String(data.leanAngle, 1) + ",";
  packet += String(data.harshBraking ? 1 : 0);
  return packet;
}

void printSensorData() {
  Serial.println("========================================");
  
  // Gyro
  Serial.print("GYRO: X=");
  Serial.print(data.gyroX, 1);
  Serial.print(" Y=");
  Serial.print(data.gyroY, 1);
  Serial.print(" Z=");
  Serial.print(data.gyroZ, 1);
  Serial.println(" deg/s");
  
  // Accel
  Serial.print("ACCEL: Fwd=");
  Serial.print(data.accelX, 2);
  Serial.print(" Lat=");
  Serial.print(data.accelY, 2);
  Serial.print(" m/s2");
  if (data.harshBraking) Serial.print(" [HARSH BRAKE!]");
  Serial.println();
  
  // Speed
  Serial.print("SPEED: ");
  Serial.print(data.speed, 1);
  Serial.print(" km/h | Decel: ");
  Serial.print(data.deceleration, 2);
  Serial.println(" m/s2");
  
  // Lean
  Serial.print("LEAN: ");
  Serial.print(data.leanAngle, 1);
  Serial.println(" deg");
  
  // GPS
  Serial.print("GPS: ");
  if (gps.location.isValid()) {
    Serial.print(data.latitude, 6);
    Serial.print(", ");
    Serial.print(data.longitude, 6);
    Serial.print(" | ");
    Serial.print(data.gpsQuality);
    Serial.println(" sats");
  } else {
    Serial.print("Searching... (");
    Serial.print(gps.satellites.value());
    Serial.println(" sats)");
  }
  
  // State
  String states[] = {"WALKING", "SCOOTER", "MOTORCYCLE"};
  Serial.print("STATE: ");
  Serial.println(states[data.rideState]);
  
  // Log
  Serial.print("LOG: ");
  Serial.print(logIndex);
  Serial.print("/");
  Serial.println(MAX_LOG_SIZE);
  
  // BLE
  Serial.print("BLE: ");
  Serial.println(deviceConnected ? "CONNECTED" : "Waiting...");
  
  Serial.println();
}