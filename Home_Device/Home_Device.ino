#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
Adafruit_SSD1306 display(128, 64, &Wire, -1);

//8.21 -> 2.61

#define DEVICE_ID "Home2"
#define latitude 23.7712142   //23.7637073
#define longitude 90.4011672  //90.3999387

#define LORA_NSS 5
#define LORA_RST 16
#define LORA_DIO0 4

#define BUZZER 32
#define BATTERY_PIN 33
#define btn1 25
#define btn2 26

int battery = 0;
bool messageActive = false;
String messageText = "";
unsigned long messageEnd = 0, cooldown = 0, refresh = 0;

void beep(int n, int d) {
  for (int i = 0; i < n; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(d);
    digitalWrite(BUZZER, LOW);
    if (i < n - 1) delay(d);
  }
}

int readBattery() {
  int raw = analogRead(BATTERY_PIN);
  raw = map(raw, 2750, 3350, 0, 100);
  int batt = constrain(raw, 0, 100);
  return batt;
}

void sendTelemetry(String type) {
  display.clearDisplay();
  text(type, 4, 12, 2);
  text("Requested", 11, 36, 2);
  delay(1000);

  type.trim();
  StaticJsonDocument<256> doc;
  doc["type"] = type;
  doc["id"] = DEVICE_ID;
  doc["lat"] = latitude;
  doc["lon"] = longitude;

  String packet;
  serializeJson(doc, packet);
  Serial.println(packet);

  int retries = 0;
  bool ackReceived = false;

  while (retries < 3 && !ackReceived) {
    LoRa.beginPacket();
    LoRa.print(packet);
    LoRa.endPacket();
    LoRa.receive();
    unsigned long waitStart = millis();
    while (millis() - waitStart < 1000) {
      int size = LoRa.parsePacket();
      if (size) {
        String incoming = LoRa.readString();
        StaticJsonDocument<128> ack;
        deserializeJson(ack, incoming);
        if (ack["type"] == "ack" && ack["id"] == DEVICE_ID) {
          ackReceived = true;
          break;
        }
      }
    }
    retries++;
  }

  display.clearDisplay();
  if (ackReceived) {
    text("   HELP   ", 4, 12, 2);
    text("On the Way", 4, 36, 2);
    beep(2, 100);
  } else {
    text(" Sending ", 11, 12, 2);
    text(" Failed! ", 11, 36, 2);
    beep(1, 500);
  }

  messageActive = true;
  messageEnd = millis() + 3000;
}

void handleCommand(String msg) {
  StaticJsonDocument<256> doc;
  deserializeJson(doc, msg);

  String type = doc["type"];

  if (type == "warn") {
    String target = doc["target"];

    if (target == DEVICE_ID || target == "ALL") {
      display.clearDisplay();
      text("  Flood  ", 11, 16, 2);
      text(" Warning! ", 4, 48, 2);
      beep(3, 500);
      messageActive = true;
      messageEnd = millis() + 10000;
    }
  }
}

void checkLoRa() {
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    String incoming = LoRa.readString();
    handleCommand(incoming);
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(BUZZER, OUTPUT);
  pinMode(btn1, INPUT_PULLUP);
  pinMode(btn2, INPUT_PULLUP);
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(1);

  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  while (!LoRa.begin(433E6)) {
    display.setCursor(0, 0);
    display.print("LoRa Error");
    display.display();
    delay(1000);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.enableCrc();
}

void loop() {
  battery = readBattery();
  if (messageActive && millis() > messageEnd) messageActive = false;
  if (!messageActive && millis() > refresh) {
    drawHome();
    refresh = millis() + 1000;
  }

  if (!digitalRead(btn1) && millis() > cooldown) {
    Serial.println("Button 1 pressed");
    sendTelemetry("  Supply  ");
    cooldown = millis() + 5000;
  }

  if (!digitalRead(btn2) && millis() > cooldown) {
    Serial.println("Button 2 pressed");
    sendTelemetry("  Rescue  ");
    cooldown = millis() + 5000;
  }

  checkLoRa();

  delay(10);
}

void text(String t, int x, int y, int s) {
  display.setTextSize(s);
  display.setCursor(x, y);
  display.print(t);
  display.display();
}

void drawHome() {
  display.clearDisplay();
  display.setTextSize(2);

  display.setCursor(0, 12);
  display.print("Batt: ");
  display.print(battery);
  display.print("%");

  display.setCursor(0, 36);
  display.print("ID: ");
  display.print(DEVICE_ID);

  display.display();
}
