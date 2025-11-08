#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <MPU6050.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <math.h>

// =========================
// Hardware Setup
// =========================
MPU6050 mpu;
float ax, ay, az, gx, gy, gz;
String motionState = "Calibrating...";

TinyGPSPlus gps;
HardwareSerial GPS(1); // UART1
float latitude=0, longitude=0, speedKmph=0;
String gpsTime="No Fix";

WebServer server(80);
const char* ssid = "ESP32_DataLogger";
const char* password = "12345678";

unsigned long lastUpdate = 0;
unsigned long interval = 200; // 5 readings/sec

// Moving average buffer for variance calculation
const int avgWindow = 10;
float accelMagBuffer[avgWindow];
int bufferIndex = 0;

// =========================
// Motion detection function
// =========================
String detectMotionType(float ax, float ay, float az, float gx, float gy, float gz) {
  float accelMag = sqrt(ax*ax + ay*ay + az*az);
  float gyroMag = sqrt(gx*gx + gy*gy + gz*gz);

  accelMagBuffer[bufferIndex] = accelMag;
  bufferIndex = (bufferIndex + 1) % avgWindow;

  float mean = 0, variance = 0;
  for (int i = 0; i < avgWindow; i++) mean += accelMagBuffer[i];
  mean /= avgWindow;
  for (int i = 0; i < avgWindow; i++) variance += pow(accelMagBuffer[i]-mean,2);
  variance /= avgWindow;

  if (variance < 0.05 && gyroMag < 1) return "Idle";
  else if (variance < 0.8 && gyroMag < 3) return "Walking";
  else return "Biking";
}

// =========================
// LeanMeter calculation
// =========================
float calculateLeanAngle(float ax, float ay, float az) {
  // Roll: side-to-side tilt
  float roll  = atan2(ay, az) * 180 / PI;
  return roll;
}

// =========================
// Webpage generation
// =========================
String htmlPage(float leanAngle) {
  String color = "green";
  if (abs(leanAngle) > 30 && abs(leanAngle) <= 45) color = "orange";
  else if (abs(leanAngle) > 45) color = "red";

  String page = "<html><head><meta http-equiv='refresh' content='3'>";
  page += "<style>body{font-family:Arial;text-align:center;background:#f4f4f4;}";
  page += ".card{display:inline-block;padding:20px;background:white;border-radius:10px;";
  page += "box-shadow:0 0 10px rgba(0,0,0,0.2);width:350px;}";
  page += "h1{color:#333;} .status{font-size:1.2em;margin:5px;}";
  page += ".lean{font-size:1.5em;font-weight:bold;color:" + color + ";}</style></head><body>";
  page += "<div class='card'><h1>🚴 Ride Telemetry Dashboard</h1>";
  page += "<div class='status'><b>Status:</b> " + motionState + "</div>";
  page += "<div class='status'><b>LeanMeter Pro:</b> <span class='lean'>" + String(leanAngle,2) + "°</span></div>";

  page += "<div class='status'><b>Acceleration (g):</b> X=" + String(ax,2) + " | Y=" + String(ay,2) + " | Z=" + String(az,2) + "</div>";
  page += "<div class='status'><b>Gyroscope (°/s):</b> X=" + String(gx,2) + " | Y=" + String(gy,2) + " | Z=" + String(gz,2) + "</div>";
  page += "<div class='status'><b>GPS:</b> Lat=" + String(latitude,6) + " | Lon=" + String(longitude,6) + " | Speed=" + String(speedKmph,2) + " km/h</div>";
  page += "<div class='status'><b>Time:</b> " + gpsTime + "</div>";
  page += "</div></body></html>";
  return page;
}

void handleRoot() {
  float leanAngle = calculateLeanAngle(ax, ay, az);
  server.send(200,"text/html", htmlPage(leanAngle));
}

// =========================
// Setup
// =========================
void setup() {
  Serial.begin(115200);
  Wire.begin();
  GPS.begin(9600, SERIAL_8N1, 2, 4);

  Serial.println("Starting MPU6050...");
  mpu.initialize();
  if(!mpu.testConnection()){ Serial.println("MPU6050 failed!"); while(1); }
  Serial.println("MPU ready!");

  // WiFi AP
  WiFi.softAP(ssid,password);
  Serial.println("WiFi started. Connect to: " + String(ssid));
  Serial.println("IP: " + WiFi.softAPIP().toString());

  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web server running...");
}

// =========================
// Main loop
// =========================
void loop() {
  server.handleClient();

  // GPS reading
  while(GPS.available()) { gps.encode(GPS.read()); }
  if(gps.location.isValid()) {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
    speedKmph = gps.speed.kmph();
    gpsTime = String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second());
  } else { gpsTime = "No Fix"; }

  // MPU reading
  unsigned long now = millis();
  if(now - lastUpdate > interval) {
    lastUpdate = now;

    int16_t rawAx, rawAy, rawAz, rawGx, rawGy, rawGz;
    mpu.getMotion6(&rawAx,&rawAy,&rawAz,&rawGx,&rawGy,&rawGz);

    ax = rawAx/16384.0;
    ay = rawAy/16384.0;
    az = rawAz/16384.0;
    gx = rawGx/131.0;
    gy = rawGy/131.0;
    gz = rawGz/131.0;

    motionState = detectMotionType(ax,ay,az,gx,gy,gz);
  }
}
