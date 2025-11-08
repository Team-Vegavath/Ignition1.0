# 🚴 Telemetry System: Node-RED Activity Tracker

A proof-of-concept repository utilizing **Node-RED** to visualize, process, and display real-time sensor data—specifically accelerometer readings—from a mobile device (likely an Android phone) on a custom web dashboard. This system is designed for basic motion analysis, step counting, and activity recognition.

## ✨ Features

* **Real-time Acceleration Visualization:** Displays X, Y, and Z axis acceleration data on a live graph.
* **Activity Recognition:** Attempts to identify the "Current Activity" (e.g., Standing, Walking, Running) based on acceleration patterns.
* **Step Counting:** Calculates and displays the "No of steps" detected from the sensor data.
* **Geographic Tracking:** Includes a map display, suggesting integration of location (GPS) data alongside motion data.
* **Interactive Dashboard:** Provides a custom UI (Node-RED Dashboard) for data monitoring accessible via a web browser (`127.0.0.1:1880/ui`).

## 🛠️ Prerequisites

Before installing the flow, you must have the following software and Node-RED nodes installed:

1.  **Node.js & npm:** Required to run Node-RED.
2.  **Node-RED:** The core application.
3.  **Required Node-RED Modules:**
    * `node-red-dashboard`: For creating the web UI.
    * `node-red-node-web-sockets` or `node-red-contrib-mqtt`: (Likely) Used for receiving data from the mobile device.
    * `node-red-contrib-ui-digital-gauge` (or `node-red-dashboard`'s standard `ui-gauge`): If you added the optional speedometer.

    You can install these via the Node-RED Palette Manager or using npm:
    ```bash
    npm install node-red-dashboard node-red-node-web-sockets node-red-contrib-ui-digital-gauge
    ```

4.  **Data Source Application:** An application on the mobile device (e.g., **Termux** with `termux-api`, **RedMobile**, or a custom app) configured to send sensor data (accelerometer, GPS) to your Node-RED instance via the configured input node (e.g., WebSocket, MQTT, or HTTP POST). The debug output suggests the data originates from an `androidsensor.accelerometer` source.

## 🚀 Installation and Setup

### 1. Import the Flow

1.  Start your Node-RED instance.
2.  Copy the JSON data of the flow (`flow.json` - *assuming this file exists in the repository*).
3.  In the Node-RED editor, click the **hamburger menu (☰)** > **Import** > **Clipboard**.
4.  Paste the flow JSON and click **Import**.

### 2. Configure Input Node

The flow uses an input node (e.g., `websocket in`, `mqtt in`, or `http in`) to receive data.

1.  **Double-click** the main input node (the one that connects to the rest of the flow) to configure its connection details (e.g., MQTT topic, WebSocket path, or HTTP endpoint).
2.  Ensure your mobile application is sending data to this exact endpoint.

### 3. Deploy

Click the **Deploy** button in the top right corner of the editor.

## 💻 Usage

### Accessing the Dashboard

1.  Open your web browser.
2.  Navigate to the dashboard URL, typically: `http://<your-node-red-ip>:1880/ui` (The image shows `127.0.0.1:1880/ui/#/0/flow937...`).

### Interacting with the Flow

* The system will immediately start displaying data once the mobile device begins sending telemetry messages.
* The **function nodes** in the flow are responsible for complex calculations (step detection and activity classification) and may require tuning based on the specific sensor data format.

---

This video demonstrates how to set up Node-RED to receive and process sensor data directly from an Android phone, which is highly relevant to your telemetry system. [Node-Red on Android Blew my Mind](https://www.youtube.com/watch?v=Xoe5Hf9Janw)


http://googleusercontent.com/youtube_content/2
