// Simple GPS Server for Dummy Phone
// Run this on your dummy phone using Node.js

const express = require('express');
const app = express();
const cors = require('cors');

app.use(cors());
app.use(express.json());

// Store latest GPS data
let latestGPSData = {
  latitude: 0,
  longitude: 0,
  speed: 0
};

// GPS endpoint - returns latest GPS data
app.get('/gps', (req, res) => {
  res.json(latestGPSData);
});

// Update GPS data endpoint (called by client-side script)
app.post('/update-gps', (req, res) => {
  latestGPSData = {
    latitude: req.body.latitude || 0,
    longitude: req.body.longitude || 0,
    speed: req.body.speed || 0
  };
  res.json({ success: true, data: latestGPSData });
});

// Serve HTML page for GPS tracking
app.get('/', (req, res) => {
  res.send(`
<!DOCTYPE html>
<html>
<head>
    <title>GPS Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: Arial; padding: 20px; background: #111; color: #fff; }
        .status { padding: 15px; margin: 10px 0; border-radius: 8px; background: #1f2937; }
        .connected { background: #065f46; }
        .data { font-family: monospace; background: #000; padding: 10px; border-radius: 4px; }
    </style>
</head>
<body>
    <h1>📡 GPS Server Running</h1>
    <div id="status" class="status">Getting GPS...</div>
    <div id="data" class="data">Waiting for location...</div>
    <div style="margin-top: 20px;">
        <strong>API Endpoint:</strong> http://YOUR_IP:3000/gps
    </div>
    <script>
        let watchId = navigator.geolocation.watchPosition(
            (pos) => {
                const data = {
                    latitude: pos.coords.latitude,
                    longitude: pos.coords.longitude,
                    speed: (pos.coords.speed || 0) * 3.6
                };
                document.getElementById('status').textContent = '✅ GPS Active';
                document.getElementById('status').className = 'status connected';
                document.getElementById('data').textContent = JSON.stringify(data, null, 2);
                
                // Send to server
                fetch('/update-gps', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(data)
                });
            },
            (err) => {
                document.getElementById('status').textContent = '❌ GPS Error: ' + err.message;
            },
            { enableHighAccuracy: true, timeout: 5000, maximumAge: 0 }
        );
    </script>
</body>
</html>
  `);
});

const PORT = 3000;
app.listen(PORT, '0.0.0.0', () => {
  console.log(`\n✅ GPS Server running on port ${PORT}`);
  console.log(`📱 Open browser: http://localhost:${PORT}`);
  console.log(`🌐 API endpoint: http://YOUR_IP:${PORT}/gps\n`);
});

