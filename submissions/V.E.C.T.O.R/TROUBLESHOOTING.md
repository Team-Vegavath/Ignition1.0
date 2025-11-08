# ESP32 Connection Troubleshooting

## ✅ Updated Connection Logic

The app now:
1. **Tries multiple endpoints** automatically:
   - `http://10.230.127.235/data` (primary)
   - `http://10.230.127.235/` (root)
   - `http://10.230.127.235/api/data`
   - `http://10.230.127.235/sensor`

2. **Handles different data formats**:
   - `{ acceleration: {x,y,z}, gyro: {x,y,z}, battery: number }`
   - `{ accX, accY, accZ, gyroX, gyroY, gyroZ, battery }`
   - `{ acc: {x,y,z}, gyro: {x,y,z}, battery }`

3. **Better error logging** - Check console for detailed errors

## 🔍 Debugging Steps

### 1. Check Console Logs
Open React Native debugger or check Metro bundler console. You should see:
- `Trying ESP32 endpoint: http://10.230.127.235/data`
- `✅ ESP32 connected! Data received:` (if successful)
- `❌ Failed to connect to...` (if failed)

### 2. Test ESP32 API Directly
Open browser on your phone and try:
```
http://10.230.127.235/data
```

If this works in browser but not in app:
- Network permission issue
- CORS issue (shouldn't affect React Native)

### 3. Verify Network Connection
- ✅ Phone and ESP32 on **same WiFi network**
- ✅ ESP32 IP is `10.230.127.235`
- ✅ ESP32 server is running
- ✅ No firewall blocking port

### 4. Check ESP32 Response Format
Your ESP32 should return JSON. Test with:
```bash
curl http://10.230.127.235/data
```

Expected formats (any of these work):
```json
// Format 1
{
  "acceleration": {"x": 0.1, "y": 0.2, "z": 9.8},
  "gyro": {"x": 0.01, "y": 0.02, "z": 0.03},
  "battery": 85
}

// Format 2
{
  "accX": 0.1, "accY": 0.2, "accZ": 9.8,
  "gyroX": 0.01, "gyroY": 0.02, "gyroZ": 0.03,
  "battery": 85
}
```

### 5. Common Issues

**Issue**: "Request timeout"
- ESP32 not responding
- Wrong IP address
- Network connectivity issue

**Issue**: "HTTP 404"
- Wrong endpoint path
- ESP32 server not configured for `/data`

**Issue**: "Network request failed"
- Phone and ESP32 on different networks
- ESP32 not accessible from phone

## 🚀 Quick Fixes

1. **Restart ESP32** - Power cycle the device
2. **Check WiFi** - Ensure both devices on same network
3. **Verify IP** - Ping `10.230.127.235` from phone
4. **Test in Browser** - If browser works, app should work
5. **Check ESP32 Code** - Ensure server is running on port 80

## 📱 What You'll See

- **Green dot + "✅ ESP32 Connected"** = Success!
- **Orange dot + "⚠️ Using Mock Data"** = Connection failed
- **Error details** shown below status (if connection fails)

The app will keep trying every 1 second, so if ESP32 comes online, it will connect automatically!

