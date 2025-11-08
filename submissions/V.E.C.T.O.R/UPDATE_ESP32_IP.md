# Quick ESP32 IP Update

## To Connect Your ESP32:

1. **Open**: `src/hooks/useTelemetry.js`

2. **Find this line** (around line 6):
   ```javascript
   const SENSOR_API = 'http://YOUR_ESP32_IP_HERE/data';
   ```

3. **Replace** `YOUR_ESP32_IP_HERE` with your ESP32 IP address

   Example:
   ```javascript
   const SENSOR_API = 'http://192.168.1.50/data';
   ```

4. **Save the file** - Expo will auto-reload

5. **Check the dashboard** - You should see real sensor data!

## Expected ESP32 API Response Format:

Your ESP32 should return JSON like this:
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

## If ESP32 Not Connected:

- App will automatically use **mock data** (so judges can still see the demo)
- Status will show "Using Mock Data" in red
- All metrics will still display with simulated values

## Testing:

1. Make sure ESP32 and phone are on same WiFi network
2. Test ESP32 API in browser: `http://YOUR_ESP32_IP/data`
3. If browser shows JSON, app will work!

