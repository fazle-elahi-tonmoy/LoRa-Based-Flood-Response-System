#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

#define LORA_NSS 5
#define LORA_RST 16
#define LORA_DIO0 4

#define WATER_PIN 34
#define BATTERY_PIN 33

#define THRESHOLD 2000  // 🔥 tune this

unsigned long lastTrigger = 0;
const unsigned long cooldown = 60000;  // 1 minutes

int readBattery() {
  int raw = analogRead(BATTERY_PIN);
  raw = map(raw, 2300, 3350, 0, 100);
  return constrain(raw, 0, 100);
}

void sendWarning() {
  StaticJsonDocument<128> doc;
  doc["type"] = "warn";
  doc["target"] = "ALL";

  String packet;
  serializeJson(doc, packet);

  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();
  LoRa.receive();

  // 🖥 Display
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(11, 12);
  display.print(" Warning ");
  display.setCursor(4, 36);
  display.print("   Sent   ");
  display.display();
  delay(1000);
}

void setup() {
  Serial.begin(115200);

  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(1);

  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  while (!LoRa.begin(433E6)) {
    delay(1000);
  }
}

void loop() {
  int water = analogRead(WATER_PIN);
  int battery = readBattery();

  // 🔴 Trigger warning
  if (water > THRESHOLD && millis() > lastTrigger) {
    sendWarning();
    lastTrigger = millis() + cooldown;
  }

  delay(1000);
  // 🖥 Display
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 12);
  display.print("Batt:");
  display.print(battery);
  display.print("%");
  display.setCursor(0, 36);
  display.print("W: ");
  display.print(water);
  display.display();
}