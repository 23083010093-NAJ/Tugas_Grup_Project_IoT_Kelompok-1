from flask import Flask, render_template, request
from flask_socketio import SocketIO
import numpy as np
import cv2
from ultralytics import YOLO
import threading
import csv
import os
from datetime import datetime

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*")

model = YOLO("yolov8n.pt")

# ================= SHARED STATE =================
sensor_data = {"distance": 0, "status": "WAITING"}
camera_data = {"objects": []}
data_lock   = threading.Lock()

CSV_FILE    = "dataset.csv"
CSV_INTERVAL = 10  # detik

# ================= CSV SETUP =================
def init_csv():
    if not os.path.exists(CSV_FILE):
        with open(CSV_FILE, "w", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(["timestamp", "distance", "status", "objects"])
        print(f"✅ CSV dibuat: {CSV_FILE}")

def csv_loop():
    import time
    while True:
        time.sleep(CSV_INTERVAL)
        with data_lock:
            distance = sensor_data.get("distance", 0)
            status   = sensor_data.get("status", "WAITING")
            objects  = ", ".join(camera_data.get("objects", [])) or "NONE"

        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        with open(CSV_FILE, "a", newline="") as f:
            writer = csv.writer(f)
            writer.writerow([timestamp, distance, status, objects])

        print(f"💾 CSV disimpan: {timestamp} | {distance}cm | {status} | {objects}")

# ================= KAMERA LAPTOP =================
def laptop_camera_loop():
    cap = cv2.VideoCapture(0)

    if not cap.isOpened():
        print("❌ Kamera laptop tidak ditemukan")
        return

    print("✅ Kamera laptop aktif")

    while True:
        ret, frame = cap.read()
        if not ret:
            continue

        results  = model(frame, verbose=False)
        detected = []

        for r in results:
            for b in r.boxes:
                detected.append(model.names[int(b.cls[0])])

        with data_lock:
            camera_data["objects"] = detected

        socketio.emit("camera", {"objects": detected})

# ================= SENSOR =================
@app.route("/sensor", methods=["POST"])
def sensor():
    with data_lock:
        sensor_data.update(request.json)
    socketio.emit("sensor", request.json)
    return {"ok": True}

# ================= DASHBOARD =================
@app.route("/")
def index():
    return render_template("dashboard.html")

# ================= MAIN =================
if __name__ == "__main__":
    init_csv()
    threading.Thread(target=laptop_camera_loop, daemon=True).start()
    threading.Thread(target=csv_loop,           daemon=True).start()
    socketio.run(app, host="0.0.0.0", port=5000, debug=False)