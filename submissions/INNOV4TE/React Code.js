import React, { useEffect, useState, useRef } from "react";
import L from "leaflet";
import {
  Activity,
  Map as MapIcon,
  Zap,
  AlertTriangle,
  Download,
  Phone,
  Clock,
  History,
  User,
} from "lucide-react";
import "leaflet/dist/leaflet.css";

const PI_URL = "http://10.240.213.80:5000/data";
const EMERGENCY_NUMBER = "+911234567890"; // Change to your contact

const App = () => {
  const [telemetry, setTelemetry] = useState({});
  const [view, setView] = useState("dashboard"); // dashboard, map, history, contact
  const [crashDetected, setCrashDetected] = useState(false);
  const [countdown, setCountdown] = useState(0);
  const [history, setHistory] = useState([]);
  const countdownRef = useRef(null);
  const mapRef = useRef(null);
  const markerRef = useRef(null);
  const polylineRef = useRef(null);

  // 🚀 Fetch live telemetry
  useEffect(() => {
    const interval = setInterval(() => {
      fetch(PI_URL)
        .then((res) => res.json())
        .then((data) => {
          if (data && data.ax !== undefined) {
            const gForce =
              Math.sqrt(data.ax ** 2 + data.ay ** 2 + data.az ** 2) / 9.8;
            const speed =
              data.gps_spd && data.gps_spd > 0.1
                ? data.gps_spd
                : data.mpu_spd || 0;

            const newData = {
              ...data,
              gForce,
              spd: parseFloat(speed.toFixed(2)),
              timestamp: Date.now(),
            };

            // Use phone GPS if module not working
            if (!data.lat || !data.lon) {
              navigator.geolocation.getCurrentPosition(
                (pos) => {
                  newData.lat = pos.coords.latitude;
                  newData.lon = pos.coords.longitude;
                },
                () => console.warn("⚠ Phone GPS unavailable")
              );
            }

            setTelemetry(newData);
            setHistory((prev) => [...prev.slice(-1000), newData]);

            if (gForce > 3.5) triggerCrashSequence(gForce);
          }
        })
        .catch((err) => console.warn("⚠ Cannot reach Pi:", err.message));
    }, 500);
    return () => clearInterval(interval);
  }, []);

  // 🚨 Crash Detection
  const triggerCrashSequence = (gForce) => {
    if (!crashDetected) {
      setCrashDetected(true);
      setCountdown(30);

      countdownRef.current = setInterval(() => {
        setCountdown((prev) => {
          if (prev <= 1) {
            clearInterval(countdownRef.current);
            callEmergency();
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

  const callEmergency = () => {
    window.location.href = tel:${EMERGENCY_NUMBER};
  };

  // 🗺 Map setup
  useEffect(() => {
    if (view === "map") {
      setTimeout(() => {
        const mapContainer = document.getElementById("map");
        if (!mapContainer) return;

        if (!mapRef.current) {
          mapRef.current = L.map(mapContainer).setView([12.9716, 77.5946], 15);

          L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
            attribution: "© OpenStreetMap",
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

  // 💾 Ride history save & download
  const downloadJSON = () => {
    const blob = new Blob([JSON.stringify(history, null, 2)], {
      type: "application/json",
    });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = ride_${new Date().toISOString()}.json;
    a.click();
  };

  const downloadCSV = () => {
    if (!history.length) return;
    const headers = Object.keys(history[0]).join(",");
    const rows = history.map((obj) => Object.values(obj).join(","));
    const csv = [headers, ...rows].join("\n");
    const blob = new Blob([csv], { type: "text/csv" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = ride_${new Date().toISOString()}.csv;
    a.click();
  };

  // 📊 Dashboard
  const Dashboard = () => (
    <div className="p-4 space-y-3">
      <div className="grid grid-cols-2 gap-3">
        <Card title="Speed" value={${telemetry.spd || 0} km/h} icon={<Activity />} color="from-blue-600 to-blue-700" />
        <Card title="Tilt" value={${telemetry.tilt?.toFixed(1) || 0}°} icon={<AlertTriangle />} color="from-purple-600 to-pink-600" />
        <Card title="G-Force" value={${telemetry.gForce?.toFixed(2) || 0}G} icon={<Zap />} color="from-orange-600 to-red-700" />
        <Card title="Posture" value={telemetry.posture || "Upright"} icon={<User />} color="from-green-600 to-emerald-700" />
        <Card title="Mode" value={telemetry.ride_mode || "Idle"} icon={<Clock />} color="from-gray-700 to-gray-900" />
      </div>
    </div>
  );

  // 📜 Ride History
  const HistoryView = () => (
    <div className="p-4 text-white">
      <h2 className="text-xl font-bold mb-3 flex items-center gap-2">
        <History className="w-5 h-5" /> Ride History
      </h2>
      <div className="space-y-2 max-h-[400px] overflow-y-auto">
        {history.length === 0 && (
          <p className="text-gray-400">No rides recorded yet...</p>
        )}
        {history.slice(-20).reverse().map((item, i) => (
          <div
            key={i}
            className="bg-gray-800 rounded-lg p-3 text-sm flex justify-between"
          >
            <span>{new Date(item.timestamp).toLocaleTimeString()}</span>
            <span>{item.spd} km/h</span>
            <span>{item.posture}</span>
          </div>
        ))}
      </div>
      <div className="flex justify-center gap-3 mt-4">
        <button
          onClick={downloadJSON}
          className="bg-blue-600 hover:bg-blue-700 px-4 py-2 rounded-lg flex items-center gap-1"
        >
          <Download className="w-4 h-4" /> JSON
        </button>
        <button
          onClick={downloadCSV}
          className="bg-green-600 hover:bg-green-700 px-4 py-2 rounded-lg flex items-center gap-1"
        >
          <Download className="w-4 h-4" /> CSV
        </button>
      </div>
    </div>
  );

  // 📞 Emergency Page
  const ContactPage = () => (
    <div className="p-6 text-center text-white">
      <h2 className="text-xl font-bold mb-4">Emergency Contact</h2>
      <p className="text-gray-400 mb-3">In case of crash, app will auto-call:</p>
      <p className="text-2xl font-bold mb-6">{EMERGENCY_NUMBER}</p>
      <button
        onClick={callEmergency}
        className="bg-red-600 hover:bg-red-700 px-6 py-3 rounded-lg font-semibold text-lg flex items-center justify-center gap-2 mx-auto"
      >
        <Phone className="w-5 h-5" /> Call Now
      </button>
    </div>
  );

  return (
    <div className="min-h-screen bg-gray-900 text-white">
      {/* Header */}
      <div className="bg-gradient-to-r from-blue-600 to-purple-600 p-4 flex justify-between items-center">
        <h1 className="text-xl font-bold">RideAssist Pro</h1>
        <div className="flex items-center gap-2 text-sm">
          <Activity className="w-4 h-4" /> Live
        </div>
      </div>

      {/* Crash alert */}
      {crashDetected && (
        <div className="bg-red-700 text-white text-center p-3 animate-pulse">
          🚨 Crash Detected! Auto calling in {countdown}s
          <div className="mt-3 flex justify-center gap-3">
            <button
              onClick={cancelEmergency}
              className="bg-gray-800 px-4 py-2 rounded-lg hover:bg-gray-700"
            >
              Cancel
            </button>
            <button
              onClick={callEmergency}
              className="bg-green-600 px-4 py-2 rounded-lg hover:bg-green-700 flex items-center gap-1"
            >
              <Phone className="w-4 h-4" /> Call Now
            </button>
          </div>
        </div>
      )}

      {/* Views */}
      {view === "dashboard" && <Dashboard />}
      {view === "map" && (
        <div className="p-4">
          <div
            id="map"
            className="w-full h-[500px] rounded-xl border border-gray-700 shadow-lg"
          ></div>
        </div>
      )}
      {view === "history" && <HistoryView />}
      {view === "contact" && <ContactPage />}

      {/* Bottom nav */}
      <div className="fixed bottom-0 left-0 right-0 bg-gray-800 border-t border-gray-700 flex justify-around py-2">
        {[
          { id: "dashboard", icon: Activity, label: "Live" },
          { id: "map", icon: MapIcon, label: "Map" },
          { id: "history", icon: History, label: "History" },
          { id: "contact", icon: Phone, label: "Emergency" },
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

const Card = ({ title, value, icon, color }) => (
  <div className={bg-gradient-to-br ${color} rounded-xl p-4 shadow-lg}>
    <div className="flex justify-between mb-1">
      <span className="text-sm text-white/80">{title}</span>
      <span className="text-white/60">{icon}</span>
    </div>
    <p className="text-2xl font-bold">{value}</p>
  </div>
);

export default App;