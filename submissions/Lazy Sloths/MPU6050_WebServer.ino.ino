#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>

#define MPU6050_ADDR 0x68
#define PWR_MGMT_1   0x6B
#define ACCEL_XOUT_H 0x3B

// WiFi credentials
const char* ssid = "Aryaman's Pixel";
const char* password = "ombx4922";
WebServer server(80);

// Sensor data variables
float accelX = 0, accelY = 0, accelZ = 0;
float gyroX = 0, gyroY = 0, gyroZ = 0;
bool isReading = true;

// Activity detection variables
float pitch = 0, roll = 0;
String currentActivity = "Unknown";
float activityConfidence = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== MPU6050 Web Server ===");
  
  // Initialize I2C
  Wire.begin(21, 22);
  Wire.setClock(400000);
  
  // Wake up MPU6050
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(PWR_MGMT_1);
  Wire.write(0);
  byte error = Wire.endTransmission();
  
  if (error == 0) {
    Serial.println("MPU6050 initialized!");
  } else {
    Serial.println("MPU6050 connection failed!");
  }
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/reset", handleReset);
  server.on("/style.css", handleCSS);
  server.on("/script.js", handleJS);
  
  server.begin();
  Serial.println("Web server started!");
}

void loop() {
  server.handleClient();
  if (isReading) {
    readMPU6050();
  }
  delay(50);
}

void readMPU6050() {
  int16_t ax, ay, az, gx, gy, gz;
  
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(ACCEL_XOUT_H);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 14, true);
  
  ax = Wire.read() << 8 | Wire.read();
  ay = Wire.read() << 8 | Wire.read();
  az = Wire.read() << 8 | Wire.read();
  Wire.read(); Wire.read(); // Skip temperature
  gx = Wire.read() << 8 | Wire.read();
  gy = Wire.read() << 8 | Wire.read();
  gz = Wire.read() << 8 | Wire.read();
  
  accelX = ax / 16384.0;
  accelY = ay / 16384.0;
  accelZ = az / 16384.0;
  gyroX = gx / 131.0;
  gyroY = gy / 131.0;
  gyroZ = gz / 131.0;
  
  // Calculate pitch and roll
  pitch = atan2(accelY, sqrt(accelX * accelX + accelZ * accelZ)) * 180.0 / PI;
  roll = atan2(-accelX, accelZ) * 180.0 / PI;
  
  // Detect activity based on tilt angles
  detectActivity();
}

void detectActivity() {
  // Activity detection based on back tilt when sensor is placed on person's back
  // pitch: forward/backward tilt (positive = leaning forward)
  // Simplified to 3 distinct activities with NO overlapping ranges
  
  float totalAccel = sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ);
  float gyroMagnitude = sqrt(gyroX * gyroX + gyroY * gyroY + gyroZ * gyroZ);
  
  // WALKING: Upright position, minimal forward lean
  // pitch: -5° to +15° (nearly vertical to slight forward)
  // Low to moderate movement from body sway
  if (pitch >= -5 && pitch < 15) {
    currentActivity = "Walking";
    activityConfidence = 92.0;
  }
  
  // SCOOTER: Moderate forward lean
  // pitch: 15° to 30° (clear forward lean but not aggressive)
  // Higher vibration from road/motor
  else if (pitch >= 15 && pitch < 30) {
    currentActivity = "Scooter";
    activityConfidence = 88.0;
  }
  
  // BIKING: Aggressive forward lean (sport/racing position)
  // pitch: 30° to 60° (significant forward lean)
  // Most aerodynamic cycling position
  else if (pitch >= 30 && pitch <= 60) {
    currentActivity = "Biking";
    activityConfidence = 90.0;
  }
  
  // Unknown - Outside normal activity ranges
  else {
    currentActivity = "Unknown";
    activityConfidence = 40.0;
  }
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MPU6050 Dashboard</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <div class="container">
        <header>
            <h1>MPU6050 Sensor Monitor</h1>
            <div class="status" id="status">Connecting...</div>
        </header>

        <div class="controls">
            <button class="btn btn-start" id="startBtn" onclick="startReading()">Start Reading</button>
            <button class="btn btn-stop" id="stopBtn" onclick="stopReading()">Stop Reading</button>
            <button class="btn btn-reset" onclick="resetValues()">Reset Display</button>
        </div>

        <div class="dashboard">
            <div class="section">
                <h2>Activity Detection</h2>
                <div class="activity-display">
                    <div class="activity-icon" id="activityIcon">🚶</div>
                    <div class="activity-name" id="activityName">Unknown</div>
                    <div class="activity-confidence">
                        <span>Confidence: </span>
                        <span id="confidence">0</span>%
                    </div>
                </div>
            </div>

            <div class="section">
                <h2>Accelerometer (g)</h2>
                <div class="metrics">
                    <div class="metric-card">
                        <div class="metric-label">X-Axis</div>
                        <div class="metric-value" id="accel-x">0.00</div>
                        <div class="metric-unit">g</div>
                    </div>
                    <div class="metric-card">
                        <div class="metric-label">Y-Axis</div>
                        <div class="metric-value" id="accel-y">0.00</div>
                        <div class="metric-unit">g</div>
                    </div>
                    <div class="metric-card">
                        <div class="metric-label">Z-Axis</div>
                        <div class="metric-value" id="accel-z">0.00</div>
                        <div class="metric-unit">g</div>
                    </div>
                </div>
            </div>

            <div class="section">
                <h2>Gyroscope (°/s)</h2>
                <div class="metrics">
                    <div class="metric-card">
                        <div class="metric-label">X-Axis</div>
                        <div class="metric-value" id="gyro-x">0.00</div>
                        <div class="metric-unit">°/s</div>
                    </div>
                    <div class="metric-card">
                        <div class="metric-label">Y-Axis</div>
                        <div class="metric-value" id="gyro-y">0.00</div>
                        <div class="metric-unit">°/s</div>
                    </div>
                    <div class="metric-card">
                        <div class="metric-label">Z-Axis</div>
                        <div class="metric-value" id="gyro-z">0.00</div>
                        <div class="metric-unit">°/s</div>
                    </div>
                </div>
            </div>

            <div class="section">
                <h2>Calculated Values</h2>
                <div class="metrics">
                    <div class="metric-card">
                        <div class="metric-label">Total Acceleration</div>
                        <div class="metric-value" id="total-accel">0.00</div>
                        <div class="metric-unit">g</div>
                    </div>
                    <div class="metric-card">
                        <div class="metric-label">Pitch</div>
                        <div class="metric-value" id="pitch">0.00</div>
                        <div class="metric-unit">°</div>
                    </div>
                    <div class="metric-card">
                        <div class="metric-label">Roll</div>
                        <div class="metric-value" id="roll">0.00</div>
                        <div class="metric-unit">°</div>
                    </div>
                </div>
            </div>
        </div>

        <footer>
            <p>Last Updated: <span id="lastUpdate">Never</span></p>
        </footer>
    </div>
    <script src="/script.js"></script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handleCSS() {
  String css = R"rawliteral(
* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: 'Arial', sans-serif;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    min-height: 100vh;
    padding: 20px;
}

.container {
    max-width: 1000px;
    margin: 0 auto;
}

header {
    background: white;
    padding: 25px;
    border-radius: 10px;
    box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
    margin-bottom: 20px;
    text-align: center;
}

h1 {
    color: #333;
    font-size: 2em;
    margin-bottom: 10px;
}

.status {
    display: inline-block;
    padding: 8px 20px;
    border-radius: 20px;
    font-weight: 600;
    font-size: 0.9em;
    transition: all 0.3s;
}

.status.connected {
    background: #4caf50;
    color: white;
}

.status.disconnected {
    background: #f44336;
    color: white;
}

.controls {
    background: white;
    padding: 20px;
    border-radius: 10px;
    box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
    margin-bottom: 20px;
    display: flex;
    gap: 15px;
    justify-content: center;
    flex-wrap: wrap;
}

.btn {
    padding: 12px 30px;
    border: none;
    border-radius: 8px;
    font-size: 1em;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.3s;
    color: white;
}

.btn:hover {
    transform: translateY(-2px);
    box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
}

.btn-start {
    background: #4caf50;
}

.btn-start:hover {
    background: #45a049;
}

.btn-stop {
    background: #f44336;
}

.btn-stop:hover {
    background: #da190b;
}

.btn-reset {
    background: #ff9800;
}

.btn-reset:hover {
    background: #e68900;
}

.dashboard {
    display: flex;
    flex-direction: column;
    gap: 20px;
}

.section {
    background: white;
    padding: 25px;
    border-radius: 10px;
    box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
}

.section h2 {
    color: #667eea;
    margin-bottom: 20px;
    font-size: 1.5em;
    border-bottom: 3px solid #667eea;
    padding-bottom: 10px;
}

.metrics {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
    gap: 15px;
}

.metric-card {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    padding: 20px;
    border-radius: 8px;
    text-align: center;
    color: white;
    transition: transform 0.3s;
}

.metric-card:hover {
    transform: scale(1.05);
}

.metric-label {
    font-size: 0.9em;
    opacity: 0.9;
    margin-bottom: 10px;
    font-weight: 600;
}

.metric-value {
    font-size: 2.2em;
    font-weight: bold;
    margin: 10px 0;
    font-family: 'Courier New', monospace;
}

.metric-unit {
    font-size: 1em;
    opacity: 0.8;
}

.metric-unit {
    font-size: 1em;
    opacity: 0.8;
}

.activity-display {
    text-align: center;
    padding: 30px;
}

.activity-icon {
    font-size: 5em;
    margin-bottom: 20px;
    animation: pulse 2s infinite;
}

@keyframes pulse {
    0%, 100% { transform: scale(1); }
    50% { transform: scale(1.1); }
}

.activity-name {
    font-size: 2.5em;
    font-weight: bold;
    color: #667eea;
    margin-bottom: 15px;
}

.activity-confidence {
    font-size: 1.2em;
    color: #666;
}

.activity-confidence span:last-child {
    font-weight: bold;
    color: #4caf50;
}

footer {
    background: white;
    padding: 15px;
    border-radius: 10px;
    box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
    margin-top: 20px;
    text-align: center;
    color: #666;
}

@media (max-width: 768px) {
    h1 {
        font-size: 1.5em;
    }
    
    .controls {
        flex-direction: column;
    }
    
    .btn {
        width: 100%;
    }
    
    .metrics {
        grid-template-columns: 1fr;
    }
}
)rawliteral";
  server.send(200, "text/css", css);
}

void handleJS() {
  String js = R"rawliteral(
let isReading = true;

async function fetchData() {
    if (!isReading) return;
    
    try {
        const response = await fetch('/data');
        const data = await response.json();
        
        updateDisplay(data);
        
        document.getElementById('status').textContent = 'Connected';
        document.getElementById('status').className = 'status connected';
        document.getElementById('lastUpdate').textContent = new Date().toLocaleTimeString();
    } catch (error) {
        document.getElementById('status').textContent = 'Disconnected';
        document.getElementById('status').className = 'status disconnected';
    }
}

function updateDisplay(data) {
    document.getElementById('accel-x').textContent = data.accelX.toFixed(2);
    document.getElementById('accel-y').textContent = data.accelY.toFixed(2);
    document.getElementById('accel-z').textContent = data.accelZ.toFixed(2);
    document.getElementById('gyro-x').textContent = data.gyroX.toFixed(2);
    document.getElementById('gyro-y').textContent = data.gyroY.toFixed(2);
    document.getElementById('gyro-z').textContent = data.gyroZ.toFixed(2);
    
    const totalAccel = Math.sqrt(
        data.accelX * data.accelX + 
        data.accelY * data.accelY + 
        data.accelZ * data.accelZ
    );
    document.getElementById('total-accel').textContent = totalAccel.toFixed(2);
    
    const pitch = Math.atan2(data.accelY, Math.sqrt(data.accelX * data.accelX + data.accelZ * data.accelZ)) * 180 / Math.PI;
    const roll = Math.atan2(-data.accelX, data.accelZ) * 180 / Math.PI;
    
    document.getElementById('pitch').textContent = pitch.toFixed(2);
    document.getElementById('roll').textContent = roll.toFixed(2);
    
    // Update activity detection display
    document.getElementById('activityName').textContent = data.activity;
    document.getElementById('confidence').textContent = data.confidence.toFixed(0);
    
    // Update activity icon
    const activityIcon = document.getElementById('activityIcon');
    switch(data.activity) {
        case 'Walking':
            activityIcon.textContent = '🚶';
            break;
        case 'Scooter':
            activityIcon.textContent = '🛴';
            break;
        case 'Biking':
            activityIcon.textContent = '🚴';
            break;
        default:
            activityIcon.textContent = '❓';
    }
}

async function startReading() {
    isReading = true;
    await fetch('/start');
    document.getElementById('startBtn').style.opacity = '0.5';
    document.getElementById('stopBtn').style.opacity = '1';
}

async function stopReading() {
    isReading = false;
    await fetch('/stop');
    document.getElementById('startBtn').style.opacity = '1';
    document.getElementById('stopBtn').style.opacity = '0.5';
}

function resetValues() {
    document.querySelectorAll('.metric-value').forEach(el => {
        el.textContent = '0.00';
    });
    document.getElementById('lastUpdate').textContent = 'Reset';
}

setInterval(fetchData, 100);
fetchData();
)rawliteral";
  server.send(200, "application/javascript", js);
}

void handleData() {
  String json = "{";
  json += "\"accelX\":" + String(accelX, 2) + ",";
  json += "\"accelY\":" + String(accelY, 2) + ",";
  json += "\"accelZ\":" + String(accelZ, 2) + ",";
  json += "\"gyroX\":" + String(gyroX, 2) + ",";
  json += "\"gyroY\":" + String(gyroY, 2) + ",";
  json += "\"gyroZ\":" + String(gyroZ, 2) + ",";
  json += "\"pitch\":" + String(pitch, 2) + ",";
  json += "\"roll\":" + String(roll, 2) + ",";
  json += "\"activity\":\"" + currentActivity + "\",";
  json += "\"confidence\":" + String(activityConfidence, 1);
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleStart() {
  isReading = true;
  server.send(200, "text/plain", "Started");
}

void handleStop() {
  isReading = false;
  server.send(200, "text/plain", "Stopped");
}

void handleReset() {
  accelX = accelY = accelZ = 0;
  gyroX = gyroY = gyroZ = 0;
  server.send(200, "text/plain", "Reset");
}