import requests, time, json, numpy as np
from scipy import fftpack

PHONE_IP = "100.106.222.64:8080"  # change to your phone's IP

SAMPLE_RATE = 10  # Hz
WINDOW_SIZE = 3   # seconds
BUFFER_LEN = SAMPLE_RATE * WINDOW_SIZE

accel_buf = []
gyro_buf = []
speed_buf = []

def magnitude(v):
    return np.sqrt(sum(x**2 for x in v))

def classify(accel_std, gyro_std, speed, dom_freq):
    if speed < 0.5:
        if accel_std < 0.3:
            return "standing"
        else:
            return "walking"
    elif speed < 4:
        if 0.8 < dom_freq < 2.5:
            return "walking"
        else:
            return "bike"
    else:
        if accel_std < 0.4 and gyro_std < 10:
            return "scooter"
        else:
            return "bike"

while True:
    try:
        sensors = requests.get(f"http://{PHONE_IP}/sensors.json", timeout=5).json()
        gps = requests.get(f"http://{PHONE_IP}/gps.json", timeout=5).json()

        ax = sensors.get("accel", {}).get("x", 0)
        ay = sensors.get("accel", {}).get("y", 0)
        az = sensors.get("accel", {}).get("z", 0)
        gx = sensors.get("gyro", {}).get("x", 0)
        gy = sensors.get("gyro", {}).get("y", 0)
        gz = sensors.get("gyro", {}).get("z", 0)
        light = sensors.get("light", {}).get("light", 0)
        temp = sensors.get("temp", {}).get("temperature", 0)
        compass = sensors.get("compass", {}).get("azimuth", 0)
        speed = gps.get("speed", 0) or 0
        if speed > 10:  # convert km/h to m/s if needed
            speed /= 3.6

        a_mag = magnitude([ax, ay, az])
        g_mag = magnitude([gx, gy, gz])

        accel_buf.append(a_mag)
        gyro_buf.append(g_mag)
        speed_buf.append(speed)

        if len(accel_buf) > BUFFER_LEN:
            accel_buf = accel_buf[-BUFFER_LEN:]
            gyro_buf = gyro_buf[-BUFFER_LEN:]
            speed_buf = speed_buf[-BUFFER_LEN:]

            a_std = np.std(accel_buf)
            g_std = np.std(gyro_buf)
            spd_mean = np.mean(speed_buf)

            # dominant frequency of accel
            a_centered = np.array(accel_buf) - np.mean(accel_buf)
            freqs = fftpack.fftfreq(len(a_centered), 1 / SAMPLE_RATE)
            fftm = np.abs(fftpack.fft(a_centered))
            pos = freqs > 0
            dom_freq = freqs[pos][np.argmax(fftm[pos])]

            state = classify(a_std, g_std, spd_mean, dom_freq)

            # 📦 package into JSON
            data = {
                "timestamp": time.time(),
                "accel": {"x": ax, "y": ay, "z": az, "magnitude": a_mag},
                "gyro": {"x": gx, "y": gy, "z": gz, "magnitude": g_mag},
                "light": light,
                "temperature": temp,
                "compass": compass,
                "gps": gps,
                "stats": {
                    "accel_std": a_std,
                    "gyro_std": g_std,
                    "speed_mean": spd_mean,
                    "dominant_frequency": dom_freq
                },
                "activity": state
            }

            print(json.dumps(data, indent=2))  # print full JSON

            # 👉 optionally send to your server:
            # requests.post("http://yourserver:5000/upload", json=data)

        time.sleep(1 / SAMPLE_RATE)

    except Exception as e:
        print("Error:", e)
        time.sleep(1)
