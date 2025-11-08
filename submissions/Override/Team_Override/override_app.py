import streamlit as st
import json
import time
import websocket
import threading
from datetime import datetime
from paho.mqtt import client as mqtt


# -------------------- MQTT CONFIG --------------------
BROKER = "broker.hivemq.com"
PORT = 1883
TOPIC = "owntracks/#"


latest_gps = {"timestamp": "--", "lat": None, "lon": None, "speed_kmh": None}
esp32_data = {"TS": "--", "A": 0.0, "P": 0.0, "Fwd": 0.0, "Mode": "--", "Tmp": 0.0, "Hum": 0.0, "Crash": 0}
connection_status = {"esp32": False, "mqtt": False}


# -------------------- MQTT CALLBACKS --------------------
def on_connect(client, userdata, flags, reason_code, properties=None):
    print(f"[MQTT] Connected: {reason_code}")
    connection_status["mqtt"] = True
    client.subscribe(TOPIC)


def on_message(client, userdata, msg):
    global latest_gps
    try:
        data = json.loads(msg.payload.decode(errors="ignore"))
        lat, lon, vel = data.get("lat"), data.get("lon"), data.get("vel", 0)
        tst = data.get("tst") or time.time()
        if lat and lon:
            latest_gps = {
                "timestamp": datetime.utcfromtimestamp(int(tst)).isoformat() + "Z",
                "lat": round(lat, 6),
                "lon": round(lon, 6),
                "speed_kmh": round(float(vel), 2)
            }
    except Exception as e:
        print("[MQTT ERROR]", e)


def mqtt_thread():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(BROKER, PORT, 60)
    client.loop_forever()


# -------------------- ESP32 WEBSOCKET HANDLER --------------------
def esp32_listener():
    global esp32_data
    try:
        ws = websocket.WebSocket()
        ws.connect("ws://192.168.4.1:81/")
        connection_status["esp32"] = True
        print("[ESP32] Connected via WebSocket")

        while True:
            msg = ws.recv()
            if msg and ":" in msg:
                parts = msg.split(",")
                data_dict = {}
                for part in parts:
                    kv = part.split(":")
                    if len(kv) == 2:
                        data_dict[kv[0]] = kv[1]
                esp32_data.update(data_dict)
    except Exception as e:
        print("[ESP32 ERROR]", e)
        connection_status["esp32"] = False


# -------------------- START THREADS --------------------
threading.Thread(target=mqtt_thread, daemon=True).start()
threading.Thread(target=esp32_listener, daemon=True).start()


# -------------------- STREAMLIT UI --------------------
st.set_page_config(page_title="Rider Telemetry Dashboard", layout="wide")

st.markdown(
    """
    <style>
    /* Remove default Streamlit container spacing at top to bring navbar up */
    .block-container { padding-top: 0rem; padding-bottom: 0rem; }

    body, .stApp {
        background: linear-gradient(135deg, #2b1055 0%, #7597de 100%);
        color: #fff;
        font-family: 'Inter', 'Segoe UI', sans-serif;
        margin: 0;
        padding: 0;
    }

    #MainMenu, footer, header {visibility: hidden;}

    /* Navigation Bar pinned near top */
    .navbar {
        background: rgba(255, 255, 255, 0.08);
        backdrop-filter: blur(10px);
        padding: 10px 20px;
        border-radius: 0px;
        margin: 0;
        margin-top: 4px; /* slight offset from very top */
        display: flex;
        justify-content: space-between;
        align-items: center;
        border: 1px solid rgba(255, 255, 255, 0.15);
        border-top: none;
    }

    .navbar-brand {
        font-size: 20px;
        font-weight: 700;
        color: #fff;
        letter-spacing: 1px;
    }

    .navbar-links {
        display: flex;
        gap: 20px;
        align-items: center;
    }

    .nav-link {
        position: relative;
        color: rgba(255, 255, 255, 0.9);
        text-decoration: none;
        font-weight: 500;
        padding: 6px 14px;
        border-radius: 6px;
        transition: all 0.2s ease;
        cursor: pointer;
    }

    .nav-link::after {
        content: "";
        position: absolute;
        left: 50%;
        bottom: 0;
        width: 0%;
        height: 2px;
        background: #76a8ff;
        transition: all 0.3s ease;
        transform: translateX(-50%);
        border-radius: 2px;
    }

    .nav-link:hover::after {
        width: 60%;
    }

    /* Header Section tighter to navbar */
    .header-section {
    text-align: center;
    padding: 4rem 0;      /* increased top/bottom padding */
    margin: 0.75rem 0;    /* keep balanced outer spacing */
    }


    /* Larger title and subtitle inside the header */
    .header-section .main-title {
    font-size: 44px;     /* was 36px; increase as needed */
    line-height: 1.1;    /* keeps tight vertical rhythm */
    }

    .header-section .subtitle {
    font-size: 16px;     /* was 14px; subtle bump */
    line-height: 1.35;   /* improves readability */
    }

    .main-title {
        font-size: 36px;
        font-weight: 800;
        color: #ffffff;
        text-shadow: 1px 1px 6px rgba(0,0,0,0.3);
        margin-bottom: 3px;
    }

    .subtitle {
        font-size: 14px;
        color: rgba(255, 255, 255, 0.85);
        font-weight: 400;
    }

    /* Status Bar */
    .status-bar {
        background: rgba(255, 255, 255, 0.1);
        backdrop-filter: blur(10px);
        padding: 10px 20px;
        border-radius: 10px;
        margin: 16px 0;
        display: flex;
        justify-content: center;
        gap: 30px;
        font-size: 15px;
        border: 1px solid rgba(255, 255, 255, 0.15);
    }

    .status-item {
        display: flex;
        align-items: center;
        gap: 8px;
        font-weight: 500;
    }

    .status-indicator {
        width: 10px;
        height: 10px;
        border-radius: 50%;
    }

    /* Section Headers */
    .section-header {
        font-size: 18px;
        font-weight: 700;
        color: #fff;
        margin: 18px 0 12px 0;
        padding-left: 8px;
        border-left: 3px solid rgba(255, 255, 255, 0.5);
        text-transform: uppercase;
        letter-spacing: 0.5px;
    }

    /* Card Styles */
    .card {
        background: rgba(255, 255, 255, 0.95);
        padding: 25px 18px;
        border-radius: 12px;
        text-align: center;
        box-shadow: 0 8px 20px rgba(0, 0, 0, 0.15);
        border: 1px solid rgba(255, 255, 255, 0.2);
        height: 160px;
        display: flex;
        flex-direction: column;
        justify-content: center;
        align-items: center;
    }

    .label {
        font-size: 13px;
        font-weight: 600;
        color: #777;
        margin-bottom: 6px;
        text-transform: uppercase;
    }

    .value {
        font-size: 30px;
        font-weight: 800;
        color: #2b1055;
        line-height: 1.1;
    }

    .unit {
        font-size: 12px;
        font-weight: 600;
        color: #888;
        margin-top: 2px;
    }

    /* Crash Card */
    .crash-card {
        padding: 20px;
        border-radius: 12px;
        text-align: center;
        font-weight: 600;
    }

    .crash-alert {
        animation: blink 1s infinite;
    }

    @keyframes blink {
        0%, 50%, 100% { opacity: 1; }
        25%, 75% { opacity: 0.6; }
    }
    </style>
    """,
    unsafe_allow_html=True,
)

# Navigation Bar
st.markdown("""
<div class='navbar'>
    <div class='navbar-brand'>RiderTech</div>
    <div class='navbar-links'>
        <div class='nav-link'>Dashboard</div>
        <div class='nav-link'>Analytics</div>
        <div class='nav-link'>Ride History</div>
        <div class='nav-link'>Settings</div>
    </div>
</div>
""", unsafe_allow_html=True)

# Header Section
st.markdown("""
<div class='header-section'>
    <div class='main-title'>Rider Telemetry Dashboard</div>
    <div class='subtitle'>Real-time Monitoring and Performance Analytics</div>
</div>
""", unsafe_allow_html=True)

# Connection Status Bar
esp32_status_color = "#4caf50" if connection_status['esp32'] else "#f44336"
mqtt_status_color = "#4caf50" if connection_status['mqtt'] else "#f44336"

st.markdown(f"""
<div class='status-bar'>
    <div class='status-item'>
        <div class='status-indicator' style='background-color: {esp32_status_color};'></div>
        <span>ESP32: {'Connected' if connection_status['esp32'] else 'Disconnected'}</span>
    </div>
    <div class='status-item'>
        <div class='status-indicator' style='background-color: {mqtt_status_color};'></div>
        <span>MQTT: {'Connected' if connection_status['mqtt'] else 'Disconnected'}</span>
    </div>
</div>
""", unsafe_allow_html=True)

# GPS Section
st.markdown("<div class='section-header'>GPS Location Data</div>", unsafe_allow_html=True)
gps_col1, gps_col2, gps_col3, gps_col4 = st.columns(4)

gps_lat = gps_col1.empty()
gps_lon = gps_col2.empty()
gps_speed = gps_col3.empty()
gps_time = gps_col4.empty()

# ESP32 Section
st.markdown("<div class='section-header'>Vehicle Telemetry</div>", unsafe_allow_html=True)
esp_row1 = st.columns(3)
esp_row2 = st.columns(3)

esp_accel = esp_row1[0].empty()
esp_pitch = esp_row1[1].empty()
esp_mode = esp_row1[2].empty()
esp_temp = esp_row2[0].empty()
esp_hum = esp_row2[1].empty()
esp_crash = esp_row2[2].empty()

# -------------------- MAIN LOOP --------------------
while True:
    gps_lat.markdown(f"<div class='card'><div class='label'>Latitude</div><div class='value'>{latest_gps['lat'] or '--'}</div></div>", unsafe_allow_html=True)
    gps_lon.markdown(f"<div class='card'><div class='label'>Longitude</div><div class='value'>{latest_gps['lon'] or '--'}</div></div>", unsafe_allow_html=True)
    gps_speed.markdown(f"<div class='card'><div class='label'>Speed</div><div class='value'>{latest_gps['speed_kmh'] or '--'}</div><div class='unit'>km/h</div></div>", unsafe_allow_html=True)
    gps_time.markdown(f"<div class='card'><div class='label'>Last Update</div><div class='value' style='font-size: 18px;'>{latest_gps['timestamp']}</div></div>", unsafe_allow_html=True)

    esp_accel.markdown(f"<div class='card'><div class='label'>Acceleration</div><div class='value'>{esp32_data['A']}</div><div class='unit'>g-force</div></div>", unsafe_allow_html=True)
    esp_pitch.markdown(f"<div class='card'><div class='label'>Pitch Angle</div><div class='value'>{esp32_data['P']}</div><div class='unit'>degrees</div></div>", unsafe_allow_html=True)
    esp_mode.markdown(f"<div class='card'><div class='label'>Riding Mode</div><div class='value' style='font-size: 24px;'>{esp32_data['Mode']}</div></div>", unsafe_allow_html=True)
    esp_temp.markdown(f"<div class='card'><div class='label'>Temperature</div><div class='value'>{esp32_data['Tmp']}</div><div class='unit'>°C</div></div>", unsafe_allow_html=True)
    esp_hum.markdown(f"<div class='card'><div class='label'>Humidity</div><div class='value'>{esp32_data['Hum']}</div><div class='unit'>%</div></div>", unsafe_allow_html=True)

    is_crash = esp32_data["Crash"] == "1"
    crash_bg = "linear-gradient(135deg, #f44336 0%, #d32f2f 100%)" if is_crash else "linear-gradient(135deg, #4caf50 0%, #388e3c 100%)"
    crash_text = "CRASH DETECTED" if is_crash else "All Systems Normal"
    crash_class = "crash-alert" if is_crash else ""
    
    esp_crash.markdown(f"""
    <div class='crash-card {crash_class}' style='background: {crash_bg}; color: white;'>
        <div class='label' style='color: rgba(255,255,255,0.9);'>System Status</div>
        <div class='value' style='color: white; font-size: 22px;'>{crash_text}</div>
    </div>
    """, unsafe_allow_html=True)

    time.sleep(1)
