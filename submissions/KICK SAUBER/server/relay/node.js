const express = require('express');
const bodyParser = require('body-parser');
const WebSocket = require('ws');

const app = express();
app.use(bodyParser.json());
const server = require('http').createServer(app);
const wss = new WebSocket.Server({ server });

app.post('/update', (req, res) => {
  console.log('Received:', req.body);
  const msg = JSON.stringify(req.body);
  wss.clients.forEach(c => c.readyState === WebSocket.OPEN && c.send(msg));
  res.send('ok');
});

server.listen(3000, () => console.log('Relay running on port 3000'));