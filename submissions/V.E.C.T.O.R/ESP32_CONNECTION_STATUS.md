# ESP32 Connection Status

## ✅ IP Address Updated

**ESP32 IP**: `10.230.127.235`  
**API Endpoint**: `http://10.230.127.235/data`

## 📊 Stats Displayed on Dashboard

### Real-Time Metrics (Updated Every 1 Second):

1. **Speed** (km/h)
   - From GPS (dummy phone) or mock data
   - Displayed in large card

2. **Battery** (%)
   - From ESP32 sensor
   - Shows ESP32 battery level

3. **Vehicle Mode** (🚶 Walking / 🛵 Scooter / 🏍️ Motorcycle)
   - Auto-detected from speed + posture angle
   - Icon + text display

4. **Posture Angle** (°)
   - Calculated from accelerometer (X, Y, Z)
   - Formula: `atan2(accX, sqrt(accY² + accZ²)) * 180/π`

5. **Acceleration** (m/s²)
   - **Accel X** - Forward/backward
   - **Accel Y** - Left/right
   - **Accel Z** - Up/down (includes gravity ~9.8)

6. **Gyroscope** (rad/s)
   - **Gyro X** - Roll rate
   - **Gyro Y** - Pitch rate
   - **Gyro Z** - Yaw rate

7. **GPS Coordinates**
   - Latitude & Longitude
   - From dummy phone or mock data

### Charts:

- **Acceleration Trends** - Line chart showing X, Y, Z over time
- Updates every second with latest 20 data points

## 🔌 Connection Status

The app will show:
- **Green dot** = "Sensor Connected" (ESP32 connected)
- **Orange dot** = "Using Mock Data" (ESP32 not connected)

## 📡 Expected ESP32 API Response

Your ESP32 should return JSON at `http://10.230.127.235/data`:

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

## 🚀 What Happens Now

1. App automatically tries to connect to ESP32 every 1 second
2. If connected → Shows real sensor data
3. If not connected → Shows mock data (so judges can still see demo)
4. Dashboard updates in real-time
5. All metrics display immediately (no need to start ride)

## ✅ Ready for Demo!

The app is now configured and will:
- Display all stats immediately
- Try to connect to your ESP32
- Show connection status
- Update every second

**Reload the app** and you should see data flowing! 🎉

