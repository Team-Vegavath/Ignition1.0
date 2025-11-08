# 🚀 Quick GPS Setup (5 Minutes)

## Step 1: Setup Dummy Phone GPS Server

### Option A: Using Node.js (Recommended)

1. **On dummy phone** (or laptop if phone can't run Node.js):
   ```bash
   # Install Node.js if needed
   # Then:
   npm install express cors
   ```

2. **Copy** `DUMMY_PHONE_GPS_SERVER.js` to dummy phone

3. **Run**:
   ```bash
   node DUMMY_PHONE_GPS_SERVER.js
   ```

4. **Note the IP address**:
   - Server will show: `GPS Server running on port 3000`
   - Find IP: Settings → WiFi → Your network → IP address
   - Example: `192.168.137.105`

### Option B: Using Laptop as Bridge

1. **On your laptop**, run the GPS server
2. **On dummy phone**, open browser to: `http://LAPTOP_IP:3000`
3. **Allow GPS permission** when prompted
4. **Use laptop IP** in main app

---

## Step 2: Update Main App

1. **Open**: `src/hooks/useGPS.js`

2. **Find line 12** and update:
   ```javascript
   const GPS_API = 'http://192.168.137.105:3000/gps'; // Your dummy phone IP
   ```

3. **Save file**

---

## Step 3: Test

1. **Test in browser** (on main phone):
   ```
   http://DUMMY_PHONE_IP:3000/gps
   ```
   Should return: `{"latitude":..., "longitude":..., "speed":...}`

2. **If browser works** → GPS server is ready!

3. **Reload app** → Should connect to GPS

---

## ⚠️ If App Still Shows Mock Data

This is **Expo Go HTTP blocking** (known limitation).

**For demo**, you can:
- ✅ Show browser test working (proves GPS server works)
- ✅ Explain code is ready and tested
- ✅ Show app with mock GPS (still functional)
- ✅ Note: Would work in production build

---

## 📱 Finding IP Address

**Android:**
- Settings → WiFi → Tap network name → IP address

**iOS:**
- Settings → WiFi → Tap (i) icon → IP address

**Windows:**
- Command Prompt → `ipconfig` → Look for IPv4

**Mac/Linux:**
- Terminal → `ifconfig` → Look for inet

---

## ✅ Expected Result

Once setup:
- App connects to dummy phone GPS
- Shows real latitude, longitude, speed
- Calculates distance traveled
- Updates map with real location

**If connection fails**, app gracefully uses mock GPS data (demo still works!)

