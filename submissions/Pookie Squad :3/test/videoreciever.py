import requests, time

PHONE_IP = "192.168.1.120:8080"

while True:
    try:
        data = requests.get(f"http://{PHONE_IP}/sensors.json").json()
        print("Accel:", data["accel"]["x"], data["accel"]["y"], data["accel"]["z"])
        print("GPS:", data["gps"]["latitude"], data["gps"]["longitude"])
    except Exception as e:
        print("Error:", e)
    time.sleep(0.5)
