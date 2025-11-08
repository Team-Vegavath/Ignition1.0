# Rider Telemetry System - Mobile App

React Native (Expo) mobile app for real-time rider telemetry tracking.

## Phase 1 Features (Current)

✅ GPS tracking with distance calculation  
✅ Sensor data integration (ESP32 WiFi)  
✅ Real-time dashboard with metrics  
✅ Live map with route tracking  
✅ Start/Stop ride functionality  
✅ Vehicle mode detection (Walking/Scooter/Motorcycle)  
✅ Mock data fallback for testing  

## Setup

1. Install dependencies:
```bash
npm install
```

2. Start the app:
```bash
npm start
```

3. Scan QR code with Expo Go app (iOS/Android)

## Configuration

- **Sensor API**: Edit `SENSOR_API` in `src/hooks/useTelemetry.js`
- Default: `http://192.168.4.1/data`

## Phase 2 (Coming Soon)

- SQLite database storage
- Data logs screen with pagination
- Ride summary screen
- CSV/JSON export
- Auto-export at 1000 records

