#include <MPU6050_tockn.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

MPU6050 mpu6050(Wire);
WebServer server(80);

const char* ssid = "RiderTelemetry";
const char* password = "12345678";

float accelX=0, accelY=0, accelZ=0, gyroX=0, gyroY=0, gyroZ=0;
float angleX=0, angleY=0, angleZ=0, accelMag=0, decel=0, prevAccelMag=0;
unsigned long prevTime=0;
float gpsLat=0, gpsLon=0, gpsSpeed=0;
String vehicleType="Unknown";

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\n=== ESP32 STARTING ===");
  
  Wire.begin();
  mpu6050.begin();
  Serial.println("MPU6050 Initialized!");
  mpu6050.calcGyroOffsets(true);
  Serial.println("Calibration complete!");
  
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("WiFi Started! IP: ");
  Serial.println(IP);
  
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/gps", handleGPS);
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("Web Server Started!");
}

void loop() {
  server.handleClient();
  
  mpu6050.update();
  unsigned long t=millis();
  
  accelX=mpu6050.getAccX(); 
  accelY=mpu6050.getAccY(); 
  accelZ=mpu6050.getAccZ();
  gyroX=mpu6050.getGyroX(); 
  gyroY=mpu6050.getGyroY(); 
  gyroZ=mpu6050.getGyroZ();
  angleX=mpu6050.getAngleX(); 
  angleY=mpu6050.getAngleY(); 
  angleZ=mpu6050.getAngleZ();
  
  accelMag=sqrt(accelX*accelX+accelY*accelY+accelZ*accelZ);
  
  if(prevTime>0){
    float deltaTime = (t-prevTime)/1000.0;
    if(deltaTime > 0) {
      decel=(prevAccelMag-accelMag)/deltaTime;
    }
  }
  prevAccelMag=accelMag; 
  prevTime=t;
  
  if(gpsSpeed<5){
    vehicleType="Walking";
  }else if(gpsSpeed<30){
    if(abs(angleX)<15){
      vehicleType="Scooter";
    }else{
      vehicleType="Motorcycle";
    }
  }else{
    vehicleType="Motorcycle";
  }
  
  delay(50);
}

void handleGPS(){
  Serial.println("=== GPS Request Received ===");
  
  if(server.hasArg("lat")){
    String latStr = server.arg("lat");
    gpsLat = latStr.toFloat();
    Serial.print("Lat: "); Serial.println(gpsLat);
  }
  
  if(server.hasArg("lon")){
    String lonStr = server.arg("lon");
    gpsLon = lonStr.toFloat();
    Serial.print("Lon: "); Serial.println(gpsLon);
  }
  
  if(server.hasArg("speed")){
    String speedStr = server.arg("speed");
    gpsSpeed = speedStr.toFloat();
    Serial.print("Speed: "); Serial.println(gpsSpeed);
  }
  
  server.send(200, "text/plain", "GPS OK");
  Serial.println("Response sent!");
}

void handleNotFound(){
  Serial.print("Unknown request: ");
  Serial.println(server.uri());
  server.send(404, "text/plain", "Not Found");
}

void handleRoot(){
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'><title>Rider Telemetry</title>";
  html += "<style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:Arial;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#fff;padding:15px}";
  html += ".c{max-width:600px;margin:0 auto}h1{text-align:center;margin-bottom:20px;font-size:24px}.card{background:rgba(255,255,255,0.15);border-radius:15px;padding:15px;margin-bottom:15px}";
  html += ".d{display:flex;justify-content:space-between;padding:10px 0;border-bottom:1px solid rgba(255,255,255,0.2)}.d:last-child{border-bottom:none}.l{font-weight:600}.v{font-weight:700;font-size:18px}";
  html += ".badge{display:inline-block;padding:10px 25px;background:rgba(255,255,255,0.3);border-radius:25px;font-weight:700;font-size:22px}";
  html += "button{width:48%;padding:12px;border:none;border-radius:10px;font-size:16px;font-weight:600;color:#fff;cursor:pointer}";
  html += "#r{background:#4CAF50}#r.rec{background:#f44336}#dl{background:#2196F3}h2{margin-bottom:10px;font-size:18px}</style></head>";
  html += "<body><div class='c'><h1>RIDER TELEMETRY</h1>";
  html += "<div class='card'><h2>Vehicle</h2><div style='text-align:center'><div class='badge' id='vt'>Unknown</div></div></div>";
  html += "<div class='card'><h2>GPS</h2><div class='d'><span class='l'>Lat:</span><span class='v' id='lat'>--</span></div>";
  html += "<div class='d'><span class='l'>Lon:</span><span class='v' id='lon'>--</span></div><div class='d'><span class='l'>Speed:</span><span class='v' id='spd'>--</span></div></div>";
  html += "<div class='card'><h2>Acceleration</h2><div class='d'><span class='l'>X:</span><span class='v' id='ax'>--</span></div>";
  html += "<div class='d'><span class='l'>Y:</span><span class='v' id='ay'>--</span></div><div class='d'><span class='l'>Z:</span><span class='v' id='az'>--</span></div>";
  html += "<div class='d'><span class='l'>Mag:</span><span class='v' id='am'>--</span></div><div class='d'><span class='l'>Decel:</span><span class='v' id='dc'>--</span></div></div>";
  html += "<div class='card'><h2>Orientation</h2><div class='d'><span class='l'>Pitch:</span><span class='v' id='px'>--</span></div>";
  html += "<div class='d'><span class='l'>Roll:</span><span class='v' id='py'>--</span></div><div class='d'><span class='l'>Yaw:</span><span class='v' id='pz'>--</span></div></div>";
  html += "<div style='display:flex;gap:10px'><button id='r' onclick='toggleRec()'>Start</button><button id='dl' onclick='downloadCSV()'>Download</button></div></div>";
  html += "<script>let recording=0,dataLog=[];function toggleRec(){recording=!recording;document.getElementById('r').textContent=recording?'Stop':'Start';";
  html += "document.getElementById('r').classList.toggle('rec');if(recording){dataLog=[];dataLog.push('Time,Lat,Lon,Speed,AX,AY,AZ,GX,GY,GZ,AngleX,AngleY,AngleZ,Mag,Decel,Vehicle')}}";
  html += "function downloadCSV(){if(dataLog.length==0){alert('No data');return}const b=new Blob([dataLog.join('\\n')],{type:'text/csv'});";
  html += "const u=URL.createObjectURL(b);const a=document.createElement('a');a.href=u;a.download='data_'+Date.now()+'.csv';a.click()}";
  html += "function updateData(){fetch('/data').then(r=>r.json()).then(d=>{";
  html += "document.getElementById('lat').textContent=d.lat.toFixed(6);document.getElementById('lon').textContent=d.lon.toFixed(6);";
  html += "document.getElementById('spd').textContent=d.speed.toFixed(1)+' km/h';document.getElementById('ax').textContent=d.ax.toFixed(2);";
  html += "document.getElementById('ay').textContent=d.ay.toFixed(2);document.getElementById('az').textContent=d.az.toFixed(2);";
  html += "document.getElementById('am').textContent=d.accelMag.toFixed(2);document.getElementById('dc').textContent=d.decel.toFixed(2);";
  html += "document.getElementById('px').textContent=d.angleX.toFixed(1);document.getElementById('py').textContent=d.angleY.toFixed(1);";
  html += "document.getElementById('pz').textContent=d.angleZ.toFixed(1);document.getElementById('vt').textContent=d.vehicle;";
  html += "if(recording)dataLog.push(new Date().toISOString()+','+d.lat+','+d.lon+','+d.speed+','+d.ax+','+d.ay+','+d.az+','+d.gx+','+d.gy+','+d.gz+','+d.angleX+','+d.angleY+','+d.angleZ+','+d.accelMag+','+d.decel+','+d.vehicle)";
  html += "}).catch(e=>console.error(e))}setInterval(updateData,100)</script></body></html>";
  server.send(200, "text/html", html);
}

void handleData(){
  StaticJsonDocument<512> doc;
  doc["lat"]=gpsLat; 
  doc["lon"]=gpsLon; 
  doc["speed"]=gpsSpeed;
  doc["ax"]=accelX; 
  doc["ay"]=accelY; 
  doc["az"]=accelZ;
  doc["gx"]=gyroX; 
  doc["gy"]=gyroY; 
  doc["gz"]=gyroZ;
  doc["angleX"]=angleX; 
  doc["angleY"]=angleY; 
  doc["angleZ"]=angleZ;
  doc["accelMag"]=accelMag; 
  doc["decel"]=decel; 
  doc["vehicle"]=vehicleType;
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}
