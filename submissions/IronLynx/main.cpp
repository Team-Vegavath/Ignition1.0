#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TinyGPSPlus.h>
#include <math.h>

// ---------------- Wi-Fi Credentials ----------------
const char* ssid = "ESP32-AP";
const char* password = "123456789";

// ---------------- Sensor Objects ----------------
Adafruit_MPU6050 mpu;
TinyGPSPlus gps;
HardwareSerial GPSSerial(1); // UART1 for Neo-6M GPS

// ---------------- Server ----------------
WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  delay(100);

  // Initialize Wi-Fi
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
  server.begin();

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    while (1) { delay(10); }
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  delay(100);

  // Initialize GPS (Neo-6M)
  GPSSerial.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17
}

void loop() {
  // ---------------- Read GPS ----------------
  while(GPSSerial.available() > 0) gps.encode(GPSSerial.read());

  // ---------------- Read MPU ----------------
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float ax = a.acceleration.x;
  float ay = a.acceleration.y;
  float az = a.acceleration.z;

  // Pitch for tilt
  float pitch = atan2(ax, sqrt(ay*ay + az*az)) * 180.0 / PI;
  float accelMag = sqrt(ax*ax + ay*ay + az*az);

  // ---------------- Determine Mode ----------------
  float speedKmph = gps.speed.kmph();  // ✅ Fixed function call
  String mode = "Unknown";

  if(speedKmph < 5 && accelMag > 0.5 && accelMag < 3.0){
      mode = "Walking";
  } else if(speedKmph >= 5 && speedKmph < 25){
      if(abs(pitch) > 10) mode = "Bike";
      else mode = "Scooter";
  } else if(speedKmph >= 25){
      mode = "Bike";
  }

  // ---------------- Handle Clients ----------------
  WiFiClient client = server.available();
  if(client){
    Serial.println("Client connected");
    String request = client.readStringUntil('\n');

    // SSE response
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/event-stream");
    client.println("Cache-Control: no-cache");
    client.println("Connection: keep-alive");
    client.println();

    while(client.connected()){
      // Build JSON
      String json = "{";
      json += "\"ax\":" + String(ax,2);
      json += ",\"ay\":" + String(ay,2);
      json += ",\"az\":" + String(az,2);
      json += ",\"gx\":" + String(g.gyro.x,2);
      json += ",\"gy\":" + String(g.gyro.y,2);
      json += ",\"gz\":" + String(g.gyro.z,2);
      json += ",\"temp\":" + String(temp.temperature,2);
      json += ",\"lat\":" + String(gps.location.isValid()? gps.location.lat():0.0,6);
      json += ",\"lon\":" + String(gps.location.isValid()? gps.location.lng():0.0,6);
      json += ",\"speed\":" + String(speedKmph,2);
      json += ",\"mode\":\"" + mode + "\"";
      json += "}";

      // Send SSE event
      client.print("data: ");
      client.println(json);
      client.println();

      delay(1000); // update every second
    }

    Serial.println("Client disconnected");
    client.stop();
  }
}