import requests
import time
import random
import json

# 🧠 Configuration
SERVER_IP = "100.113.216.47"      # Laptop/Tailscale IP
PORT = 5000                       # Backend port
INTERVAL = 0.3                    # Time between packets
TIMEOUT = 60                      # Total runtime (seconds)

# --------------------------------------
# Generate one packet of simulated data
# --------------------------------------
def generate_data():
    return {
        "helmet": {
            "latitude": 12.9716 + random.uniform(-0.0003, 0.0003),
            "longitude": 77.5946 + random.uniform(-0.0003, 0.0003),
            "accel": {
                "x": random.uniform(-0.2, 0.2),
                "y": random.uniform(-0.2, 0.2),
                "z": random.uniform(9.6, 10.0)
            },
            "gyro": {
                "x": random.uniform(-2, 2),
                "y": random.uniform(-2, 2),
                "z": random.uniform(-2, 2)
            },
            "compass": {
                "x": random.uniform(15, 25),
                "y": random.uniform(10, 20),
                "z": random.uniform(5, 10)
            },
            "temperature": random.uniform(28, 36),
            "humidity": random.uniform(35, 55),
            "light": random.randint(400, 800),
            "barometer": random.randint(1005, 1015)
        },
        "jacket": {
            "accel": {
                "x": random.uniform(-0.3, 0.3),
                "y": random.uniform(-0.3, 0.3),
                "z": random.uniform(9.6, 10.0)
            },
            "gyro": {
                "x": random.uniform(-2, 2),
                "y": random.uniform(-2, 2),
                "z": random.uniform(-2, 2)
            },
            "flex": {
                "left": random.uniform(0.4, 0.6),
                "right": random.uniform(0.4, 0.6),
                "back": random.uniform(0.4, 0.6)
            },
            "impact": {
                "left": random.uniform(0.1, 0.4),
                "right": random.uniform(0.1, 0.4)
            }
        }
    }

# --------------------------------------
# Main loop
# --------------------------------------
if _name_ == "_main_":
    print("====================================")
    print(" 🏍  Pookie’s Riding Club - RPi Sender")
    print("====================================")
    ride_code = input("👉 Enter Ride Code (from Rider Screen): ").strip().upper()

    if not ride_code:
        print("❌ No code entered. Exiting.")
        exit()

    url = f"http://{SERVER_IP}:{PORT}/update/{ride_code}"
    print(f"✅ Sending telemetry to: {url}")
    print(f"⏱  Duration: {TIMEOUT}s | Interval: {INTERVAL}s\n")

    start_time = time.time()
    sent = 0

    while True:
        if time.time() - start_time >= TIMEOUT:
            break

        data = generate_data()
        try:
            r = requests.post(url, json=data, timeout=2)
            if r.status_code == 200:
                sent += 1
                print(f"📤 Packet {sent} sent", end="\r")
            else:
                print(f"⚠ Server responded with {r.status_code}")
        except requests.exceptions.RequestException:
            print("❌ Server unreachable, retrying...")

        time.sleep(INTERVAL)

    print(f"\n🏁 Finished! Sent {sent} packets successfully.")
