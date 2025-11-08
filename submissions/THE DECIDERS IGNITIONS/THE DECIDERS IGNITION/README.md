# IGNITION 1.0

**Rider Telemetry Challenge**

The Rider Telemetry System is a wearable solution that tracks a rider's real-time motion and location using an IMU and GPS, integrated with an interactive Flutter-based app. The app works seamlessly on both web and mobile platforms, displaying live data such as acceleration, speed, and position, while classifying the rider's activity as walking, scooter riding, or motorcycle riding. All data is stored locally with timestamps for analysis.

## FOLDER STRUCTURE - 
THE_DECIDERS_IGNITION/
├── README.md
├── results/
│   └── images of our results ...
├── rider_telemetry_code.txt (For Hardware Programming)
└── ride_telemetry_app.zip (For Software application)

## FEATURES -

- Acceleration (3-axis): Detect motion, braking, lean, and body orientation.
- Deceleration (derived from acceleration): Detect stops or harsh braking.
- Speed: From GPS or IMU integration; used to validate ride segments.
- GPS Position (latitude, longitude, timestamp): For track matching, ride path, and
scoring.
- Timestamped Data Storage (mandatory): All sensor data and GPS points must be
stored locally on the mobile app for evaluation.

## HARDWARE USED -

1 ESP8266
1 MPU6050(IMU sensor)
Jumper Wires
1 HW 9V battery

## SETUP AND INSTALLATION -

1. Install the `ESP8266` library and choose the board NodeMCU 1.0 (ESP-12E Module) on Arduino IDE.
2. For the flutter app setup,
 a. Download and install Flutter SDK from flutter.dev.
 Verify installation using: `flutter doctor`
 b. Extract the zip folder `ride_telemetry_app` to a clean folder to use it.
 c. Navigate to the folder and install dependencies: `flutter pub get`
 d. Use `flutter run`, to run the app.
    To run it as a mobile app, make sure the android dependencies are installed (Android SDK) else it will run on the system's web browser.
3. Connect to ESP8266.
The app reads real-time data from the ESP8266 (via Serial/Wi-Fi depending on setup).
Verify if live data is being displayed correctly (acceleration, speed, and GPS info).

