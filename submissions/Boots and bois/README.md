# Smart Rider+

[![React Native](https://img.shields.io/badge/React%20Native-v0.71-blue)](https://reactnative.dev/)
[![Expo](https://img.shields.io/badge/Expo-v49.0.0-green)](https://expo.dev/)

## Overview
Smart Rider+ is a **real-time activity tracking app** that uses **mobile sensors** (Accelerometer, Gyroscope, GPS) to detect and differentiate between **walking, biking, and scooter riding**.  

It also supports **phone mirroring** using **ApowerMirror**, allowing the app display to be viewed on another device for monitoring or demonstration purposes.

---

## Features
- **Real-time sensor tracking:** Accelerometer, Gyroscope, and GPS data collection.  
- **Activity recognition:** Automatically classifies walking, biking, or scooter riding.  
- **Interactive map display:** Shows real-time location and route traveled.  
- **Data visualization:** Speed, distance, and activity type shown in real-time.  
- **Phone mirroring support:** View the app on another device using **ApowerMirror**.  
- **Cross-platform:** Works on both Android and iOS devices via **Expo Go**.

---

## How It Works
1. **Sensors:**  
   - **Accelerometer:** Measures linear acceleration (x, y, z axes).  
   - **Gyroscope:** Measures angular velocity and rotation.  
   - **GPS:** Tracks location, speed, and distance.

2. **Activity Recognition:**  
   - Uses sensor data to classify movement into walking, biking, or scooter riding in real-time.

3. **Map Display:**  
   - Displays the user’s route and current location on a map interface.

4. **Screen Mirroring with ApowerMirror:**  
   - Mirror the rider's phone to another device in real-time.  
   - Ideal for demonstrations or monitoring without physically holding the phone.

---

## Requirements
- **Mobile device (Android or iOS)** with sensors and GPS.  
- **Expo Go app** installed to run Smart Rider+ on the phone.  
- **ApowerMirror** (optional) for screen mirroring.

---

## Installation
1. Clone the repository:  
   ```bash
   git clone https://github.com/your-username/smart-rider.git
   cd smart-rider
   

**Install dependencies:**
npm install


**Start the app using Expo:**
npx expo start


**Scan the QR code using Expo Go on your mobile device to open the app.**


**Using ApowerMirror for Display**
Install ApowerMirror on both devices (rider’s phone and viewing device).
Connect both devices to the same Wi-Fi network or use USB connection.
On the rider’s phone, select Phone Screen Mirroring and choose the target device.
The app display will appear on the mirrored device in real-time.


**What Smart Rider+ Displays**
Current activity type: Walking / Biking / Scooter
Speed: Real-time in km/h or mph
Distance traveled: Real-time
Route: Displayed on a map
Sensor data visualization: Optional (debug mode)


**Technologies Used**
React Native – Cross-platform mobile app framework
Expo Go – Run the app on mobile devices without building a standalone app
React Native Maps – Map visualization
ApowerMirror – Phone screen mirroring


**Future Enhancements**
Integrate machine learning for improved activity classification
Add historical activity tracking with charts and summaries
Implement notifications for milestones or goals
Support multiple users or profiles
