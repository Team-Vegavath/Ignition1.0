import React, { useEffect, useState } from "react";

function LiveData() {
  const [data, setData] = useState(null);

  useEffect(() => {
    const ws = new WebSocket("ws://192.168.x.x:3001"); // 👈 Replace with your PC's IP

    ws.onopen = () => console.log("✅ Connected to relay");
    ws.onmessage = (e) => {
      try {
        const json = JSON.parse(e.data);
        setData(json);
        console.log("Received:", json);
      } catch (err) {
        console.error("Bad JSON:", e.data);
      }
    };
    ws.onerror = (err) => console.error("WS error:", err);
    ws.onclose = () => console.log("❌ Disconnected");

    return () => ws.close();
  }, []);

  return (
    <div>
      <h2>Live ESP32 Data</h2>
      {data ? (
        <pre>{JSON.stringify(data, null, 2)}</pre>
      ) : (
        <p>Waiting for data...</p>
      )}
    </div>
  );
}

export default LiveData;
