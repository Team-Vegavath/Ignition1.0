Rider Telemetry System

Hackathon Project – Rider Telemetry Challenge

Overview
This project implements a wearable rider telemetry system that captures real-time motion and location data from a helmet-mounted ESP32 + MPU6050 setup and displays the live data on a Streamlit-based dashboard.

The system automatically detects the rider’s mode of travel (walking, scooter, or motorcycle) based on posture and motion patterns, and visualizes live sensor readings, posture, and GPS speed.

All data transmission is wireless, achieved via Bluetooth (ESP32 → Phone) and MQTT (Phone → Dashboard), allowing the prototype to run independently on the test track.

---

System Architecture

Hardware Components

| Component                                    | Function                                                               |
| -------------------------------------------- | ---------------------------------------------------------------------- |
| ESP32.                                       | Central microcontroller for data collection and Bluetooth transmission |
| MPU6050 (Accelerometer + Gyroscope)          | Measures motion, lean, and posture angles                              |
| Battery (6V)                                 | Powers the ESP32 and MPU6050 module                                    |
| Smartphone GPS                               | Provides real-time location and speed data                             |

---

Software Components

`rider_telemetry.ino` – ESP32 Firmware

This Arduino sketch runs on the ESP32 and performs the following:

Sensor Data Acquisition

* Reads 3-axis acceleration and gyroscope values from the MPU6050 via I²C.
* Computes:

  * Motion Magnitude (|a|) for detecting activity.
  * Pitch Angle to estimate rider posture.
  * Forward Acceleration/Deceleration to detect braking or acceleration events.

Data Processing & Classification

Applies smoothing filters and threshold-based logic to identify:

* Accelerating / Braking / Decelerating states
* Posture Type — upright (scooter) vs bent-forward (bike)
* Riding Mode — Walking, Scooter, or Bike based on speed + posture + vibration levels

Wireless Data Transmission

* Sends processed sensor data via **Bluetooth Serial** as a live telemetry stream.
* Accepts optional GPS data through serial input in the format `GPS:23.4` (km/h).
* Output format example:

  ```
  Mag(g):1.03 Var:0.0234 | Ax(g):0.11 Ay:0.02 Az:0.98 | Pitch:23.5 | FwdG:0.08 | Mode:Scooter | Posture:Upright | Accel:0 Brk:0 Decel:0 | GPS(km/h):28.3
  ```

---

`app.py` – Streamlit Rider Dashboard

A real-time dashboard built using Streamlit that visualizes live data from both ESP32 and GPS sources.

Data Sources

1. ESP32 (via WebSocket or Bluetooth relay)

   * Receives telemetry data such as acceleration, pitch, mode, temperature, humidity, and crash detection flags.
2. GPS (via MQTT – HiveMQ Broker)

   * Subscribed to the topic `owntracks/#`, receiving live GPS position, timestamp, and speed from a phone app like *OwnTracks*.

Interface Highlights

* Dynamic Navbar & Themed UI: Minimal, gradient-based aesthetic optimized for live display.
* Connection Status Indicators: Real-time status lights for ESP32 and MQTT connection health.
* Data Cards:

  * GPS Latitude, Longitude, Speed, Timestamp
  * Acceleration (g-force), Pitch Angle, Riding Mode
  * Temperature, Humidity, System Status (Crash / Normal)
  * Crash Detection Alerts: Visually blinking red cards for emergency detection.
  * Live Update Loop (1s interval)**: Continuously refreshes all parameters in real time.

Example UI Layout

```
RiderTech Dashboard
├── Status Bar: [ESP32 ✅] [MQTT ✅]
├── GPS Section:
│   ├── Latitude | Longitude | Speed | Last Update
├── Vehicle Telemetry:
│   ├── Acceleration | Pitch | Mode
│   ├── Temperature | Humidity | Crash Status
```

---

Functional Flow

1. ESP32 Setup

   * Reads MPU6050 sensor data.
   * Calculates motion parameters.
   * Sends formatted data via Bluetooth or WebSocket.

2. Smartphone / MQTT Bridge

   * Transmits GPS updates through MQTT (OwnTracks app).
   * Optional: Forwards ESP32 data if connected via Bluetooth.

3. Streamlit Dashboard

   * Subscribes to MQTT for GPS.
   * Connects to ESP32 WebSocket for live sensor stream.
   * Displays all data in a unified UI with mode and posture detection.

---

Key Features Summary

| Feature                              | Description                                                                         |
| ------------------------------------ | ----------------------------------------------------------------------------------- |
| Motion Detection                     | Uses 3-axis accelerometer data to detect activity and transitions                   |
| Acceleration / Braking Detection     | Identifies acceleration spikes and braking events based on forward g-force          |
| Posture Recognition                  | Estimates pitch angle to classify between bike (bent) and scooter (upright) posture |
| Riding Mode Classification           | Determines if the rider is walking, on scooter, or on motorcycle                    |
| Real-time Dashboard                  | Displays live data cards for telemetry and GPS                                      |
| Crash Detection (Extendable)         | Displays alert if a crash or abnormal motion is detected                            |
| Bluetooth Telemetry                  | Wireless transmission of sensor data                                                |
| MQTT GPS Integration                 | Uses open broker (HiveMQ) for easy real-time GPS data stream                        |

---

Bill of Materials (BoM)

| Component                  | Quantity | Approx. Cost (INR) | Purpose                           |
| -------------------------- | -------- | ------------------ | --------------------------------- |
| ESP32 Dev Board            | 1        | ₹200               | Main processing and communication |
| MPU6050 Sensor             | 1        | ₹120               | Acceleration and gyro data        |
| 6V Battery Pack            | 1        | ₹50                | Portable power supply             |
| Connecting Wires           | -        | ₹15                | Circuit connections               |
| Smartphone (OwnTracks app) | 1        | –                  | GPS + MQTT transmission           |
| Total                      |          | ≈ ₹385             |                                   |

---

Setup & Usage

Hardware Setup

1. Connect MPU6050 → ESP32:

   ```
   VCC → 3.3V (or VIN)
   GND → GND
   SDA → GPIO21
   SCL → GPIO22
   ```
2. Power the ESP32 using a 6V battery via VIN and GND.
3. Mount the ESP32 + MPU6050 module on the rider’s helmet or jacket.

Firmware Setup

1. Open Arduino IDE → Load `rider_telemetry.ino`
2. Select **ESP32 Dev Module** as board.
3. Upload the code via USB.
4. Pair via Bluetooth: Device name **ESP32_Telemetry**

Dashboard Setup

1. Install requirements:

   ```bash
   pip install streamlit paho-mqtt websocket-client
   ```
2. Run the dashboard:

   ```bash
   streamlit run app.py
   ```
3. Open browser → [http://localhost:8501](http://localhost:8501)
4. Observe real-time updates from sensors and GPS.

---

Test Procedure (Hackathon Flow)

| Step         | Expected System Behavior                        |
| ------------ | ----------------------------------------------- |
| Start        | Idle mode (Walking)                             |
| Walk         | Detects low-speed movement → “Walking”          |
| Ride Scooter | Detects upright posture + vibration → “Scooter” |
| Ride Bike    | Detects bent posture + vibration → “Bike”       |
| Brake / Stop | Shows deceleration events                       |
| End          | Mode returns to “Walking”                       |

---

Innovations Explored & Demonstrated (in previous reviews)
* Crash Detection (via MPU6050)
* EV Battery Efficiency Calculator (via ambient temperature using DHT22)

---

Demonstration Setup

* ESP32 + MPU6050 mounted on helmet
* Bluetooth connection to smartphone for GPS input
* Streamlit app running on laptop for real-time monitoring
* Tested using the sequence: Walk → Ride → Stop → Walk

---

Outcome

This project successfully demonstrates a **fully wireless, live rider telemetry system** that:

* Captures motion, orientation, and GPS data in real-time
* Differentiates between walking, scooter, and bike states
* Provides a professional live dashboard
* Is lightweight and affordable

---
