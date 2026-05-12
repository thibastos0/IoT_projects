#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

const char *ssid = "Wokwi-GUEST";
const char *password = "";
//const char *ssid = "Sala Maker";
//const char *password = "Maker@fatec";

WebServer server(80);
const int LED_PIN = 13;

// Página HTML estilizada
String getHTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>ESP32 Web Control</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:sans-serif; text-align:center; padding:20px;}";
  html += ".btn{background:#2ecc71; color:white; padding:15px 25px; border:none; border-radius:5px; font-size:18px; cursor:pointer; text-decoration:none;}";
  html += ".btn-off{background:#e74c3c;}";
  html += "</style></head><body>";
  html += "<h1>ESP32 Wokwi Server</h1>";
  html += "<p>Status do LED: <strong>" + String(digitalRead(LED_PIN) ? "LIGADO" : "DESLIGADO") + "</strong></p>";
  html += "<p><a href='/on'><button class='btn'>LIGAR</button></a></p>";
  html += "<p><a href='/off'><button class='btn btn-off'>DESLIGAR</button></a></p>";
  html += "</body></html>";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", getHTML());
}

void handleLedOn() {
  digitalWrite(LED_PIN, HIGH);
  server.send(200, "text/html", getHTML());
}

void handleLedOff() {
  digitalWrite(LED_PIN, LOW);
  server.send(200, "text/html", getHTML());
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=== ESP32 Web Server ===");
  Serial.print("Conectando ao WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  // Timeout de 20 segundos para conectar
  int tentativas = 40;
  while (WiFi.status() != WL_CONNECTED && tentativas > 0) {
    delay(500);
    Serial.print(".");
    tentativas--;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado ao WiFi!");
    Serial.print("IP local: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nNão conectou ao WiFi, mas iniciando servidor mesmo assim...");
    Serial.println("IP esperado: 10.0.0.1");
  }

  server.on("/", handleRoot);
  server.on("/on", handleLedOn);
  server.on("/off", handleLedOff);
  
  server.begin();
  Serial.println("Servidor HTTP iniciado na porta 80");
  Serial.println("Acesse: http://localhost:8180");
}

void loop() {
  server.handleClient();
  delay(2);
}