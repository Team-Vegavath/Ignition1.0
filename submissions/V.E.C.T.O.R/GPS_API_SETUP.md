# GPS API Setup Guide

## Architecture Overview

The app uses **two separate data sources**:

1. **Dummy Phone** → Provides GPS data (latitude, longitude, speed)
2. **ESP32** → Provides IMU sensor data (acceleration, gyro, battery)

## Dummy Phone GPS API Requirements

Your dummy phone needs to expose a GPS API endpoint that returns JSON data every second.

### Expected API Response Format:

```json
{
  "latitude": 12.9716,
  "longitude": 77.5946,
  "speed": 45.3
}
```

### API Endpoint:
- **URL**: `http://YOUR_DUMMY_PHONE_IP:PORT/gps`
- **Method**: GET
- **Response**: JSON with `latitude`, `longitude`, `speed` (km/h)
- **Update Rate**: 1 second intervals

### Configuration:

Update the `GPS_API` constant in `src/hooks/useGPS.js`:

```javascript
const GPS_API = 'http://192.168.1.100:3000/gps'; // Change to your dummy phone's IP
```

## ESP32 Sensor API Requirements

ESP32 should provide IMU data (NO GPS, NO speed):

### Expected ESP32 API Response Format:

```json
{
  "acceleration": {
    "x": 0.12,
    "y": 0.45,
    "z": 9.81
  },
  "gyro": {
    "x": 0.05,
    "y": 0.01,
    "z": 0.02
  },
  "battery": 82
}
```

### API Endpoint:
- **URL**: `http://192.168.4.1/data` (default ESP32 AP mode)
- **Method**: GET
- **Response**: JSON with `acceleration`, `gyro`, `battery`

## Data Flow:

1. **Main Phone App** polls dummy phone GPS API (1/second)
2. **Main Phone App** polls ESP32 sensor API (1/second)
3. **Main Phone App** merges both data sources:
   - GPS speed + IMU posture → Vehicle mode detection
   - GPS coordinates → Map display
   - GPS coordinates → Distance calculation
   - IMU data → Posture angle calculation

## Testing Without Hardware:

The app automatically falls back to **mock data** when:
- Dummy phone GPS API is unavailable
- ESP32 sensor is not connected
- Network connection fails

## Example Dummy Phone GPS API (Node.js/Express):

```javascript
const express = require('express');
const app = express();
const Geolocation = require('@react-native-community/geolocation'); // or expo-location

app.get('/gps', (req, res) => {
  // Get GPS from dummy phone
  Geolocation.getCurrentPosition(
    (position) => {
      res.json({
        latitude: position.coords.latitude,
        longitude: position.coords.longitude,
        speed: (position.coords.speed || 0) * 3.6, // Convert m/s to km/h
      });
    },
    (error) => {
      res.status(500).json({ error: 'GPS error' });
    }
  );
});

app.listen(3000, '0.0.0.0', () => {
  console.log('GPS API running on port 3000');
});
```

## Network Setup:

1. Connect dummy phone and main phone to same WiFi network
2. Find dummy phone's local IP address (e.g., `192.168.1.100`)
3. Update `GPS_API` in `useGPS.js` with dummy phone's IP
4. Ensure dummy phone's GPS API is accessible from main phone
5. ESP32 should be in AP mode (`192.168.4.1`) or on same WiFi network

