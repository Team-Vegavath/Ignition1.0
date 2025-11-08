from flask import Flask, request
import csv, time

app = Flask(__name__)
logfile = open("helmet_log.csv", "a", newline="")
writer = csv.writer(logfile)
writer.writerow(["timestamp", "lat", "lon", "speed",
                 "ax", "ay", "az", "gx", "gy", "gz",
                 "compass", "light", "temp"])

@app.route("/data", methods=["POST"])
def data():
    d = request.json
    writer.writerow([
        d["timestamp"],
        d["gps"]["lat"], d["gps"]["lon"], d["gps"]["speed"],
        d["accel"]["x"], d["accel"]["y"], d["accel"]["z"],
        d["rotation"]["alpha"], d["rotation"]["beta"], d["rotation"]["gamma"],
        d["compass"], d["light"], d["temp"]
    ])
    logfile.flush()
    return "OK"

app.run(host="0.0.0.0", port=5000)
