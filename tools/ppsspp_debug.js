// Talks to the PPSSPP WebSocket debugger.
// Usage: node tools/ppsspp_debug.js <port> '<json event>' ['<json event>' ...]
// Prints the responses; for screenshots it saves the PNG to build/screenshot.png.
const fs = require("fs");
const path = require("path");

const port = process.argv[2];
const events = process.argv.slice(3).map((e) => JSON.parse(e));
const ws = new WebSocket(`ws://127.0.0.1:${port}/debugger`);
let pending = 0;

ws.onopen = () => {
  // cpu.stepping / cpu.resume do not reply with the ticket: we do not wait for them
  const noReply = ["cpu.stepping", "cpu.resume"];
  events.forEach((ev, i) => {
    ev.ticket = String(i);
    if (!noReply.includes(ev.event)) pending++;
    ws.send(JSON.stringify(ev));
  });
  if (!pending) setTimeout(() => ws.close(), 500);
};

ws.onmessage = (msg) => {
  const data = JSON.parse(msg.data);
  if (data.ticket === undefined) return; // eventi spontanei (log, ecc.)
  if (data.uri && data.uri.startsWith("data:image/png;base64,")) {
    const out = path.join(__dirname, "..", "build", "screenshot.png");
    fs.writeFileSync(out, Buffer.from(data.uri.split(",")[1], "base64"));
    data.uri = `(salvato in ${out})`;
  }
  if (data.base64 && data.base64.length > 200) data.base64 = data.base64.slice(0, 200) + "...";
  console.log(JSON.stringify(data));
  if (--pending === 0) setTimeout(() => ws.close(), 300);
};

ws.onclose = () => process.exit(0);
ws.onerror = (e) => {
  console.error("WebSocket error:", e.message || e);
  process.exit(1);
};
setTimeout(() => {
  console.error("Timeout");
  process.exit(1);
}, 20000);
