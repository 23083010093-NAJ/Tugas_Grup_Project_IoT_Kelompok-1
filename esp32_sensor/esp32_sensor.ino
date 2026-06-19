#include <WiFi.h>
#include <HTTPClient.h>

// ================= PIN =================
#define TRIG_PIN 13
#define ECHO_PIN 14
#define BUZZER   33

#define GREEN_LED  4
#define YELLOW_LED 18
#define RED_LED    19

// ================= WIFI =================
#define WIFI_SSID "Nao"
#define WIFI_PASSWORD "1sampe89"
String SERVER_URL = "http://172.20.10.5:5000/sensor";

// ================= VARIABLES =================
float distance = 999;

unsigned long lastSensor = 0;
unsigned long lastSend = 0;
unsigned long lastBeep = 0;
bool buzzerState = false;

// ================= WIFI =================
void connectWiFi() {
  Serial.print("Connecting WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  Serial.println(WiFi.localIP());
}

// ================= SENSOR =================
float readDistance() {

  float total = 0;
  int valid = 0;

  for (int i = 0; i < 3; i++) {

    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 20000);

    if (duration > 0) {
      total += duration * 0.0343 / 2;
      valid++;
    }

    delay(3);
  }

  if (valid == 0) return 999;

  return total / valid;
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(BUZZER, OUTPUT);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  digitalWrite(BUZZER, LOW);

  connectWiFi();

  Serial.println("SMART BLIND ASSISTANT READY");
}

// ================= LOOP =================
void loop() {

  unsigned long now = millis();

  // ================= SENSOR =================
  if (now - lastSensor > 150) {
    lastSensor = now;
    distance = readDistance();

    Serial.print("Distance: ");
    Serial.println(distance);
  }

  // ================= STATUS =================
  String status;

  if (distance > 100) status = "SAFE";
  else if (distance > 50) status = "WARNING";
  else if (distance > 20) status = "DANGER";
  else status = "CRITICAL";

  // ================= LED =================
  digitalWrite(GREEN_LED, status == "SAFE");
  digitalWrite(YELLOW_LED, status == "WARNING");
  digitalWrite(RED_LED, status == "DANGER" || status == "CRITICAL");

  // ================= ACTIVE BUZZER LOGIC =================

  int interval;

  if (status == "SAFE") {
    digitalWrite(BUZZER, LOW);
  }

  else if (status == "WARNING") {
    interval = 700;   // lambat
  }

  else if (status == "DANGER") {
    interval = 250;   // cepat
  }

  else { // CRITICAL
    digitalWrite(BUZZER, HIGH); // bunyi terus
    return;
  }

  // blinking logic (WARNING & DANGER)
  if (status == "WARNING" || status == "DANGER") {

    if (now - lastBeep > interval) {
      lastBeep = now;
      buzzerState = !buzzerState;

      digitalWrite(BUZZER, buzzerState ? HIGH : LOW);
    }
  }

  // ================= HTTP =================
  if (now - lastSend > 2000) {
    lastSend = now;

    HTTPClient http;
    http.begin(SERVER_URL);
    http.addHeader("Content-Type", "application/json");

    String payload =
      "{\"distance\":" + String(distance) +
      ",\"status\":\"" + status + "\"}";

    int code = http.POST(payload);

    Serial.print("HTTP: ");
    Serial.println(code);

    http.end();
  }

  delay(5);
}