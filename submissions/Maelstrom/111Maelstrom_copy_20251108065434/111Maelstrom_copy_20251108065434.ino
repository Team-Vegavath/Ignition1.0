#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

// WiFi credentials
const char* ssid = "PESU-EC-Campus";
const char* password = "PESU-EC-Campus";

WebServer server(80);

// MPU6050
uint8_t mpuAddress = 0x68;

// Sensor data
float accelX, accelY, accelZ;
float gyroX, gyroY, gyroZ;
bool motionDetected = false;

// GPS data from phone
float gpsLat = 0.0;
float gpsLon = 0.0;
float gpsSpeed = 0.0;
float gpsAltitude = 0.0;
int gpsSatellites = 0;
String gpsTime = "00:00:00";
bool gpsValid = false;
unsigned long lastGpsUpdate = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize MPU6050
  Wire.begin();
  Wire.beginTransmission(mpuAddress);
  Wire.write(0x6B); // PWR_MGMT_1
  Wire.write(0);    // Wake up
  Wire.endTransmission(true);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP address: ");
  Serial.println(WiFi.localIP());
  
  // Setup server routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/gps", handleGPS);
  server.on("/style.css", handleCSS);
  server.on("/manual", handleManualGPS);
  
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("Open http://" + WiFi.localIP().toString() + " in your browser");
}

void loop() {
  server.handleClient();
  readSensorData();
  
  // Check if GPS data is stale (older than 30 seconds)
  if (millis() - lastGpsUpdate > 30000) {
    gpsValid = false;
  }
  
  delay(100);
}

void readSensorData() {
  int16_t accelRaw[3], gyroRaw[3];
  
  // Read accelerometer
  Wire.beginTransmission(mpuAddress);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(mpuAddress, (uint8_t)6, (uint8_t)true);
  accelRaw[0] = Wire.read() << 8 | Wire.read();
  accelRaw[1] = Wire.read() << 8 | Wire.read();
  accelRaw[2] = Wire.read() << 8 | Wire.read();
  
  // Read gyroscope
  Wire.beginTransmission(mpuAddress);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(mpuAddress, (uint8_t)6, (uint8_t)true);
  gyroRaw[0] = Wire.read() << 8 | Wire.read();
  gyroRaw[1] = Wire.read() << 8 | Wire.read();
  gyroRaw[2] = Wire.read() << 8 | Wire.read();
  
  // Convert to real units
  accelX = accelRaw[0] / 16384.0;
  accelY = accelRaw[1] / 16384.0;
  accelZ = accelRaw[2] / 16384.0;
  
  gyroX = gyroRaw[0] / 131.0;
  gyroY = gyroRaw[1] / 131.0;
  gyroZ = gyroRaw[2] / 131.0;
  
  // Motion detection
  float accelMag = sqrt(accelX*accelX + accelY*accelY + accelZ*accelZ);
  float gyroMag = sqrt(gyroX*gyroX + gyroY*gyroY + gyroZ*gyroZ);
  motionDetected = (abs(accelMag - 1.0) > 0.3) || (gyroMag > 20.0);
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Sensor & GPS Monitor</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" href="/style.css">
    <script>
        function updateData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    // Update sensor data
                    document.getElementById('accelX').textContent = data.accelX.toFixed(3);
                    document.getElementById('accelY').textContent = data.accelY.toFixed(3);
                    document.getElementById('accelZ').textContent = data.accelZ.toFixed(3);
                    document.getElementById('gyroX').textContent = data.gyroX.toFixed(2);
                    document.getElementById('gyroY').textContent = data.gyroY.toFixed(2);
                    document.getElementById('gyroZ').textContent = data.gyroZ.toFixed(2);
                    
                    // Update motion indicator
                    const motionIndicator = document.getElementById('motionIndicator');
                    if (data.motion) {
                        motionIndicator.className = 'motion-yes';
                        motionIndicator.textContent = 'MOTION DETECTED!';
                    } else {
                        motionIndicator.className = 'motion-no';
                        motionIndicator.textContent = 'No Motion';
                    }
                    
                    // Update GPS data
                    document.getElementById('gpsLat').textContent = data.gpsLat.toFixed(6);
                    document.getElementById('gpsLon').textContent = data.gpsLon.toFixed(6);
                    document.getElementById('gpsSpeed').textContent = data.gpsSpeed.toFixed(1);
                    document.getElementById('gpsAlt').textContent = data.gpsAltitude.toFixed(1);
                    document.getElementById('gpsSats').textContent = data.gpsSatellites;
                    document.getElementById('gpsTime').textContent = data.gpsTime;
                    
                    // Update GPS status
                    const gpsStatus = document.getElementById('gpsStatus');
                    if (data.gpsValid) {
                        gpsStatus.className = 'gps-valid';
                        gpsStatus.textContent = 'GPS ACTIVE';
                    } else {
                        gpsStatus.className = 'gps-invalid';
                        gpsStatus.textContent = 'NO GPS SIGNAL';
                    }
                    
                    // Update progress bars
                    updateProgressBar('accelXBar', Math.abs(data.accelX), 2);
                    updateProgressBar('accelYBar', Math.abs(data.accelY), 2);
                    updateProgressBar('accelZBar', Math.abs(data.accelZ), 2);
                    updateProgressBar('gyroXBar', Math.abs(data.gyroX), 200);
                    updateProgressBar('gyroYBar', Math.abs(data.gyroY), 200);
                    updateProgressBar('gyroZBar', Math.abs(data.gyroZ), 200);
                })
                .catch(error => console.error('Error:', error));
        }
        
        function updateProgressBar(barId, value, max) {
            const bar = document.getElementById(barId);
            const percentage = Math.min((value / max) * 100, 100);
            bar.style.width = percentage + '%';
            
            if (percentage < 30) {
                bar.className = 'progress-fill low';
            } else if (percentage < 70) {
                bar.className = 'progress-fill medium';
            } else {
                bar.className = 'progress-fill high';
            }
        }
        
        function getLocation() {
            if (navigator.geolocation) {
                document.getElementById('shareStatus').textContent = 'Getting location...';
                
                navigator.geolocation.getCurrentPosition(
                    function(position) {
                        const lat = position.coords.latitude;
                        const lon = position.coords.longitude;
                        const speed = (position.coords.speed || 0) * 3.6; // Convert m/s to km/h
                        const alt = position.coords.altitude || 0;
                        const time = new Date().toLocaleTimeString();
                        
                        // Send GPS data to ESP32
                        const url = `/gps?lat=${lat}&lon=${lon}&speed=${speed}&alt=${alt}&sats=8&time=${encodeURIComponent(time)}`;
                        
                        fetch(url)
                            .then(response => response.text())
                            .then(data => {
                                console.log('GPS data sent:', data);
                                document.getElementById('shareStatus').textContent = 'Location shared successfully!';
                                updateData(); // Refresh data immediately
                                setTimeout(() => {
                                    document.getElementById('shareStatus').textContent = '';
                                }, 3000);
                            })
                            .catch(error => {
                                console.error('Error sending GPS:', error);
                                document.getElementById('shareStatus').textContent = 'Error sending location';
                            });
                    },
                    function(error) {
                        console.error('Geolocation error:', error);
                        let errorMsg = 'Error getting location: ';
                        switch(error.code) {
                            case error.PERMISSION_DENIED:
                                errorMsg += 'Permission denied. Please allow location access.';
                                break;
                            case error.POSITION_UNAVAILABLE:
                                errorMsg += 'Location unavailable.';
                                break;
                            case error.TIMEOUT:
                                errorMsg += 'Location request timeout.';
                                break;
                            default:
                                errorMsg += 'Unknown error.';
                        }
                        document.getElementById('shareStatus').textContent = errorMsg;
                        
                        // Show manual input option
                        document.getElementById('manualInput').style.display = 'block';
                    },
                    {
                        enableHighAccuracy: true,
                        timeout: 10000,
                        maximumAge: 0
                    }
                );
            } else {
                document.getElementById('shareStatus').textContent = 'Geolocation not supported by this browser.';
                document.getElementById('manualInput').style.display = 'block';
            }
        }
        
        function startAutoShare() {
            if (navigator.geolocation) {
                const watchId = navigator.geolocation.watchPosition(
                    function(position) {
                        const lat = position.coords.latitude;
                        const lon = position.coords.longitude;
                        const speed = (position.coords.speed || 0) * 3.6;
                        const alt = position.coords.altitude || 0;
                        const time = new Date().toLocaleTimeString();
                        
                        fetch(`/gps?lat=${lat}&lon=${lon}&speed=${speed}&alt=${alt}&sats=8&time=${encodeURIComponent(time)}`)
                            .then(response => response.text())
                            .then(data => {
                                console.log('Auto GPS update:', data);
                                updateData();
                            });
                    },
                    function(error) {
                        console.error('Auto-share error:', error);
                        document.getElementById('autoShareStatus').textContent = 'Auto-share error';
                    },
                    {
                        enableHighAccuracy: true,
                        timeout: 5000,
                        maximumAge: 10000
                    }
                );
                
                document.getElementById('autoShareStatus').textContent = 'Auto-sharing active';
                document.getElementById('stopAutoShare').onclick = function() {
                    navigator.geolocation.clearWatch(watchId);
                    document.getElementById('autoShareStatus').textContent = 'Auto-sharing stopped';
                };
                
            } else {
                document.getElementById('autoShareStatus').textContent = 'Geolocation not supported';
            }
        }
        
        function submitManualLocation() {
            const lat = document.getElementById('manualLat').value;
            const lon = document.getElementById('manualLon').value;
            
            if (lat && lon) {
                fetch(`/gps?lat=${lat}&lon=${lon}&speed=0&alt=0&sats=0&time=${encodeURIComponent(new Date().toLocaleTimeString())}`)
                    .then(response => response.text())
                    .then(data => {
                        document.getElementById('manualStatus').textContent = 'Manual location set!';
                        updateData();
                        setTimeout(() => {
                            document.getElementById('manualStatus').textContent = '';
                            document.getElementById('manualInput').style.display = 'none';
                        }, 3000);
                    });
            } else {
                document.getElementById('manualStatus').textContent = 'Please enter both latitude and longitude';
            }
        }
        
        // Use phone's location automatically on page load (if permission granted)
        function tryAutoLocation() {
            if (navigator.geolocation) {
                navigator.geolocation.getCurrentPosition(
                    function(position) {
                        // Auto-share if permission already granted
                        const lat = position.coords.latitude;
                        const lon = position.coords.longitude;
                        const speed = (position.coords.speed || 0) * 3.6;
                        const alt = position.coords.altitude || 0;
                        
                        fetch(`/gps?lat=${lat}&lon=${lon}&speed=${speed}&alt=${alt}&sats=8&time=${encodeURIComponent(new Date().toLocaleTimeString())}`);
                    },
                    function(error) {
                        // Silent fail - user hasn't granted permission yet
                        console.log('Auto-location not available');
                    },
                    {
                        enableHighAccuracy: false,
                        timeout: 3000,
                        maximumAge: 30000
                    }
                );
            }
        }
        
        // Initialize
        setInterval(updateData, 1000);
        window.onload = function() {
            updateData();
            tryAutoLocation(); // Try to get location automatically
        };
    </script>
</head>
<body>
    <div class="container">
        <h1>🎯 Sensor & GPS Monitor</h1>
        
        <div class="status-grid">
            <div class="motion-indicator">
                <div id="motionIndicator" class="motion-no">No Motion</div>
            </div>
            <div class="gps-indicator">
                <div id="gpsStatus" class="gps-invalid">NO GPS SIGNAL</div>
            </div>
        </div>
        
        <div class="sensor-grid">
            <div class="sensor-card">
                <h2>📊 Accelerometer (g)</h2>
                <div class="data-row">
                    <span>X:</span>
                    <span id="accelX">0.000</span>
                    <div class="progress-bar">
                        <div id="accelXBar" class="progress-fill"></div>
                    </div>
                </div>
                <div class="data-row">
                    <span>Y:</span>
                    <span id="accelY">0.000</span>
                    <div class="progress-bar">
                        <div id="accelYBar" class="progress-fill"></div>
                    </div>
                </div>
                <div class="data-row">
                    <span>Z:</span>
                    <span id="accelZ">0.000</span>
                    <div class="progress-bar">
                        <div id="accelZBar" class="progress-fill"></div>
                    </div>
                </div>
            </div>
            
            <div class="sensor-card">
                <h2>🔄 Gyroscope (°/s)</h2>
                <div class="data-row">
                    <span>X:</span>
                    <span id="gyroX">0.00</span>
                    <div class="progress-bar">
                        <div id="gyroXBar" class="progress-fill"></div>
                    </div>
                </div>
                <div class="data-row">
                    <span>Y:</span>
                    <span id="gyroY">0.00</span>
                    <div class="progress-bar">
                        <div id="gyroYBar" class="progress-fill"></div>
                    </div>
                </div>
                <div class="data-row">
                    <span>Z:</span>
                    <span id="gyroZ">0.00</span>
                    <div class="progress-bar">
                        <div id="gyroZBar" class="progress-fill"></div>
                    </div>
                </div>
            </div>
            
            <div class="sensor-card">
                <h2>📍 GPS Data</h2>
                <div class="data-row">
                    <span>Lat:</span>
                    <span id="gpsLat">0.000000</span>
                </div>
                <div class="data-row">
                    <span>Lon:</span>
                    <span id="gpsLon">0.000000</span>
                </div>
                <div class="data-row">
                    <span>Speed:</span>
                    <span id="gpsSpeed">0.0</span><span>km/h</span>
                </div>
                <div class="data-row">
                    <span>Alt:</span>
                    <span id="gpsAlt">0.0</span><span>m</span>
                </div>
                <div class="data-row">
                    <span>Sats:</span>
                    <span id="gpsSats">0</span>
                </div>
                <div class="data-row">
                    <span>Time:</span>
                    <span id="gpsTime">00:00:00</span>
                </div>
            </div>
        </div>
        
        <div class="gps-controls">
            <button onclick="getLocation()">📍 Share Current Location</button>
            <button onclick="startAutoShare()">🔄 Start Auto-Share</button>
            <button id="stopAutoShare">⏹️ Stop Auto-Share</button>
            <div id="shareStatus" class="share-status"></div>
            <div id="autoShareStatus" class="share-status"></div>
            
            <div id="manualInput" class="manual-input" style="display: none;">
                <h3>Manual Location Input</h3>
                <div class="input-group">
                    <input type="text" id="manualLat" placeholder="Latitude (e.g., 40.7128)">
                    <input type="text" id="manualLon" placeholder="Longitude (e.g., -74.0060)">
                    <button onclick="submitManualLocation()">Set Location</button>
                </div>
                <div id="manualStatus" class="share-status"></div>
            </div>
        </div>
        
        <div class="info">
            <p>📡 Connected to: )rawliteral" + WiFi.localIP().toString() + R"rawliteral(</p>
            <p>🕒 Auto-updates every second</p>
            <p>📍 <strong>Tip:</strong> Allow location access when browser asks</p>
        </div>
    </div>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleData() {
  String json = "{";
  json += "\"accelX\":" + String(accelX, 3) + ",";
  json += "\"accelY\":" + String(accelY, 3) + ",";
  json += "\"accelZ\":" + String(accelZ, 3) + ",";
  json += "\"gyroX\":" + String(gyroX, 2) + ",";
  json += "\"gyroY\":" + String(gyroY, 2) + ",";
  json += "\"gyroZ\":" + String(gyroZ, 2) + ",";
  json += "\"motion\":" + String(motionDetected ? "true" : "false") + ",";
  json += "\"gpsLat\":" + String(gpsLat, 6) + ",";
  json += "\"gpsLon\":" + String(gpsLon, 6) + ",";
  json += "\"gpsSpeed\":" + String(gpsSpeed, 1) + ",";
  json += "\"gpsAltitude\":" + String(gpsAltitude, 1) + ",";
  json += "\"gpsSatellites\":" + String(gpsSatellites) + ",";
  json += "\"gpsTime\":\"" + gpsTime + "\",";
  json += "\"gpsValid\":" + String(gpsValid ? "true" : "false");
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleGPS() {
  if (server.hasArg("lat") && server.hasArg("lon")) {
    gpsLat = server.arg("lat").toFloat();
    gpsLon = server.arg("lon").toFloat();
    gpsSpeed = server.hasArg("speed") ? server.arg("speed").toFloat() : 0.0;
    gpsAltitude = server.hasArg("alt") ? server.arg("alt").toFloat() : 0.0;
    gpsSatellites = server.hasArg("sats") ? server.arg("sats").toInt() : 0;
    gpsTime = server.hasArg("time") ? server.arg("time") : "00:00:00";
    gpsValid = true;
    lastGpsUpdate = millis();
    
    Serial.println("GPS Updated: Lat=" + String(gpsLat, 6) + 
                   ", Lon=" + String(gpsLon, 6) + 
                   ", Speed=" + String(gpsSpeed, 1) + "km/h");
    
    server.send(200, "text/plain", "GPS data received");
  } else {
    server.send(400, "text/plain", "Missing lat/lon parameters");
  }
}

void handleManualGPS() {
  // Alternative endpoint for manual GPS input
  String html = "<html><body><h1>Manual GPS Input</h1>";
  html += "<form action='/gps' method='GET'>";
  html += "Latitude: <input type='text' name='lat'><br>";
  html += "Longitude: <input type='text' name='lon'><br>";
  html += "<input type='submit' value='Set Location'>";
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}

void handleCSS() {
  String css = R"rawliteral(
body {
    font-family: Arial, sans-serif;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    margin: 0;
    padding: 20px;
    min-height: 100vh;
}

.container {
    max-width: 1000px;
    margin: 0 auto;
    background: white;
    padding: 30px;
    border-radius: 15px;
    box-shadow: 0 10px 30px rgba(0,0,0,0.2);
}

h1 {
    text-align: center;
    color: #333;
    margin-bottom: 30px;
}

.status-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 20px;
    margin: 20px 0;
}

.motion-indicator, .gps-indicator {
    text-align: center;
}

.motion-yes {
    background: #ff4444;
    color: white;
    padding: 15px;
    border-radius: 25px;
    font-weight: bold;
    font-size: 1.1em;
    animation: pulse 1s infinite;
}

.motion-no {
    background: #44ff44;
    color: white;
    padding: 15px;
    border-radius: 25px;
    font-weight: bold;
    font-size: 1.1em;
}

.gps-valid {
    background: #4488ff;
    color: white;
    padding: 15px;
    border-radius: 25px;
    font-weight: bold;
    font-size: 1.1em;
}

.gps-invalid {
    background: #ffaa44;
    color: white;
    padding: 15px;
    border-radius: 25px;
    font-weight: bold;
    font-size: 1.1em;
}

@keyframes pulse {
    0% { transform: scale(1); }
    50% { transform: scale(1.05); }
    100% { transform: scale(1); }
}

.sensor-grid {
    display: grid;
    grid-template-columns: 1fr 1fr 1fr;
    gap: 20px;
    margin-bottom: 30px;
}

.sensor-card {
    background: #f8f9fa;
    padding: 20px;
    border-radius: 10px;
    border-left: 5px solid #667eea;
}

.sensor-card h2 {
    margin-top: 0;
    color: #555;
    font-size: 1.1em;
}

.data-row {
    display: flex;
    align-items: center;
    margin: 10px 0;
    gap: 10px;
}

.data-row span:first-child {
    font-weight: bold;
    width: 40px;
    color: #666;
}

.data-row span:nth-child(2) {
    width: 80px;
    font-family: monospace;
    background: #e9ecef;
    padding: 5px 10px;
    border-radius: 5px;
    text-align: center;
}

.progress-bar {
    flex: 1;
    background: #e9ecef;
    height: 20px;
    border-radius: 10px;
    overflow: hidden;
}

.progress-fill {
    height: 100%;
    transition: width 0.3s ease;
}

.progress-fill.low { background: #51cf66; }
.progress-fill.medium { background: #ffd43b; }
.progress-fill.high { background: #ff6b6b; }

.gps-controls {
    text-align: center;
    margin: 20px 0;
}

.gps-controls button {
    background: #667eea;
    color: white;
    border: none;
    padding: 12px 20px;
    margin: 5px;
    border-radius: 25px;
    cursor: pointer;
    font-size: 1em;
    transition: background 0.3s;
}

.gps-controls button:hover {
    background: #5a6fd8;
}

.share-status {
    margin: 10px 0;
    padding: 10px;
    border-radius: 5px;
    font-weight: bold;
    text-align: center;
}

.manual-input {
    margin-top: 20px;
    padding: 20px;
    background: #f8f9fa;
    border-radius: 10px;
}

.manual-input h3 {
    margin-top: 0;
    color: #555;
}

.input-group {
    display: flex;
    gap: 10px;
    flex-wrap: wrap;
    justify-content: center;
}

.input-group input {
    padding: 10px;
    border: 1px solid #ddd;
    border-radius: 5px;
    flex: 1;
    min-width: 150px;
}

.info {
    text-align: center;
    color: #666;
    font-size: 0.9em;
    border-top: 1px solid #ddd;
    padding-top: 20px;
}

@media (max-width: 768px) {
    .sensor-grid {
        grid-template-columns: 1fr;
    }
    
    .status-grid {
        grid-template-columns: 1fr;
    }
    
    .container {
        padding: 15px;
        margin: 10px;
    }
    
    .input-group {
        flex-direction: column;
    }
}
)rawliteral";

  server.send(200, "text/css", css);
}