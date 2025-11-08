import axios from "axios";

setInterval(async () => {
  const randomData = {
    speed: (Math.random() * 50).toFixed(2),
    acceleration: {
      x: (Math.random() - 0.5) * 4,
      y: (Math.random() - 0.5) * 4,
      z: (Math.random() - 0.5) * 4,
    },
    gps: {
      lat: 17.4485 + (Math.random() - 0.5) * 0.0005,
      lon: 78.3908 + (Math.random() - 0.5) * 0.0005,
      timestamp: new Date().toLocaleTimeString(),
    },
  };

  await axios.post("http://localhost:5000/api/sensors/data", randomData);
  console.log("Sent mock data");
}, 1000);
