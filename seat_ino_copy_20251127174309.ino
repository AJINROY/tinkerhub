#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "DHT.h"

const char* ssid = "ajin";
const char* password = "87654321";

#define DHT_PIN D4         
#define DHT_TYPE DHT11
#define TRIG_PIN D5        
#define ECHO_PIN D6      
#define BUZZER_PIN D7   

const unsigned long MEASURE_INTERVAL_MS = 2000; 
const float OCCUPIED_DISTANCE_CM = 40.0;        
const unsigned long ALARM_SECONDS = 10;         

DHT dht(DHT_PIN, DHT_TYPE);
ESP8266WebServer server(80);

unsigned long lastMeasure = 0;
float lastTemp = NAN;
float lastHum = NAN;
float lastDistance = NAN;
bool occupied = false;
unsigned long occupiedStart = 0;
bool alarmPlaying = false;
unsigned long uptimeStart = 0;

void handleRoot() {
  String page = R"rawliteral(
    <!doctype html><html><head><meta charset="utf-8">
    <title>Seat Monitor</title>
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <style>
      body{font-family:Arial,Helvetica,sans-serif;padding:18px;max-width:600px;margin:auto;background-color:light-blue}
      .card{border:1px solid #ddd;padding:12px;border-radius:8px;margin-bottom:12px}
      .big{font-size:1.6rem;font-weight:700}
      .small{color:#555}
      .muted{color:#777;font-size:0.9rem}
      button{padding:8px 12px;border-radius:6px;border:0;background:#1976d2;color:white}
      #aj{text-align:center;}
    </style>
    </head><body>
    <h1 id="aj">Seat Monitoring</h1>
    <h2>Seat Monitor</h2>
    <div id="info" class="card">Loading...</div>
    <div class="card small">Auto-updates every 2s</div>
    <script>
      function formatMinutes(min) {
        // show minutes as M min S sec if >1 minute, else show seconds
        try {
          var m = Math.floor(min);
          var seconds = Math.round((min - m) * 60);
          if (m > 0) return m + " min " + seconds + " s";
          return seconds + " s";
        } catch (e) { return min.toFixed(1) + " min"; }
      }

      async function fetchData(){
        try {
          const r = await fetch('/data');
          const j = await r.json();
          const status = j.occupied ? 'Occupied' : 'Empty';
          const minutesText = formatMinutes(Number(j.minutes_sitting));
          const html = `
            <div class="big">${status}</div>
            <div>Time sitting: <strong>${minutesText}</strong></div>
            <div>Distance: ${Number(j.distance).toFixed(1)} cm</div>
            <div>Temp: ${Number(j.temp).toFixed(1)} °C, Humidity: ${Number(j.humidity).toFixed(1)}%</div>
            <div class="muted">Uptime (s): ${j.uptime_s}</div>
          `;
          document.getElementById('info').innerHTML = html;
        } catch(e) {
          document.getElementById('info').innerText = 'Cannot fetch data';
        }
      }

      setInterval(fetchData, 2000);
      fetchData();
    </script>
    </body></html>
  )rawliteral";
  server.send(200, "text/html", page);
}


void handleData() {
  float minutes_sitting = 0.0;
  if (occupied && occupiedStart != 0) {
    minutes_sitting = (millis() - occupiedStart) / 60000.0; 
  }

  String json = "{";
  json += "\"occupied\":" + String(occupied ? "true" : "false") + ",";
  json += "\"distance\":" + String(lastDistance) + ",";
  json += "\"temp\":" + String(lastTemp) + ",";
  json += "\"humidity\":" + String(lastHum) + ",";
  json += "\"uptime_s\":" + String((millis()-uptimeStart)/1000) + ",";
  json += "\"minutes_sitting\":" + String(minutes_sitting, 1); 
  json += "}";
  server.send(200, "application/json", json);
}


void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println("Booting...");
  dht.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.printf("Connecting to WiFi '%s' ...\n", ssid);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi connect failed (timeout).");
  }

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("HTTP server started");

  uptimeStart = millis();
  lastMeasure = 0;
  
}

float measureDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL); 
  if (duration == 0) return 999.0; 
  float cm = (duration / 2.0) / 29.1;
  return cm;
}

void playAlarmOnce() {
  for (int i=0;i<3;i++){
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(150);
  }
}

void loop() {
  server.handleClient();

  if (millis() - lastMeasure >= MEASURE_INTERVAL_MS) {
    lastMeasure = millis();
    lastTemp = dht.readTemperature();
    lastHum = dht.readHumidity();
    lastDistance = measureDistanceCM();
    Serial.printf("Temp: %.1f C, Hum: %.1f%%, Dist: %.1f cm\n", lastTemp, lastHum, lastDistance);

    bool nowOccupied = (lastDistance > 0 && lastDistance < OCCUPIED_DISTANCE_CM);
    if (nowOccupied && !occupied) {
      occupiedStart = millis();
      occupied = true;
      alarmPlaying = false;
      Serial.println("Occupied started.");
    } else if (!nowOccupied && occupied) {
      occupied = false;
      occupiedStart = 0;
      alarmPlaying = false;
      Serial.println("seat is free.");
    } else if (nowOccupied && occupied) {
      unsigned long secs = (millis() - occupiedStart) / 1000;
      if (!alarmPlaying && secs >= ALARM_SECONDS) {
        Serial.println("ALARM: occupied too long - playing beep");
        playAlarmOnce();
        alarmPlaying = true;
      }
    }
  }
}
