// server/server.js
import express from "express";
import http from "http";
import cors from "cors";
import { Server } from "socket.io";

const app = express();
app.use(cors());
app.use(express.json());

// HTTP + WebSocket server
const server = http.createServer(app);
const io = new Server(server, {
  cors: {
    origin: "http://localhost:5173", // React dev server
    methods: ["GET", "POST"],
  },
});

// When React connects
io.on("connection", (socket) => {
  console.log("🟢 Web client connected:", socket.id);
  socket.emit("message", "Connected to Rider Telemetry Server");

  socket.on("disconnect", () => {
    console.log("🔴 Web client disconnected:", socket.id);
  });
});

// Endpoint for ESP32 data
app.post("/api/sensor", (req, res) => {
  const data = req.body;
  console.log("📩 Sensor Data Received:", data);

  // Send to all connected React clients
  io.emit("sensor:update", data);

  res.json({ status: "ok" });
});

// Health check
app.get("/", (req, res) => res.send("Rider Telemetry Backend Running 🚀"));

const PORT = 5000;
server.listen(PORT, () => console.log(`✅ Server running on port ${PORT}`));
