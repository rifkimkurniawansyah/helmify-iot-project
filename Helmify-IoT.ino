#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// LiquidCrystal_I2C lcd(0x27, 16, 2);
LiquidCrystal_I2C lcd(0x3f, 20, 4);
Servo servo;

const char* ssid = "BOD 2.0";
const char* password = "enigma2020";

ESP8266WebServer server(80);

const int waterPump = D3;
const int blower = D4;
const int sabunScrub = D6;
const int sabunFoam = D7;
const int parfumLavender = D8;
const int parfumLemon = 3;
const int doorButton = D5;
const int buzzerPin = 1;

bool isDryClean = false;
String sabunChoice;
String parfumChoice;
String machineId;

// IP yang diizinkan untuk mengirim permintaan
const char* allowedIp = "10.10.103.27";

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  welcomeMessage();

  pinMode(waterPump, OUTPUT);
  pinMode(blower, OUTPUT);
  pinMode(sabunScrub, OUTPUT);
  pinMode(sabunFoam, OUTPUT);
  pinMode(parfumLavender, OUTPUT);
  pinMode(parfumLemon, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(doorButton, INPUT_PULLUP);

  servo.attach(D0);

  digitalWrite(waterPump, HIGH);
  digitalWrite(blower, HIGH);
  digitalWrite(sabunScrub, HIGH);
  digitalWrite(sabunFoam, HIGH);
  digitalWrite(parfumLavender, HIGH);
  digitalWrite(parfumLemon, HIGH);
  digitalWrite(buzzerPin, LOW);

  Serial.println("Menghubungkan ke Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi tersambung");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Mac Address: ");
  Serial.println(WiFi.macAddress());

  server.on("/", HTTP_POST, handleWashRequest);

  server.onNotFound([]() {
    server.send(404, "application/json", "{\"status\":\"error\", \"message\":\"Endpoint tidak ditemukan\"}");
  });

  server.begin();
  Serial.println("Server HTTP dimulai");
}

void loop() {
  server.handleClient();
}

void handleWashRequest() {
  // Cek IP pengirim
  if (server.client().remoteIP().toString() != allowedIp) {
    server.send(403, "application/json", "{\"status\":\"error\", \"message\":\"Akses ditolak: IP tidak diizinkan\"}");
    return;
  }

  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"No JSON data provided\"}");
    return;
  }

  String jsonData = server.arg("plain");
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, jsonData);

  if (error) {
    Serial.print("Parsing JSON gagal: ");
    Serial.println(error.c_str());
    server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Invalid JSON\"}");
    return;
  }

  const char* sabun = doc["sabun"];
  const char* parfum = doc["parfum"];
  const char* machine_id = doc["machine_id"];

  machineId = String(machine_id);

  if ((strlen(sabun) > 0) && (strlen(parfum) > 0)) {
    isDryClean = false;
    sabunChoice = sabun ? String(sabun) : "";
    parfumChoice = parfum ? String(parfum) : "";
  } else {
    isDryClean = true;
    sabunChoice = "";
    parfumChoice = "";
  }

  Serial.println("Machine ID: " + machineId);

  server.send(200, "application/json", "{\"status\":\"success\", \"message\":\"Machine Working\"}");

  buzzer();

  displayLcd("Close The Door");

  while (digitalRead(doorButton) == HIGH) {
    delay(100);
  }
  displayLcd("Processing ...");
  delay(1000);

  buzzer();

  washProcess(sabunChoice.c_str(), parfumChoice.c_str());
}

void washProcess(const char* sabunChoice, const char* parfumChoice) {
  if (isDryClean) {
    displayLcd("Dry Cleaning");
    dryCleanProcess();
    sendMachineId();
  } else {
    displayLcd("Washing");
    digitalWrite(waterPump, LOW);
    moveServoWithWaterPump(20000);
    digitalWrite(waterPump, HIGH);

    buzzer();

    if (strlen(sabunChoice) > 0) {
      displayLcd("Detergent:" + String(sabunChoice));
      if (strcmp(sabunChoice, "scrub") == 0) {
        digitalWrite(sabunScrub, LOW);
      } else if (strcmp(sabunChoice, "foam") == 0) {
        digitalWrite(sabunFoam, LOW);
      }
      delay(8000);
      digitalWrite(sabunScrub, HIGH);
      digitalWrite(sabunFoam, HIGH);
    }

    buzzer();

    displayLcd("Rinsing");
    digitalWrite(waterPump, LOW);
    moveServoWithWaterPump(15000);
    digitalWrite(waterPump, HIGH);

    buzzer();
    dryCleanProcess();
    buzzer();

    if (strlen(parfumChoice) > 0) {
      displayLcd("Perfume:" + String(parfumChoice));
      if (strcmp(parfumChoice, "lavender") == 0) {
        digitalWrite(parfumLavender, LOW);
      } else if (strcmp(parfumChoice, "lemon") == 0) {
        digitalWrite(parfumLemon, LOW);
      }
      delay(5000);
      digitalWrite(parfumLavender, HIGH);
      digitalWrite(parfumLemon, HIGH);
    }
  }
  buzzer();

  displayLcd("Process Finish");
  delay(3000);
  welcomeMessage();
  Serial.println(machineId);
  sendMachineId();
}

void dryCleanProcess() {
  displayLcd("Drying");
  digitalWrite(blower, LOW);
  delay(10000);
  digitalWrite(blower, HIGH);
}

void displayLcd(String message) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);
}

void welcomeMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  displayLcd("Welcome To");
  lcd.setCursor(0, 1);
  lcd.print("Helmify");
}

void buzzer() {
  digitalWrite(buzzerPin, HIGH);
  delay(500);
  digitalWrite(buzzerPin, LOW);
}

void moveServoWithWaterPump(unsigned long duration) {
  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    for (int pos = 0; pos <= 180; pos++) {
      servo.write(pos);
      delay(15);
    }
    for (int pos = 180; pos >= 0; pos--) {
      servo.write(pos);
      delay(15);
    }
  }
}

void sendMachineId() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = "http://139.59.253.126:8080/api/transaction-finish/" + machineId;
    WiFiClient client;

    http.begin(client, url);

    int httpResponseCode = http.POST("");

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Response Code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error Code: " + String(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("Error: Not connected to WiFi");
  }
}
