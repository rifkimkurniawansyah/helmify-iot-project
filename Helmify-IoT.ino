#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo;

// Konfigurasi WiFi
const char* ssid = "BOD 2.0";
const char* password = "enigma2020";

ESP8266WebServer server(80);
IPAddress local_IP(10, 10, 103, 248);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

const int infraredSensor = D0;
const int waterPump = D3;
const int blower = D4;
const int shampooScrub = D6;
const int shampooFoam = D7;
const int parfum = D8;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Helmify App");

  pinMode(infraredSensor, INPUT);
  pinMode(waterPump, OUTPUT);
  pinMode(blower, OUTPUT);
  pinMode(shampooScrub, OUTPUT);
  pinMode(shampooFoam, OUTPUT);
  pinMode(parfum, OUTPUT);
  servo.attach(D5);

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Konfigurasi IP Statis Gagal");
  }

  Serial.println("Menghubungkan ke Wi-Fi...");
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi tersambung");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

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

  const char* shampooChoice = doc["shampoo"];
  if (!shampooChoice) {
    server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Shampoo choice not provided\"}");
    return;
  }

  server.send(200, "application/json", "{\"status\":\"success\", \"message\":\"Memulai proses pencucian\"}");
  
  // unsigned long startTime = millis();
  washProcess(shampooChoice);
  // unsigned long workDuration = millis() - startTime;
  // float workDurationSeconds = workDuration / 1000.0;

  // StaticJsonDocument<300> response;
  // response["status"] = "success";
  // response["message"] = "Proses selesai";
  // JsonObject data = response.createNestedObject("data");
  // data["work_duration"] = workDurationSeconds;

  // String responseBody;
  // serializeJson(response, responseBody);
  // server.send(200, "application/json", responseBody);
}

void washProcess(const char* shampooChoice) {
  // Proses mencuci
  displayLCD("Sedang Mencuci");
  Serial.println("Mulai proses mencuci.");
  
  // Menggerakkan servo dan mengaktifkan waterPump secara bersamaan
  digitalWrite(waterPump, HIGH);
  moveServoWithWaterPump(20000);
  digitalWrite(waterPump, LOW);
  Serial.println("Proses mencuci selesai. WaterPump OFF.");

  // Proses shampoo
  displayLCD("Shampoo : " + String(shampooChoice));
  if (strcmp(shampooChoice, "scrub") == 0) {
    digitalWrite(shampooScrub, HIGH);
    Serial.println("Shampoo scrub ON.");
  } else if (strcmp(shampooChoice, "foam") == 0) {
    digitalWrite(shampooFoam, HIGH);
    Serial.println("Shampoo foam ON.");
  }
  delay(8000);
  digitalWrite(shampooScrub, LOW);
  digitalWrite(shampooFoam, LOW);
  Serial.println("Shampoo selesai. Shampoo OFF.");

  // Proses membilas
  displayLCD("Membilas");
  Serial.println("Mulai proses membilas.");
  
  digitalWrite(waterPump, HIGH);
  moveServoWithWaterPump(15000); 
  digitalWrite(waterPump, LOW);
  Serial.println("Proses membilas selesai. WaterPump OFF.");

  // Proses mengeringkan
  displayLCD("Mengeringkan");
  digitalWrite(blower, HIGH);
  Serial.println("Blower ON. Mengeringkan selama 10 detik.");
  delay(10000);
  digitalWrite(blower, LOW);
  Serial.println("Proses mengeringkan selesai. Blower OFF.");

  // Proses parfum
  displayLCD("Parfum");
  digitalWrite(parfum, HIGH);
  Serial.println("Parfum ON. Penyemprotan parfum selama 5 detik.");
  delay(5000);
  digitalWrite(parfum, LOW);
  Serial.println("Proses parfum selesai. Parfum OFF.");

  displayLCD("Proses Selesai");
  Serial.println("Semua proses selesai.");
}

void displayLCD(String message) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);
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
  Serial.println("Servo bergerak bersama WaterPump selama " + String(duration / 1000) + " detik.");
}
