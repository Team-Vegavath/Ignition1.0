#!/bin/bash

# Kill old video streams if any
sudo pkill mjpg_streamer

# Start mjpg_streamer in background
sudo mjpg_streamer -i "input_uvc.so -d /dev/video0 -r 1280x720 -f 30" \
  -o "output_http.so -p 8081 -w /usr/local/www -l 0.0.0.0" &

# Wait a bit for the camera to start
sleep 5

# Start the Python telemetry sender
python3 /home/pi/transmitter.py
