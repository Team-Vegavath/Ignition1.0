const express = require("express");
const cors = require("cors");

const app = express();

app.use(cors());
app.use(express.json()); // For JSON body parsing
app.use(express.urlencoded({ extended: true })); // For form data parsing

// Store last GPS data
let lastGPS = {
  latitude: 0,
  longitude: 0,
  speed: 0, // km/h
  timestamp: Date.now()
};

// Store previous location for speed calculation
let previousLocation = null;
let previousTime = null;

// Calculate speed from GPS coordinates (Haversine formula)
function calculateSpeed(lat1, lon1, lat2, lon2, timeDiffSeconds) {
  if (!lat1 || !lon1 || !lat2 || !lon2 || timeDiffSeconds <= 0) {
    return 0;
  }

  const R = 6371e3; // Earth radius in meters
  const φ1 = (lat1 * Math.PI) / 180;
  const φ2 = (lat2 * Math.PI) / 180;
  const Δφ = ((lat2 - lat1) * Math.PI) / 180;
  const Δλ = ((lon2 - lon1) * Math.PI) / 180;

  const a =
    Math.sin(Δφ / 2) ** 2 +
    Math.cos(φ1) * Math.cos(φ2) * Math.sin(Δλ / 2) ** 2;
  const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));

  const distance = R * c; // Distance in meters
  const speedMps = distance / timeDiffSeconds; // meters per second
  const speedKmh = (speedMps * 3600) / 1000; // convert to km/h

  return Math.max(0, Math.min(200, speedKmh)); // Clamp between 0-200 km/h
}

// MIT Inventor app will POST GPS data here
app.post("/updategps", (req, res) => {
  try {
    // MIT App sends: { lat, lon, timestamp }
    // Handle both formats: { lat, lon, timestamp } or { latitude, longitude }
    const lat = parseFloat(req.body.lat || req.body.latitude);
    const lon = parseFloat(req.body.lon || req.body.longitude);
    const timestamp = req.body.timestamp ? parseInt(req.body.timestamp) : Date.now();
    const speed = parseFloat(req.body.speed || req.body.speedKmh || 0);

    if (isNaN(lat) || isNaN(lon)) {
      console.log("⚠️ Invalid GPS data received:", req.body);
      res.status(400).send("Invalid GPS data - lat and lon required");
      return;
    }

    const now = timestamp || Date.now();
    let calculatedSpeed = speed;

    // Calculate speed from GPS coordinates if not provided
    if (previousLocation && previousTime) {
      const timeDiff = (now - previousTime) / 1000; // seconds
      calculatedSpeed = calculateSpeed(
        previousLocation.latitude,
        previousLocation.longitude,
        lat,
        lon,
        timeDiff
      );
    }

    // Update GPS data (store timestamp from MIT App)
    lastGPS = {
      latitude: lat,
      longitude: lon,
      speed: calculatedSpeed > 0 ? calculatedSpeed : speed, // Use calculated speed if available
      timestamp: timestamp // Use timestamp from MIT App
    };

    // Update previous location for next speed calculation
    previousLocation = { latitude: lat, longitude: lon };
    previousTime = now;

    console.log(`📍 GPS Updated: ${lat.toFixed(6)}, ${lon.toFixed(6)} | Speed: ${lastGPS.speed.toFixed(1)} km/h`);
    
    res.send("OK");
  } catch (error) {
    console.error("❌ Error processing GPS update:", error);
    res.status(500).send("Error processing GPS data");
  }
});

// App will GET GPS data from here
// Returns format: { latitude: number, longitude: number, speed: number }
// Also supports MIT App format: { lat: number, lon: number, timestamp: number }
app.get("/gps", (req, res) => {
  // Return in format expected by React Native app
  res.json({
    latitude: lastGPS.latitude,
    longitude: lastGPS.longitude,
    speed: lastGPS.speed,
    // Also include MIT App format for compatibility
    lat: lastGPS.latitude,
    lon: lastGPS.longitude,
    timestamp: lastGPS.timestamp
  });
});

// Health check endpoint
app.get("/", (req, res) => {
  res.json({
    status: "GPS Server Running",
    lastUpdate: new Date(lastGPS.timestamp).toISOString(),
    location: {
      latitude: lastGPS.latitude,
      longitude: lastGPS.longitude,
      speed: lastGPS.speed
    }
  });
});

const PORT = 3000;
app.listen(PORT, () => {
  console.log(`✅ GPS Server running on port ${PORT}`);
  console.log(`📡 MIT Inventor app should POST to: http://YOUR_PC_IP:${PORT}/updategps`);
  console.log(`📱 React Native app will GET from: http://YOUR_PC_IP:${PORT}/gps`);
  console.log(`\nWaiting for GPS data from MIT Inventor app...\n`);
});

