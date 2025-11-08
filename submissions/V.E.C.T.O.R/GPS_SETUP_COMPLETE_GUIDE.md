# Complete GPS Setup Guide for Dummy Phone

## 🎯 Quick Setup (Choose One Method)

### Method 1: Simple Node.js Server (Recommended for Hackathon)

#### Step 1: On Your Dummy Phone

1. **Install Node.js** (if not installed):
   - Download from: https://nodejs.org/
   - Or use Termux app on Android

2. **Create the server file**:
   - Copy `DUMMY_PHONE_GPS_SERVER.js` to your dummy phone
   - Or create a new file with that code

3. **Install dependencies**:
   ```bash
   npm install express cors
   ```

4. **Run the server**:
   ```bash
   node DUMMY_PHONE_GPS_SERVER.js
   ```

5. **Find your dummy phone's IP**:
   - Android: Settings → WiFi → Tap connected network → See IP address
   - iOS: Settings → WiFi → Tap (i) icon → See IP address
   - Example: `192.168.1.105`

6. **Test the API**:
   - Open browser on dummy phone: `http://localhost:3000`
   - Should show GPS data updating
   - Test API: `http://YOUR_IP:3000/gps` (from main phone browser)

#### Step 2: Update Main App

1. **Open**: `src/hooks/useGPS.js`

2. **Update the IP address** (line 6):
   ```javascript
   const GPS_API = 'http://192.168.1.105:3000/gps'; // Change to your dummy phone's IP
   ```

3. **Save and reload app**

---

### Method 2: Use Expo Go on Dummy Phone (Alternative)

If you have Expo Go on dummy phone, you can create a simple GPS sharing app:

1. Create a simple React Native app that:
   - Gets GPS using `expo-location`
   - Serves it via HTTP using `expo-server-sdk` or similar
   - Exposes endpoint at `/gps`

2. Run on dummy phone and share IP

---

### Method 3: Use Your Laptop as Bridge (If Phone Can't Run Server)

1. **On your laptop**, create a simple server:

```javascript
// laptop-gps-bridge.js
const express = require('express');
const app = express();
const cors = require('cors');

app.use(cors());

// This will receive GPS from dummy phone
let gpsData = { latitude: 0, longitude: 0, speed: 0 };

app.post('/phone-gps', (req, res) => {
  gpsData = req.body;
  res.json({ success: true });
});

app.get('/gps', (req, res) => {
  res.json(gpsData);
});

app.listen(3000, '0.0.0.0', () => {
  console.log('GPS Bridge running on http://YOUR_LAPTOP_IP:3000/gps');
});
```

2. **On dummy phone**, create a simple HTML page that:
   - Gets GPS using browser geolocation
   - Sends to laptop: `http://YOUR_LAPTOP_IP:3000/phone-gps`

3. **Update main app** to use laptop IP:
   ```javascript
   const GPS_API = 'http://YOUR_LAPTOP_IP:3000/gps';
   ```

---

## 📍 Where to Add GPS API in Code

**File**: `src/hooks/useGPS.js`

**Line 6**: Update the `GPS_API` constant:

```javascript
const GPS_API = 'http://192.168.1.105:3000/gps'; // Your dummy phone's IP
```

---

## 🔧 Network Setup

### Requirements:
1. ✅ Dummy phone and main phone on **same WiFi network**
2. ✅ Dummy phone GPS server running
3. ✅ Firewall allows port 3000 (or your chosen port)
4. ✅ Dummy phone's IP address is known

### Finding IP Address:

**Android:**
- Settings → WiFi → Tap connected network → IP address shown

**iOS:**
- Settings → WiFi → Tap (i) icon → IP address shown

**Windows/Mac:**
- Open terminal/command prompt
- Type: `ipconfig` (Windows) or `ifconfig` (Mac/Linux)
- Look for IPv4 address

---

## 🧪 Testing

1. **Test on dummy phone browser**:
   ```
   http://localhost:3000/gps
   ```
   Should return JSON with latitude, longitude, speed

2. **Test from main phone browser**:
   ```
   http://DUMMY_PHONE_IP:3000/gps
   ```
   Should return same JSON

3. **If browser works but app doesn't**:
   - This is Expo Go HTTP blocking issue
   - App will use mock GPS data (still works for demo)

---

## 🚀 Quick Start (Fastest for Hackathon)

1. **On dummy phone**, install Termux (Android) or use laptop
2. **Run**: `node DUMMY_PHONE_GPS_SERVER.js`
3. **Note the IP address** shown
4. **Update** `src/hooks/useGPS.js` with that IP
5. **Reload app** - GPS should work!

---

## 💡 For Judges Demo

If GPS API setup is complex, you can:
- ✅ Show the code is ready (GPS hook implemented)
- ✅ Show mock GPS data working
- ✅ Explain: "GPS would come from dummy phone API at [IP]"
- ✅ Show browser test working (proves concept)

The app gracefully falls back to mock data, so demo still works!

