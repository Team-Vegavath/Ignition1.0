# ESP32 JSON Data Format

## Current Expected Format

The app currently supports multiple JSON formats. You can provide your ESP32 data in any format, and I'll adapt the code to parse it.

## Current Supported Formats

### Format 1 (Flat structure - YOUR CURRENT FORMAT):
```json
{
  "accelX": -9.70,
  "accelY": -8.30,
  "accelZ": -16.30,
  "gyroX": 25.80,
  "gyroY": 17.10,
  "gyroZ": -44.40,
  "batteryPercent": 26
}
```

### Format 2 (Nested structure):
```json
{
  "acceleration": {
    "x": -9.70,
    "y": -8.30,
    "z": -16.30
  },
  "gyro": {
    "x": 25.80,
    "y": 17.10,
    "z": -44.40
  },
  "battery": 26
}
```

### Format 3 (Alternative names):
```json
{
  "accX": -9.70,
  "accY": -8.30,
  "accZ": -16.30,
  "gyroX": 25.80,
  "gyroY": 17.10,
  "gyroZ": -44.40,
  "bat": 26
}
```

## How to Provide Your Format

**Just send me a sample JSON from your ESP32, and I'll update the code to match it exactly!**

For example:
```json
{
  "your_key_name_1": value1,
  "your_key_name_2": value2,
  ...
}
```

## Posture Angle Calculation

**IMPORTANT:** The app now uses:
- **90° = Standing straight (vertical)**
- **0° = Lying flat (horizontal)**

The posture angle is calculated from accelerometer data using:
```
angle = arccos(accZ / magnitude) * (180 / π)
```

Where:
- `magnitude = sqrt(accX² + accY² + accZ²)`
- Result is in degrees (0-90 range)

## Vehicle Detection Logic

- **Walking**: speed < 5 km/h OR posture > 75°
- **Scooter**: speed 5-30 km/h AND posture 45-75°
- **Motorcycle**: speed > 30 km/h AND posture 30-60°

