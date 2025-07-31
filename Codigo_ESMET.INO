#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP085_U.h>
#include <Adafruit_BME280.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESP_Google_Sheet_Client.h>
#include "time.h"

// Pines y objetos de sensores
#define SDA_PIN 9
#define SCL_PIN 8
#define DHTPIN   10
#define DHTTYPE  DHT11
const int sensorPin = 4;

const char* ssid     = "Ai.L.";
const char* password = "654973ejthnd";

Adafruit_AHTX0 aht;
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
Adafruit_BME280 bme;
DHT dht(DHTPIN, DHTTYPE);

WebServer server(80);

bool ahtOk=false, bmpOk=false, bmeOk=false, dhtOk=false;
volatile unsigned long ultimaVuelta = 0;
volatile float tiempoEntreVueltas = 0;
const unsigned long tiempoRebote = 200;

// Google Sheets (Mobizt) configuración:
#define PROJECT_ID "estmet-log"
#define CLIENT_EMAIL "log-estmet@estmet-log.iam.gserviceaccount.com"
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDhcW+xAvqaaCK2\nKKxhARIAdO7REgJeio7u7y52BiMW0VmnaJfUIC5mMf2sF1PnxN12tZDBIUvARXjy\nR9L01ySr9uYgFMgdDRcubCxMdhT5P6AebITj9DHPS2VtoFkCzQ3ev2g/hfZVQekL\n9+LXAbz7MS89W9g3fX1E1vos9/sfK7BYooyqofExZ5fXMpdjSO/+6NUjDYBJRH1O\nNKZBq4dephD2aSgKfGYcvxv8G40r/4cokag6v3OP0ZcBsL47XH2S6QFYyD1ThNlQ\n76fbBdq6iTy/7A8a30efgr6N9Qjw26P9YidsO/6V4W9u+gzrWAJOXBmJbpR3rveI\ntYt+VD+DAgMBAAECggEARi7s4RP21Wv25nOqGnbdVlQQ5vhu0WTcU4kVkzuc3+Uj\nvojCv5CXMUx91v3aKqywb1zd6CoH7Z8TJIX0qDF0Iq71/GXL2LNzFIorjlAyflfu\n7pnFEzw553lm5aaGH2eb2k0HVMNNKUSgGxA8R8gsP50Twkvd94RzI6OzdZ1ryctX\n18TIxdfUuZhutF3J9TN8EQrc5PuZEYJnwCgvqOzWHMtFO6JFr4Ah/elswdG2bgSY\naVCwy+G7WxMUn2RWh4nNBtkYupviUnR68+DDeyyl1fRDF9uRib6yHdMtrIAPRPl2\n7hzL/9fhQpxrzyvFK2wg8gQ4zKqwiRStXTAyUXOvXQKBgQD7ziy05euC5/CDxfjw\n13O6UEgLwJI/Z1vPudnl0DXXX0ENPOwYVwqJiWLwR2txGFnag9dSR6FuKiKjQTTw\nMjYxqzDEkBUIe6R4AM4q/kn/whqXztF/RrYvZASdIdyI65vqqeo2ym9nesmCgNN3\nMmOuddCOevG3wa/YM63H9x2/5QKBgQDlMtb0HJAA160kR/npXJehK3haaqzQu1HH\nXSJCTU3ftTJPUW0IwKQGzB6fioXCQLE7Q/qvIUdzK9i5/WooZEcesdhzvuLwGpmm\nsJfX+GSjPuw4CmwsBauCNuWIzHhQx1/VicCZ3UtpokV/vBKeFtftjpaP645BtvqX\n6+G1Ck17RwKBgAU8pXthqMNBNaRX8rR3lqWSU91CtQfyi1tHnzuDWAtRpNW0pahf\n7w0rOU6L/bUNb5j20vyYFshmwe+pRIOWV8AyskH0SSuy/pmsUp1cSoeyLsFngL3n\n55QWvW3TLXX/iG+Z9xAKp8R8LkKzkTJSbvZK5vp/nhVXRuP1NzEId6rtAoGBAOEG\nwTR6toedOUjdwef0GIsMeuWKxpFI2Em8jt45t7Nj2zk04LAvMtvZnFdXUWwex+NA\nIue+7L+O1QIGZCu0CH+CbNl3GKhT9XdK+kn4fhYouRNsaDfweSsnasScu6PTm6uz\ny31FX70erM10gXLlSqrZy8kRpGGwnOh7luaAbwIRAoGAJ09R7u4PRe5XZZ2cJI4e\n63L3DMabxGIDaQ2J3/7bJbOKqIE9wWPFv++PoYhWSVvlNO7vLKaCgtJ0iZghWSZ4\ncI9Xpji4Fcj290LOFnNs7RB2pue+iC2va5lZcmz+1aU8cDW50iIXMwruNLoS123i\nRTFigIO5QD8Bc7CIn13mERs=\n-----END PRIVATE KEY-----\n";
const char spreadsheetId[] = "1gHOcUPHknd9NSwt-69-ZOzvSwD1rmEIIS-GkCYLmRp8";

void tokenStatusCallback(TokenInfo info) {
  if (info.status == token_status_error) {
    Serial.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
  } else {
    Serial.printf("Token estado: %s\n", GSheet.getTokenStatus(info).c_str());
  }
}

// index_html para página web
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Estación Meteorológica</title>
  <style>
    :root { --bg:#f5f7fa; --card:#fff; --accent:#4a90e2; --text:#333; }
    *{box-sizing:border-box;margin:0;padding:0;}
    body{font-family:Arial,sans-serif;background:var(--bg);color:var(--text);padding:1rem;}
    h1{text-align:center;color:var(--accent);margin-bottom:1rem;}
    .row{display:flex;flex-wrap:wrap;gap:1rem;justify-content:center;margin-bottom:1rem;}
    .card{background:var(--card);border-radius:8px;box-shadow:0 2px 6px rgba(0,0,0,0.1);padding:1rem;flex:1 1 200px;max-width:240px;}
    .card h2{color:var(--accent);font-size:1.1rem;margin-bottom:0.5rem;}
    .card p{margin:0.3rem 0;font-size:1rem;}
  </style>
</head>
<body>
  <h1>Estación Meteorológica</h1>
  <!-- Primer nivel: Sensores -->
  <div class="row">
    <div class="card"><h2>AHT10</h2><p>Temp: <span id="aht_t">--</span>°C</p><p>Hum: <span id="aht_h">--</span>%</p></div>
    <div class="card"><h2>DHT11</h2><p>Temp: <span id="dht_t">--</span>°C</p><p>Hum: <span id="dht_h">--</span>%</p></div>
    <div class="card"><h2>BMP180</h2><p>Temp: <span id="bmp_t">--</span>°C</p><p>Presión: <span id="bmp_p">--</span> hPa</p></div>
    <div class="card"><h2>BME280</h2><p>Temp: <span id="bme_t">--</span>°C</p><p>Hum: <span id="bme_h">--</span>%</p><p>Presión: <span id="bme_p">--</span> hPa</p></div>
  </div>
  <!-- Segundo nivel: Promedios -->
  <div class="row">
    <div class="card"><h2>Promedio Temp</h2><p><span id="avg_t">--</span>°C</p></div>
    <div class="card"><h2>Promedio Hum</h2><p><span id="avg_h">--</span>%</p></div>
    <div class="card"><h2>Promedio Presión</h2><p><span id="avg_p">--</span> hPa</p></div>
  </div>
  <!-- Tercer nivel: Anemómetro -->
  <div class="row">
    <div class="card"><h2>Anemómetro</h2><p>Vel: <span id="wind_ms">--</span> m/s | <span id="wind_kmh">--</span> km/h</p><p>Tiempo vuelta: <span id="tiempoEntreVueltas">--</span> s</p></div>
  </div>
  <script>
    async function actualizar(){
      const d = await (await fetch('/data')).json();
      document.getElementById('aht_t').textContent = d.aht_t.toFixed(2);
      document.getElementById('aht_h').textContent = d.aht_h.toFixed(2);
      document.getElementById('dht_t').textContent = d.dht_t.toFixed(2);
      document.getElementById('dht_h').textContent = d.dht_h.toFixed(2);
      document.getElementById('bmp_t').textContent = d.bmp_t.toFixed(2);
      document.getElementById('bmp_p').textContent = d.bmp_p.toFixed(2);
      document.getElementById('bme_t').textContent = d.bme_t.toFixed(2);
      document.getElementById('bme_h').textContent = d.bme_h.toFixed(2);
      document.getElementById('bme_p').textContent = d.bme_p.toFixed(2);
      document.getElementById('avg_t').textContent = d.avg_t.toFixed(2);
      document.getElementById('avg_h').textContent = d.avg_h.toFixed(2);
      document.getElementById('avg_p').textContent = d.avg_p.toFixed(2);
      document.getElementById('wind_ms').textContent = d.wind_ms.toFixed(2);
      document.getElementById('wind_kmh').textContent = d.wind_kmh.toFixed(2);
      document.getElementById('tiempoEntreVueltas').textContent = d.tiempoEntreVueltas.toFixed(2);
    }
    setInterval(actualizar,2000);
    window.onload=actualizar;
  </script>
</body>
</html>
)rawliteral";

void IRAM_ATTR contarVuelta(){
  unsigned long t = millis();
  if(t - ultimaVuelta > tiempoRebote){
    tiempoEntreVueltas = (t - ultimaVuelta) / 1000.0;
    ultimaVuelta = t;
  }
}

void inicializarSensores(){
  Wire.begin(SDA_PIN, SCL_PIN);
  ahtOk = aht.begin();
  bmpOk = bmp.begin();
  bmeOk = bme.begin(0x76);
  dht.begin();
  delay(100);
  dhtOk = !isnan(dht.readTemperature());
}

void handleRoot(){
  server.send_P(200, "text/html", index_html);
}

void handleData(){
  float aht_t=0, aht_h=0, bmp_t=0, bmp_p=0, bme_t=0, bme_h=0, bme_p=0;
  float dht_t=0, dht_h=0, wind_kmh=0, wind_ms=0;
  float sum_t=0, sum_h=0, sum_p=0;
  int cnt_t=0, cnt_h=0, cnt_p=0;

  if(ahtOk){
    sensors_event_t hum, temp;
    aht.getEvent(&hum, &temp);
    aht_t = temp.temperature; aht_h = hum.relative_humidity;
    sum_t += aht_t; sum_h += aht_h; cnt_t++; cnt_h++;
  }
  if(dhtOk){
    dht_t = dht.readTemperature(); dht_h = dht.readHumidity();
    sum_t += dht_t; sum_h += dht_h; cnt_t++; cnt_h++;
  }
  if(bmpOk){
    bmp.getTemperature(&bmp_t);
    bmp.getPressure(&bmp_p);
    bmp_p /= 100.0;
    sum_t += bmp_t; sum_p += bmp_p; cnt_t++; cnt_p++;
  }
  if(bmeOk){
    bme_t = bme.readTemperature(); bme_h = bme.readHumidity(); bme_p = bme.readPressure()/100.0F;
    sum_t += bme_t; sum_h += bme_h; sum_p += bme_p; cnt_t++; cnt_h++; cnt_p++;
  }
  if(tiempoEntreVueltas > 0){
    float freq = 1.0 / tiempoEntreVueltas;
    wind_kmh = freq * 2.4;
    wind_ms  = wind_kmh * 0.27778;
  }

  float avg_t = cnt_t ? sum_t / cnt_t : 0;
  float avg_h = cnt_h ? sum_h / cnt_h : 0;
  float avg_p = cnt_p ? sum_p / cnt_p : 0;

  StaticJsonDocument<256> doc;
  doc["aht_t"] = aht_t;    doc["aht_h"] = aht_h;
  doc["bmp_t"] = bmp_t;    doc["bmp_p"] = bmp_p;
  doc["bme_t"] = bme_t;    doc["bme_h"] = bme_h;    doc["bme_p"] = bme_p;
  doc["dht_t"] = dht_t;    doc["dht_h"] = dht_h;
  doc["wind_kmh"] = wind_kmh; doc["wind_ms"] = wind_ms;
  doc["tiempoEntreVueltas"] = tiempoEntreVueltas;
  doc["avg_t"] = avg_t;    doc["avg_h"] = avg_h;    doc["avg_p"] = avg_p;

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

unsigned long lastTime = 0;
unsigned long timerDelay = 10000;  // cada 10 segundos (ajústalo)

void setup(){
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while(WiFi.status()!=WL_CONNECTED) delay(500);
  Serial.printf("\nIP: %s\n", WiFi.localIP().toString().c_str());

  inicializarSensores();
  pinMode(sensorPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(sensorPin), contarVuelta, FALLING);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.begin();
  Serial.println("Servidor iniciado");

  // Configuración de Sheets
  configTime(0, 0, "pool.ntp.org");
  delay(2000);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    time_t now = mktime(&timeinfo);
    GSheet.setSystemTime(now);
  }
  GSheet.setTokenCallback(tokenStatusCallback);
  GSheet.setPrerefreshSeconds(300);
  GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);
}

void loop(){
  server.handleClient();

  if (!GSheet.ready()) return;

  if (millis() - lastTime >= timerDelay) {
    lastTime = millis();

    // Lee valores idénticos a handleData, pero sin enviar JSON al cliente
    float aht_t=0,aht_h=0,bmp_t=0,bmp_p=0,bme_t=0,bme_h=0,bme_p=0;
    float dht_t=0,dht_h=0,wind_kmh=0,wind_ms=0;
    float sum_t=0,sum_h=0,sum_p=0;
    int cnt_t=0,cnt_h=0,cnt_p=0;

    if(ahtOk){ sensors_event_t hum,temp; aht.getEvent(&hum,&temp); aht_t=temp.temperature; aht_h=hum.relative_humidity; sum_t+=aht_t; sum_h+=aht_h; cnt_t++; cnt_h++; }
    if(dhtOk){ dht_t=dht.readTemperature(); dht_h=dht.readHumidity(); sum_t+=dht_t; sum_h+=dht_h; cnt_t++; cnt_h++; }
    if(bmpOk){ bmp.getTemperature(&bmp_t); bmp.getPressure(&bmp_p); bmp_p/=100.0; sum_t+=bmp_t; sum_p+=bmp_p; cnt_t++; cnt_p++; }
    if(bmeOk){ bme_t=bme.readTemperature(); bme_h=bme.readHumidity(); bme_p=bme.readPressure()/100.0F; sum_t+=bme_t; sum_h+=bme_h; sum_p+=bme_p; cnt_t++; cnt_h++; cnt_p++; }
    if(tiempoEntreVueltas>0){ float freq=1.0/tiempoEntreVueltas; wind_kmh = freq * 2.4; wind_ms = wind_kmh * 0.27778; }

    float avg_t = cnt_t? sum_t/cnt_t:0;
    float avg_h = cnt_h? sum_h/cnt_h:0;
    float avg_p = cnt_p? sum_p/cnt_p:0;

    // Prepara JSON para append
    FirebaseJson response;
    FirebaseJson valueRange;
    valueRange.add("majorDimension", "COLUMNS");
    // timestamp en primera columna
    valueRange.set("values/[0]/[0]", time(nullptr));
    valueRange.set("values/[1]/[0]", aht_t);
    valueRange.set("values/[2]/[0]", aht_h);
    valueRange.set("values/[3]/[0]", dht_t);
    valueRange.set("values/[4]/[0]", dht_h);
    valueRange.set("values/[5]/[0]", bmp_t);
    valueRange.set("values/[6]/[0]", bmp_p);
    valueRange.set("values/[7]/[0]", bme_t);
    valueRange.set("values/[8]/[0]", bme_h);
    valueRange.set("values/[9]/[0]", bme_p);
    valueRange.set("values/[10]/[0]", avg_t);
    valueRange.set("values/[11]/[0]", avg_h);
    valueRange.set("values/[12]/[0]", avg_p);
    valueRange.set("values/[13]/[0]", wind_ms);
    valueRange.set("values/[14]/[0]", wind_kmh);
    valueRange.set("values/[15]/[0]", tiempoEntreVueltas);

    bool ok = GSheet.values.append(&response, spreadsheetId, "Sheet1!A1", &valueRange);
    if (ok) {
      Serial.println("Values appended to Google Sheets");
      response.toString(Serial, true);
      valueRange.clear();
    } else {
      Serial.printf("Append failed: %s\n", GSheet.errorReason().c_str());
    }
  }
}
