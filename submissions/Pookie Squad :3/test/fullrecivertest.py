from flask import Flask, request
import csv, time

app = Flask(__name__)
filename = "helmet_log.csv"

with open(filename, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow([
        "timestamp", "lat", "lon", "speed", "alt",
        "ax", "ay", "az",
        "g_alpha", "g_beta", "g_gamma",
        "o_alpha", "o_beta", "o_gamma",
        "light"
    ])

@app.route("/data", methods=["POST"])
def data():
    d = request.json
    with open(filename, "a", newline="") as f:
        writer = csv.writer(f)
        writer.writerow([
            d["timestamp"],
            d["gps"]["lat"], d["gps"]["lon"], d["gps"]["speed"], d["gps"]["alt"],
            d["accel"]["x"], d["accel"]["y"], d["accel"]["z"],
            d["gyro"]["alpha"], d["gyro"]["beta"], d["gyro"]["gamma"],
            d["orient"]["alpha"], d["orient"]["beta"], d["orient"]["gamma"],
            d["light"]
        ])
    return "OK"

app.run(host="0.0.0.0", port=5000)
