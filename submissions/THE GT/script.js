let socket;
let map, marker;
let prevLat = null, prevLon = null, prevTime = null;
let recordedData = []; // store all sensor data

// Initialize map
map = L.map('map').setView([12.9716, 77.5946], 15);
L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
  attribution: '&copy; OpenStreetMap contributors'
}).addTo(map);
marker = L.marker([12.9716, 77.5946]).addTo(map);

document.getElementById('connectBtn').addEventListener('click', () => {
  const espIp = prompt("Enter ESP32 IP (e.g. ws://192.168.4.1:81):");
  if (!espIp) return;

  socket = new WebSocket(espIp);
  socket.onopen = () => alert("✅ Connected to ESP32!");
  socket.onerror = () => alert("❌ Connection failed.");

  socket.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      updateSensorValues(data);
      recordData(data);
    } catch (e) {
      console.error("Error parsing data:", e);
    }
  };
});

// --- Update UI ---
function updateSensorValues(data) {
  document.getElementById("tilt").textContent = data.tilt + "°";
  document.getElementById("accel").textContent = data.accel + " m/s²";
  document.getElementById("brake").textContent = data.brake + " N";
  document.getElementById("ychange").textContent = data.ychange + " mm";
  document.getElementById("heart").textContent = data.heart + " bpm";

  if (data.lat && data.lon) updateMap(data.lat, data.lon);

  const prob = computeWalkingProbability(data);
  updateGauge(prob);
}

function updateMap(lat, lon) {
  marker.setLatLng([lat, lon]);
  map.setView([lat, lon], 16);
}

// --- Probability Logic ---
function computeWalkingProbability(data) {
  let accelFactor = Math.min(data.accel / 9.8, 1.0);
  let tiltFactor = Math.min(Math.abs(data.tilt) / 45.0, 1.0);
  const prob = ((accelFactor + tiltFactor) / 2) * 100;
  return Math.round(prob);
}

function updateGauge(prob) {
  const gauge = document.getElementById("gauge-bar");
  const text = document.getElementById("prob");
  text.textContent = prob + "%";
  gauge.setAttribute("stroke-dasharray", `${prob},100`);

  if (prob < 40) gauge.style.stroke = "#ff4b5c";
  else if (prob < 70) gauge.style.stroke = "#ffc107";
  else gauge.style.stroke = "#00ff99";
}

// --- Record + Download Data ---
function recordData(data) {
  const row = {
    time: new Date().toLocaleTimeString(),
    tilt: data.tilt,
    accel: data.accel,
    brake: data.brake,
    ychange: data.ychange,
    heart: data.heart,
    lat: data.lat,
    lon: data.lon,
    probability: computeWalkingProbability(data)
  };
  recordedData.push(row);
}

document.getElementById("downloadBtn").addEventListener("click", () => {
  if (recordedData.length === 0) {
    alert("No data recorded yet!");
    return;
  }

  const csvHeader = Object.keys(recordedData[0]).join(",") + "\n";
  const csvRows = recordedData.map(row =>
    Object.values(row).join(",")
  ).join("\n");

  const csvContent = csvHeader + csvRows;
  const blob = new Blob([csvContent], { type: "text/csv" });
  const url = URL.createObjectURL(blob);

  const a = document.createElement("a");
  a.href = url;
  a.download = "esp32_data.csv";
  a.click();
  URL.revokeObjectURL(url);
});
