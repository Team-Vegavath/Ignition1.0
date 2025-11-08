import L from "leaflet";
import "leaflet/dist/leaflet.css";
import React, { useState, useEffect, useRef } from "react";
import { Battery, Gauge } from "lucide-react";
import { io } from "socket.io-client";
import "./index.css";

// Fix for Leaflet default markers
delete L.Icon.Default.prototype._getIconUrl;
L.Icon.Default.mergeOptions({
  iconRetinaUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.7.1/images/marker-icon-2x.png',
  iconUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.7.1/images/marker-icon.png',
  shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.7.1/images/marker-shadow.png',
});

const RiderTelemetryApp = () => {
  const [activeTab, setActiveTab] = useState("live");
  const [rideState, setRideState] = useState("walking");
  const [speed, setSpeed] = useState(0);
  const [acceleration, setAcceleration] = useState({ x: 0, y: 0, z: 0 });
  const [deceleration, setDeceleration] = useState(0);
  const [gps, setGps] = useState({
    lat: 17.4485,
    lon: 78.3908,
    timestamp: new Date().toLocaleTimeString(),
  });
  const [gpsPoints, setGpsPoints] = useState(0);
  const [isRecording, setIsRecording] = useState(false);
  const [batteryLevel, setBatteryLevel] = useState(87);
  const [rideTime, setRideTime] = useState(0);

  const mapRef = useRef(null);
  const mapInstanceRef = useRef(null);
  const markerRef = useRef(null);
  const pathLineRef = useRef(null);

  // ✅ Initialize Leaflet Map - FIXED VERSION
  useEffect(() => {
    // Only initialize when map tab is active and map container exists
    if (activeTab !== "map" || !mapRef.current || mapInstanceRef.current) return;

    console.log("🔄 Initializing map...");

    // Small delay to ensure DOM is ready
    setTimeout(() => {
      try {
        const mapInstance = L.map(mapRef.current, {
          center: [gps.lat, gps.lon],
          zoom: 16,
          zoomControl: false,
          attributionControl: false,
        });

        // Add dark tile layer for better visibility with your theme
        L.tileLayer("https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png", {
          maxZoom: 19,
          attribution: '© OpenStreetMap contributors, © CartoDB'
        }).addTo(mapInstance);

        // Create marker
        const marker = L.marker([gps.lat, gps.lon]).addTo(mapInstance);

        // Create path line
        const pathLine = L.polyline([], { 
          color: "#007bff", 
          weight: 4,
          opacity: 0.7,
        }).addTo(mapInstance);

        markerRef.current = marker;
        pathLineRef.current = pathLine;
        mapInstanceRef.current = mapInstance;

        console.log("✅ Map initialized successfully");
        
        // Force a resize to ensure tiles load
        setTimeout(() => {
          mapInstance.invalidateSize();
        }, 100);

      } catch (error) {
        console.error("❌ Map initialization error:", error);
      }
    }, 100);

    return () => {
      // Cleanup when component unmounts OR when switching away from map tab
      if (mapInstanceRef.current) {
        console.log("🧹 Cleaning up map");
        mapInstanceRef.current.remove();
        mapInstanceRef.current = null;
        markerRef.current = null;
        pathLineRef.current = null;
      }
    };
  }, [activeTab]); // Re-initialize when tab changes to map

  // ✅ Update map when GPS changes - FIXED
  useEffect(() => {
    if (activeTab !== "map" || !mapInstanceRef.current || !markerRef.current) return;

    const newLatLng = [gps.lat, gps.lon];
    
    // Update marker position
    markerRef.current.setLatLng(newLatLng);
    
    // Update path
    if (pathLineRef.current) {
      const currentPath = pathLineRef.current.getLatLngs();
      currentPath.push(newLatLng);
      pathLineRef.current.setLatLngs(currentPath);
    }
    
    // Smooth pan to new position
    mapInstanceRef.current.panTo(newLatLng, {
      animate: true,
      duration: 0.5
    });

    // Ensure map size is correct
    mapInstanceRef.current.invalidateSize();
  }, [gps, activeTab]);

  // ✅ Handle tab change - FIXED
  useEffect(() => {
    if (activeTab === "map" && mapInstanceRef.current) {
      // When switching to map tab, ensure map is properly sized
      setTimeout(() => {
        mapInstanceRef.current.invalidateSize();
      }, 300);
    }
  }, [activeTab]);

  // ✅ Real-time data listener from backend
  useEffect(() => {
    const socket = io("http://localhost:5000");

    socket.on("connect", () => console.log("🟢 Connected to backend"));
    
    socket.on("sensor:update", (data) => {
      console.log("📥 New data from sensor:", data);

      setSpeed(data.speed || 0);
      setAcceleration(data.acceleration || { x: 0, y: 0, z: 0 });
      setDeceleration(data.deceleration || 0);
      
      if (data.gps) {
        setGps({
          ...data.gps,
          timestamp: new Date().toLocaleTimeString()
        });
      }
      
      setBatteryLevel(data.batteryLevel || batteryLevel);
      setGpsPoints(prev => prev + 1);
    });

    socket.on("disconnect", () => console.log("🔴 Disconnected from backend"));
    
    return () => {
      socket.disconnect();
    };
  }, []);

  // ✅ Utility: time formatter
  const formatTime = (seconds) => {
    const mins = Math.floor(seconds / 60);
    const secs = seconds % 60;
    return `${mins}:${secs.toString().padStart(2, "0")}`;
  };

  // ✅ Utility: ride mode info
  const getRideStateInfo = () => {
    switch (rideState) {
      case "scooter":
        return { icon: "🛴", label: "Scooter" };
      case "motorcycle":
        return { icon: "🏍️", label: "Motorcycle" };
      case "walking":
        return { icon: "🚶", label: "Walking" };
      default:
        return { icon: "—", label: "Idle" };
    }
  };

  const stateInfo = getRideStateInfo();

  return (
    <div className="telemetry-app">
      <header className="header">
        <div>
          <h1>Telemetry</h1>
          <p>Rider Motion System</p>
        </div>
        <div className="battery">
          <Battery size={16} />
          <span>{batteryLevel}%</span>
        </div>
      </header>

      <div className="status-bar">
        <div className="state-info">
          <span className="icon">{stateInfo.icon}</span>
          <div>
            <p className="state-label">{stateInfo.label}</p>
            <p className="sub-label">Current Mode</p>
          </div>
        </div>
        {isRecording && (
          <div className="recording">
            <div className="dot"></div>
            <span>{formatTime(rideTime)}</span>
          </div>
        )}
      </div>

      <nav className="tabs">
        {["live", "map"].map((tab) => (
          <button
            key={tab}
            onClick={() => setActiveTab(tab)}
            className={activeTab === tab ? "active" : ""}
          >
            {tab.charAt(0).toUpperCase() + tab.slice(1)}
          </button>
        ))}
      </nav>

      <main className="content">
        {activeTab === "live" && (
          <>
            <div className="speed-display">
              <Gauge size={24} />
              <h2>{speed.toFixed(1)}</h2>
              <p>km/h</p>
            </div>

            <div className="acceleration">
              <h3>Acceleration (3-Axis)</h3>
              <div className="accel-grid">
                {["x", "y", "z"].map((axis) => (
                  <div key={axis} className="accel-box">
                    <p className="label">{axis.toUpperCase()}</p>
                    <p className="value">
                      {acceleration[axis].toFixed(2)} <span>m/s²</span>
                    </p>
                  </div>
                ))}
              </div>
            </div>

            <div className="decel">
              <h3>Deceleration</h3>
              <p className="decel-value">{deceleration.toFixed(2)} m/s²</p>
            </div>
          </>
        )}

        {activeTab === "map" && (
          <div className="map-section">
            <div 
              id="map" 
              ref={mapRef} 
              className="map-container"
              style={{ 
                height: '400px', 
                width: '100%',
                background: '#000'
              }}
            >
              {!mapInstanceRef.current && (
                <div style={{ 
                  display: 'flex', 
                  alignItems: 'center', 
                  justifyContent: 'center', 
                  height: '100%',
                  color: '#888',
                  fontSize: '14px'
                }}>
                  Loading map...
                </div>
              )}
            </div>
            <div className="gps-info">
              <p>Latitude: {gps.lat.toFixed(5)}° N</p>
              <p>Longitude: {gps.lon.toFixed(5)}° E</p>
              <p>Timestamp: {gps.timestamp}</p>
              <p>Total GPS Points: {gpsPoints}</p>
            </div>
          </div>
        )}
      </main>

      <footer className="controls">
        <button
          className={`record-btn ${isRecording ? "stop" : "start"}`}
          onClick={() => setIsRecording(!isRecording)}
        >
          {isRecording ? "Stop Recording" : "Start Recording"}
        </button>
        <button
          className="mode-btn"
          onClick={() => {
            const states = ["walking", "scooter", "motorcycle"];
            const current = states.indexOf(rideState);
            setRideState(states[(current + 1) % states.length]);
          }}
        >
          🔄
        </button>
      </footer>
    </div>
  );
};

export default RiderTelemetryApp;