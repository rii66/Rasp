#include "web_dashboard.h"

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Dioscuri</title>
<style>
body{font-family:Arial;margin:16px;max-width:520px}
.card{border:1px solid #aaa;border-radius:8px;padding:12px;margin:8px 0}
button,input{font-size:18px;padding:8px;margin:3px}
</style>
</head>
<body>
<h2>Dioscuri Master</h2>
<div class="card" id="link">Slave: --</div>
<div class="card">
<b>Solder</b><br>
Actual: <span id="st">--</span> °C
Target: <span id="ss">--</span> °C
PWM: <span id="sp">--</span>
<br><input id="sset" type="number" min="100" max="450" value="320">
<button onclick="setS()">SET</button>
<button onclick="fetch('/api/boost')">BOOST</button>
</div>
<div class="card">
<b>Hot Air</b><br>
Actual: <span id="at">--</span> °C
Target: <span id="as">--</span> °C
Fan: <span id="af">--</span>
<br><input id="aset" type="number" min="100" max="500" value="300">
<button onclick="setA()">SET</button>
<button onclick="fetch('/api/power?on=1')">ON</button>
<button onclick="fetch('/api/power?on=0')">OFF</button>
</div>
<script>
async function q(u){try{await fetch(u);load()}catch(e){}}
function setS(){q('/api/solder?temp='+document.getElementById('sset').value)}
function setA(){q('/api/hotair?temp='+document.getElementById('aset').value)}
async function load(){
 try{
  const s=await (await fetch('/api/status')).json();
  document.getElementById('link').textContent='Slave: '+(s.connected?'CONNECTED':'WAITING');
  document.getElementById('st').textContent=s.solder.temp;
  document.getElementById('ss').textContent=s.solder.target;
  document.getElementById('sp').textContent=s.solder.pwm;
  document.getElementById('at').textContent=s.air.temp;
  document.getElementById('as').textContent=s.air.target;
  document.getElementById('af').textContent=s.air.fan;
 }catch(e){}
}
setInterval(load,1000);load();
</script>
</body>
</html>
)HTML";

void WebDashboard::begin(SlaveLink* slave) {
    link = slave;

    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/api/solder", HTTP_GET, [this]() { handleSolder(); });
    server.on("/api/hotair", HTTP_GET, [this]() { handleHotAir(); });
    server.on("/api/fan", HTTP_GET, [this]() { handleFan(); });
    server.on("/api/power", HTTP_GET, [this]() { handlePower(); });
    server.on("/api/boost", HTTP_GET, [this]() { handleBoost(); });
    server.begin();
}

void WebDashboard::update() {
    server.handleClient();
}

void WebDashboard::handleRoot() {
    server.send(200, "text/html", INDEX_HTML);
}

void WebDashboard::handleStatus() {
    const auto& s = link->status();

    String json = "{";
    json += "\"connected\":" + String(link->connected() ? "true" : "false");
    json += ",\"solder\":{";
    json += "\"temp\":" + String(s.currentTemp);
    json += ",\"target\":" + String(s.targetTemp);
    json += ",\"pwm\":" + String(s.pwmOut);
    json += ",\"tipError\":" + String(s.tipError ? "true" : "false");
    json += "},\"air\":{";
    json += "\"temp\":" + String(s.airTemp);
    json += ",\"target\":" + String(s.airTargetTemp);
    json += ",\"fan\":" + String(s.airFan);
    json += ",\"on\":" + String(s.airOn ? "true" : "false");
    json += ",\"ac\":" + String(s.airHasAC ? "true" : "false");
    json += "},\"boost\":" + String(s.boostMode ? "true" : "false");
    json += ",\"sleeping\":" + String(s.sleeping ? "true" : "false");
    json += "}";

    server.send(200, "application/json", json);
}

void WebDashboard::handleSolder() {
    if (server.hasArg("temp")) link->setSolderTarget(server.arg("temp").toInt());
    server.send(200, "text/plain", "OK");
}

void WebDashboard::handleHotAir() {
    if (server.hasArg("temp")) link->setHotAirTarget(server.arg("temp").toInt());
    server.send(200, "text/plain", "OK");
}

void WebDashboard::handleFan() {
    if (server.hasArg("speed")) link->setFan((uint8_t)server.arg("speed").toInt());
    server.send(200, "text/plain", "OK");
}

void WebDashboard::handlePower() {
    bool on = server.hasArg("on") && server.arg("on").toInt() != 0;
    link->setHotAirPower(on);
    server.send(200, "text/plain", "OK");
}

void WebDashboard::handleBoost() {
    link->boost();
    server.send(200, "text/plain", "OK");
}
