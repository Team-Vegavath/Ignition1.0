

import React, { useEffect, useState, useRef } from "react";
import L from "leaflet";
import {
  Activity,
  Map as MapIcon,
  Zap,
  AlertTriangle,
  Download,
  Phone,
  History,
  Settings,
  Trash2,
  Gauge,
  Compass,
} from "lucide-react";
import "leaflet/dist/leaflet.css";

const PI_URL = "http://10.240.213.80:5000/data"; // Flask endpoint

const Index = () => {
  const [telemetry, setTelemetry] = useState({});
  const [view, setView] = useState("dashboard");
  const [crashDetected, setCrashDetected] = useState(false);
  const [countdown, setCountdown] = useState(0);
  const [history, setHistory] = useState([]);
  const [rideSessions, setRideSessions] = useState([]);
  const [rideActive, setRideActive] = useState(false);
  const [rideStartTime, setRideStartTime] = useState(null);
  const [rideMode, setRideMode] = useState("Idle");
  const [posture, setPosture] = useState("Upright");
  const [emergency, setEmergency] = useState({
    name: "Mom",
    number: "+911234567890",
  });

  const countdownRef = useRef(null);
  const mapRef = useRef(null);
  const markerRef = useRef(null);
  const polylineRef = useRef(null);
  const distanceRef = useRef(0);
  const prevPoint = useRef(null);

  // Load saved rides + contact
  useEffect(() => {
    const saved = localStorage.getItem("ride_sessions");
    if (saved) setRideSessions(JSON.parse(saved));
    const savedContact = localStorage.getItem("emergency_contact");
    if (savedContact) setEmergency(JSON.parse(savedContact));
  }, []);

  // 🛰 Fetch live telemetry
  useEffect(() => {
    const interval = setInterval(() => {
      fetch(PI_URL)
        .then((res) => res.json())
        .then((data) => {
          if (data && data.ax !== undefined) {
            const gForce =
              Math.sqrt(data.ax ** 2 + data.ay ** 2 + data.az ** 2) / 9.8;
            const speed = data.gps_spd > 0.1 ? data.gps_spd : data.mpu_spd || 0;

            // Ride mode logic
            let mode = "Idle";
            if (speed >= 1 && speed < 6) mode = "Walking";
            else if (speed >= 6 && speed < 25) mode = "Scooter";
            else if (speed >= 25) mode = "Motorcycle";

            // Posture via tilt
            const tilt = data.tilt || 0;
            let pos = "Upright";
            if (tilt > 10) pos = "Leaning Forward";
            else if (tilt < -10) pos = "Leaning Backward";

            const newData = {
              ...data,
              gForce,
              spd: parseFloat(speed.toFixed(2)),
              tilt: parseFloat(tilt.toFixed(1)),
              mode,
              posture: pos,
              timestamp: Date.now(),
            };
            setTelemetry(newData);
            setRideMode(mode);
            setPosture(pos);

            if (rideActive) {
              setHistory((prev) => [...prev.slice(-2000), newData]);
              if (prevPoint.current && data.lat && data.lon) {
                const d = calcDistance(
                  prevPoint.current.lat,
                  prevPoint.current.lon,
                  data.lat,
                  data.lon
                );
                distanceRef.current += d;
              }
              prevPoint.current = { lat: data.lat, lon: data.lon };
            }

            if (gForce > 3.5) triggerCrashSequence(gForce);
          }
        })
        .catch((err) => console.warn("⚠ Cannot reach Pi:", err.message));
    }, 500);
    return () => clearInterval(interval);
  }, [rideActive]);

  // 🚨 Crash logic
  const triggerCrashSequence = (gForce) => {
    if (!crashDetected) {
      setCrashDetected(true);
      setCountdown(30);
      countdownRef.current = setInterval(() => {
        setCountdown((prev) => {
          if (prev <= 1) {
            clearInterval(countdownRef.current);
            autoCallEmergency();
            return 0;
          }
          return prev - 1;
        });
      }, 1000);
    }
  };

  const cancelEmergency = () => {
    clearInterval(countdownRef.current);
    setCrashDetected(false);
    setCountdown(0);
  };

  const autoCallEmergency = () => {
    window.location.href = tel:${emergency.number};
  };

  // 🗺 Map
  useEffect(() => {
    if (view === "map") {
      setTimeout(() => {
        const mapContainer = document.getElementById("map");
        if (!mapContainer) return;

        if (!mapRef.current) {
          mapRef.current = L.map(mapContainer).setView([12.9716, 77.5946], 15);
          L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
            attribution: "© OpenStreetMap contributors",
          }).addTo(mapRef.current);
          polylineRef.current = L.polyline([], { color: "cyan" }).addTo(
            mapRef.current
          );
        }

        if (telemetry.lat && telemetry.lon) {
          const pos = [telemetry.lat, telemetry.lon];
          if (!markerRef.current) {
            markerRef.current = L.marker(pos)
              .addTo(mapRef.current)
              .bindPopup("📍 You are here")
              .openPopup();
          } else {
            markerRef.current.setLatLng(pos);
          }
          polylineRef.current.addLatLng(pos);
          mapRef.current.setView(pos, mapRef.current.getZoom());
        }
      }, 100);
    }
  }, [view, telemetry]);

  // 🎯 Ride tracking
  const startRide = () => {
    setRideActive(true);
    setHistory([]);
    setRideStartTime(Date.now());
    distanceRef.current = 0;
    prevPoint.current = null;
  };

  const stopRide = () => {
    setRideActive(false);
    const endTime = Date.now();
    const durationMin = ((endTime - rideStartTime) / 60000).toFixed(1);
    const distance = distanceRef.current.toFixed(2);
    const avgSpeed = distance / (durationMin / 60);
    const rideData = {
      startTime: new Date(rideStartTime).toLocaleString(),
      duration: durationMin,
      distance,
      avgSpeed: avgSpeed.toFixed(1),
      mode: rideMode,
      posture,
      data: history,
    };
    const updated = [...rideSessions, rideData];
    setRideSessions(updated);
    localStorage.setItem("ride_sessions", JSON.stringify(updated));
  };

  // 📥 Download
  const downloadJSON = (ride) => {
    const blob = new Blob([JSON.stringify(ride, null, 2)], {
      type: "application/json",
    });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = ride_${ride.startTime.replace(/[^0-9]/g, "")}.json;
    a.click();
  };

  const downloadCSV = (ride) => {
    if (ride.data.length === 0) return;
    const headers = Object.keys(ride.data[0]).join(",");
    const rows = ride.data.map((obj) => Object.values(obj).join(","));
    const csv = [headers, ...rows].join("\n");
    const blob = new Blob([csv], { type: "text/csv" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = ride_${ride.startTime.replace(/[^0-9]/g, "")}.csv;
    a.click();
  };

  const clearHistory = () => {
    localStorage.removeItem("ride_sessions");
    setRideSessions([]);
  };

  const calcDistance = (lat1, lon1, lat2, lon2) => {
    if (!lat1 || !lon1 || !lat2 || !lon2) return 0;
    const R = 6371;
    const dLat = ((lat2 - lat1) * Math.PI) / 180;
    const dLon = ((lon2 - lon1) * Math.PI) / 180;
    const a =
      Math.sin(dLat / 2) ** 2 +
      Math.cos(lat1 * (Math.PI / 180)) *
        Math.cos(lat2 * (Math.PI / 180)) *
        Math.sin(dLon / 2) ** 2;
    return R * (2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a)));
  };

  const saveEmergency = () => {
    localStorage.setItem("emergency_contact", JSON.stringify(emergency));
    alert("✅ Emergency contact saved!");
  };

  return (
    <div className="min-h-screen bg-gray-900 text-white">
      {/* Header */}
      <div className="bg-gradient-to-r from-blue-600 to-purple-600 p-4 flex justify-between items-center">
        <h1 className="text-xl font-bold">RideAssist Pro</h1>
        <div className="flex items-center gap-3 text-sm">
          <Activity className="w-4 h-4" /> Live
        </div>
      </div>

      {/* 🚨 Crash Warning */}
      {crashDetected && (
        <div className="bg-red-700 text-white text-center p-4 font-bold animate-pulse">
          🚨 Crash Detected! Calling {emergency.name} in {countdown}s unless cancelled.
          <div className="mt-3 flex justify-center gap-3">
            <button onClick={cancelEmergency} className="bg-gray-800 px-4 py-2 rounded-lg">
              Cancel
            </button>
            <button
              onClick={autoCallEmergency}
              className="bg-green-600 px-4 py-2 rounded-lg flex items-center gap-1"
            >
              <Phone className="w-4 h-4" /> Call Now
            </button>
          </div>
        </div>
      )}

      {/* Dashboard */}
      {view === "dashboard" && (
        <div className="p-4 space-y-3">
          <div className="grid grid-cols-2 gap-3">
            <Card title="Speed" value={${telemetry.spd?.toFixed(1) || 0.0} km/h} icon={<Gauge />} color="from-blue-600 to-blue-700" />
            <Card title="Tilt" value={${telemetry.tilt?.toFixed(1) || 0.0}°} icon={<Compass />} color="from-purple-600 to-pink-600" />
            <Card title="G-Force" value={${telemetry.gForce?.toFixed(2) || 0.0}G} icon={<Zap />} color="from-orange-600 to-red-700" />
            <Card title="Mode" value={rideMode} icon={<Activity />} color="from-green-600 to-emerald-700" />
            <Card title="Posture" value={posture} icon={<AlertTriangle />} color="from-cyan-600 to-sky-700" />
          </div>

          <div className="flex justify-around mt-4">
            {!rideActive ? (
              <button onClick={startRide} className="bg-green-600 px-5 py-2 rounded-lg hover:bg-green-700">
                ▶ Start Ride
              </button>
            ) : (
              <button onClick={stopRide} className="bg-red-600 px-5 py-2 rounded-lg hover:bg-red-700">
                ⏹ Stop Ride
              </button>
            )}
          </div>
        </div>
      )}

      {/* Map */}
      {view === "map" && (
        <div className="p-4">
          <div id="map" className="w-full h-[500px] rounded-xl border border-gray-700 shadow-lg"></div>
          <div className="mt-2 text-center text-sm text-gray-400">
            {telemetry.lat && telemetry.lon
              ? Lat: ${telemetry.lat.toFixed(6)} | Lon: ${telemetry.lon.toFixed(6)}
              : "Waiting for GPS fix..."}
          </div>
        </div>
      )}

      {/* History */}
      {view === "history" && (
        <div className="p-4 space-y-4">
          <div className="flex justify-between items-center">
            <h2 className="text-lg font-bold">📜 Ride History</h2>
            <button onClick={clearHistory} className="text-red-400 flex items-center gap-1 text-sm">
              <Trash2 className="w-4 h-4" /> Clear All
            </button>
          </div>
          {rideSessions.length === 0 ? (
            <p className="text-gray-400">No saved rides yet.</p>
          ) : (
            rideSessions.map((ride, i) => (
              <div key={i} className="bg-gray-800 p-3 rounded-xl space-y-1">
                <div className="text-sm text-gray-300">{ride.startTime}</div>
                <div className="text-sm text-gray-400">
                  🛞 {ride.mode} | 💺 {ride.posture} | ⏱ {ride.duration} min | 🚗 {ride.distance} km | ⚡ {ride.avgSpeed} km/h
                </div>
                <div className="flex gap-3 mt-2">
                  <button onClick={() => downloadJSON(ride)} className="bg-blue-600 px-3 py-1 rounded-lg text-sm hover:bg-blue-700">
                    JSON
                  </button>
                  <button onClick={() => downloadCSV(ride)} className="bg-green-600 px-3 py-1 rounded-lg text-sm hover:bg-green-700">
                    CSV
                  </button>
                </div>
              </div>
            ))
          )}
        </div>
      )}

      {/* Settings */}
      {view === "settings" && (
        <div className="p-6 space-y-4">
          <h2 className="text-lg font-bold flex items-center gap-2">
            <Settings className="w-5 h-5" /> Emergency Contact
          </h2>
          <div className="space-y-2">
            <label className="text-sm text-gray-300">Name</label>
            <input
              type="text"
              value={emergency.name}
              onChange={(e) => setEmergency({ ...emergency, name: e.target.value })}
              className="w-full p-2 rounded bg-gray-800 text-white outline-none"
            />
          </div>
          <div className="space-y-2">
            <label className="text-sm text-gray-300">Phone Number</label>
            <input
              type="text"
              value={emergency.number}
              onChange={(e) => setEmergency({ ...emergency, number: e.target.value })}
              className="w-full p-2 rounded bg-gray-800 text-white outline-none"
            />
          </div>
          <button onClick={saveEmergency} className="bg-blue-600 px-4 py-2 rounded-lg hover:bg-blue-700">
            Save Contact
          </button>
        </div>
      )}

      {/* Navigation */}
      <div className="fixed bottom-0 left-0 right-0 bg-gray-800 border-t border-gray-700 flex justify-around py-2">
        {[
          { id: "dashboard", icon: Activity, label: "Live" },
          { id: "map", icon: MapIcon, label: "Map" },
          { id: "history", icon: History, label: "History" },
          { id: "settings", icon: Settings, label: "Settings" },
        ].map((tab) => (
          <button
            key={tab.id}
            onClick={() => setView(tab.id)}
            className={`flex flex-col items-center ${
              view === tab.id ? "text-blue-400" : "text-gray-400"
            }`}
          >
            <tab.icon className="w-5 h-5" />
            <span className="text-xs">{tab.label}</span>
          </button>
        ))}
      </div>
    </div>
  );
};

// 📦 Card Component
const Card = ({ title, value, icon, color }) => (
  <div className={bg-gradient-to-br ${color} rounded-xl p-4 shadow-lg}>
    <div className="flex justify-between mb-2">
      <span className="text-sm text-white/80">{title}</span>
      <span className="text-white/60">{icon}</span>
    </div>
    <p className="text-2xl font-bold">{value}</p>
  </div>
);

export default Index;