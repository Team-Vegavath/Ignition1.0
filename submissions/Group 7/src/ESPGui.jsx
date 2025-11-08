// Paste of the ESPGui component produced earlier (trimmed slightly for brevity)
import React, { useEffect, useRef, useState } from "react";
import { LineChart, Line, XAxis, YAxis, Tooltip, ResponsiveContainer } from "recharts";
import { motion } from "framer-motion";

const SERIAL_BAUD = 115200;
const MAX_POINTS = 120;

function parseSerialChunk(chunk) {
  const lines = chunk.split(/\r?\n/).map(s => s.trim()).filter(Boolean);
  const parsed = [];
  for (const line of lines) {
    const timeMatch = /time_ms\s*=\s*(\d+)\s*ms/i.exec(line);
    if (timeMatch) { parsed.push({ type: 'time', value: parseInt(timeMatch[1],10) }); continue; }
    const axesMatch = /ax_lin_ms2_filtered\s*=\s*([-+]?\d*\.?\d+)\s+\s*ay_lin_ms2_filtered\s*=\s*([-+]?\d*\.?\d+)\s+\s*az_lin_ms2_filtered\s*=\s*([-+]?\d*\.?\d+)/i.exec(line);
    if (axesMatch) { parsed.push({ type:'axes', value: { ax: parseFloat(axesMatch[1]), ay: parseFloat(axesMatch[2]), az: parseFloat(axesMatch[3]) } }); continue; }
    const magMatch = /linMag_filtered\s*=\s*([-+]?\d*\.?\d+)\s+\s*abs\s*=\s*([-+]?\d*\.?\d+)\s+\s*state\s*=\s*(\w+)/i.exec(line);
    if (magMatch) { parsed.push({ type:'mag', value: { linMag: parseFloat(magMatch[1]), aabs: parseFloat(magMatch[2]), state: magMatch[3] } }); continue; }
    parsed.push({ type:'raw', value: line });
  }
  return parsed;
}

export default function ESPGui() {
  const [port, setPort] = useState(null);
  const [connected, setConnected] = useState(false);
  const [logLines, setLogLines] = useState([]);
  const [latest, setLatest] = useState({ time:0, ax:0, ay:0, az:0, linMag:0, aabs:0, state:'-' });
  const [chartData, setChartData] = useState([]);
  const [mockMode, setMockMode] = useState(false);
  const readerRef = useRef(null);
  const keepReadingRef = useRef(false);

  useEffect(()=>{ return () => { disconnectSerial(); } },[]);

  async function connectSerial() {
    if (!('serial' in navigator)) { alert('Web Serial API not supported'); return; }
    try {
      const _port = await navigator.serial.requestPort();
      await _port.open({ baudRate: SERIAL_BAUD });
      setPort(_port); setConnected(true); appendLog('Connected to serial port.'); keepReadingRef.current = true; readLoop(_port);
    } catch (e) { appendLog('Serial connect error: ' + e.message); }
  }

  async function disconnectSerial() {
    keepReadingRef.current = false;
    if (readerRef.current) { try { await readerRef.current.cancel(); } catch(e){} readerRef.current = null; }
    if (port) { try { await port.close(); } catch(e){} setPort(null); }
    setConnected(false); appendLog('Disconnected.');
  }

  function appendLog(s) { setLogLines(prev => { const next = [...prev, `${new Date().toLocaleTimeString()} | ${s}`]; return next.slice(-200); }); }

  async function readLoop(_port) {
    const textDecoder = new TextDecoderStream();
    const readableStreamClosed = _port.readable.pipeTo(textDecoder.writable);
    const reader = textDecoder.readable.getReader();
    readerRef.current = reader;
    try {
      while (keepReadingRef.current) {
        const { value, done } = await reader.read();
        if (done) break;
        if (value) {
          const parsed = parseSerialChunk(value);
          for (const p of parsed) handleParsed(p);
        }
      }
    } catch (e) { appendLog('Read error: ' + e.message); } finally { try { reader.releaseLock(); } catch(e){} readerRef.current = null; }
  }

  function handleParsed(p) {
    if (p.type === 'time') { setLatest(lat => ({ ...lat, time: p.value })); }
    else if (p.type === 'axes') { setLatest(lat => ({ ...lat, ax: p.value.ax, ay: p.value.ay, az: p.value.az })); }
    else if (p.type === 'mag') {
      setLatest(lat => {
        const newState = { ...lat, linMag: p.value.linMag, aabs: p.value.aabs, state: p.value.state };
        pushChartPoint(newState);
        return newState;
      });
    } else if (p.type === 'raw') { appendLog(p.value); }
  }

  function pushChartPoint(lat) {
    setChartData(prev => {
      const newPoint = { t: prev.length? prev[prev.length-1].t + 1 : 0, linMag: +(lat.linMag.toFixed(3)) };
      const next = [...prev, newPoint];
      return next.slice(-MAX_POINTS);
    });
  }

  useEffect(()=>{
    let iv;
    if (mockMode) {
      let t = 0;
      iv = setInterval(()=>{
        const now = Date.now();
        const ax = (Math.sin(t/5)*0.3 + (Math.random()-0.5)*0.02);
        const ay = (Math.cos(t/7)*0.25 + (Math.random()-0.5)*0.02);
        const az = (Math.sin(t/3)*0.12 + (Math.random()-0.5)*0.01);
        const linMag = Math.sqrt(ax*ax+ay*ay+az*az);
        let state = 'Stationary';
        const mag = Math.abs(linMag);
        if (mag < 0.2) state='Stationary'; else if (mag < 1.5) state='Walking'; else state='Scooter';
        const chunk = `time_ms = ${now} ms\nax_lin_ms2_filtered = ${ax.toFixed(4)}  ay_lin_ms2_filtered = ${ay.toFixed(4)}  az_lin_ms2_filtered = ${az.toFixed(4)}\nlinMag_filtered = ${linMag.toFixed(4)}  abs = ${linMag.toFixed(4)}  state = ${state}\n`;
        const parsed = parseSerialChunk(chunk);
        parsed.forEach(p=>handleParsed(p));
        appendLog(`(mock) ${state} / linMag=${linMag.toFixed(4)}`);
        t++;
      }, 800);
    }
    return ()=>{ if (iv) clearInterval(iv); };
  }, [mockMode]);

  return (
    <div style={{fontFamily:'Inter, system-ui, sans-serif', padding:20}}>
      <header style={{marginBottom:12}}>
        <h1>ESP32 MPU6050 — Live GUI</h1>
        <p style={{color:'#666'}}>Connect via Web Serial to stream the same values the sketch prints and visualise them.</p>
      </header>
      <div style={{display:'flex',gap:12}}>
        <div style={{flex:2, background:'#fff', padding:12, borderRadius:12, boxShadow:'0 6px 18px rgba(0,0,0,0.06)'}}>
          <div style={{display:'flex',justifyContent:'space-between',alignItems:'center',marginBottom:10}}>
            <div>
              <div style={{fontSize:12,color:'#666'}}>Connection</div>
              <div style={{fontWeight:600}}>{connected ? 'Connected' : 'Disconnected'}</div>
            </div>
            <div style={{display:'flex',gap:8}}>
              <button onClick={()=>setMockMode(m=>!m)} style={{padding:'6px 10px'}}>Mock: {mockMode? 'On':'Off'}</button>
              {!connected ? (<button onClick={connectSerial} style={{padding:'6px 10px',background:'#10b981',color:'#fff'}}>Connect</button>) : (<button onClick={disconnectSerial} style={{padding:'6px 10px',background:'#ef4444',color:'#fff'}}>Disconnect</button>)}
            </div>
          </div>

          <div style={{display:'flex',gap:8,marginBottom:12}}>
            <StatCard label="ax (m/s²)" value={latest.ax} />
            <StatCard label="ay (m/s²)" value={latest.ay} />
            <StatCard label="az (m/s²)" value={latest.az} />
          </div>

          <div style={{display:'flex',gap:12}}>
            <div style={{flex:2, height:220, background:'#f9fafb', padding:8, borderRadius:8}}>
              <ResponsiveContainer width="100%" height="100%">
                <LineChart data={chartData} margin={{ top: 8, right: 8, left: 4, bottom: 8 }}>
                  <XAxis dataKey="t" hide />
                  <YAxis domain={[0, 'dataMax + 0.5']} />
                  <Tooltip />
                  <Line type="monotone" dataKey="linMag" dot={false} strokeWidth={2} />
                </LineChart>
              </ResponsiveContainer>
            </div>

            <div style={{width:180, background:'#fff', padding:12, borderRadius:8}}>
              <div style={{fontSize:12,color:'#666'}}>Timestamp</div>
              <div style={{fontWeight:700}}>{latest.time}</div>
              <div style={{fontSize:12,color:'#666',marginTop:8}}>linMag</div>
              <div style={{fontSize:20,fontWeight:800}}>{latest.linMag.toFixed(3)}</div>
              <div style={{fontSize:12,color:'#666',marginTop:8}}>State</div>
              <div style={{marginTop:6, padding:'6px 10px', borderRadius:999, background:'#f1f5f9', display:'inline-block'}}>{latest.state}</div>
            </div>
          </div>
        </div>

        <aside style={{width:360, background:'#fff', padding:12, borderRadius:12}}>
          <h3 style={{fontSize:13,color:'#555'}}>Live Log</h3>
          <div style={{height:340, overflow:'auto', fontFamily:'menlo, monospace', fontSize:12, background:'#fbfdff', padding:8, borderRadius:6}}>
            {logLines.map((l,i)=>(<div key={i} style={{padding:'2px 0'}}>{l}</div>))}
          </div>
        </aside>
      </div>
    </div>
  );
}

function StatCard({ label, value }){
  return (
    <div style={{background:'#fff',padding:8,borderRadius:8,boxShadow:'0 6px 18px rgba(0,0,0,0.04)',flex:1}}>
      <div style={{fontSize:12,color:'#666'}}>{label}</div>
      <div style={{fontWeight:700,fontSize:16}}>{(value||0).toFixed(4)}</div>
    </div>
  );
}