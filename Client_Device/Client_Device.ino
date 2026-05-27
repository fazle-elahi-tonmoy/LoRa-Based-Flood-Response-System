#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define LORA_SS 5
#define LORA_RST 17
#define LORA_DIO0 16

#define BTN_A 0
#define BTN_B 33
#define BUZZER 25
#define BATTERY_PIN 34

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

String MAC = "MAC001";

float LAT = 23.7806;
float LON = 90.4070;

bool waitingACK = false;
unsigned long ackTimer = 0;

void setup() {

  Serial.begin(115200);

  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);



  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
  display.setTextColor(1);

  showReadyScreen();
}

void loop() {

  receiveLoRa();

  checkButtons();

  checkACKTimeout();
}

void checkButtons() {

  if (digitalRead(BTN_A) == LOW) {
    delay(200);
    sendHelp("FOOD");
  }

  if (digitalRead(BTN_B) == LOW) {
    delay(200);
    sendHelp("RESCUE");
  }
}

void sendHelp(String type) {

  String packet = "HELP|" + MAC + "|HOST|" + type + "|" + String(LAT, 6) + "|" + String(LON, 6);

  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();

  Serial.println(packet);

  waitingACK = true;
  ackTimer = millis();

  displaySending(type);
}

void receiveLoRa() {

  int packetSize = LoRa.parsePacket();

  if (packetSize) {

    String msg = "";

    while (LoRa.available()) {
      msg += (char)LoRa.read();
    }

    Serial.println("RX:" + msg);

    processMessage(msg);
  }
}

void processMessage(String msg) {

  if (msg.startsWith("ALERT")) {

    displayWarning();

    tone(BUZZER, 1000);

    delay(10000);

    noTone(BUZZER);

    showReadyScreen();
  }

  if (msg.startsWith("ACK")) {

    String target = getValue(msg, '|', 2);

    if (target == MAC) {

      waitingACK = false;

      displayAck();
    }
  }
}

void checkACKTimeout() {

  if (waitingACK) {

    if (millis() - ackTimer > 5000) {

      waitingACK = false;

      displayRetry();
    }
  }
}

String getValue(String data, char separator, int index) {

  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {

    if (data.charAt(i) == separator || i == maxIndex) {

      found++;

      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void showReadyScreen() {

  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("DISASTER NETWORK");

  display.drawLine(0, 10, 128, 10, WHITE);

  display.setCursor(0, 20);
  display.print("STATUS: READY");

  display.setCursor(0, 35);
  display.print("BTN1: FOOD/MED");

  display.setCursor(0, 50);
  display.print("BTN2: RESCUE");

  display.display();
}

void displaySending(String type) {

  display.clearDisplay();

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("SENDING");

  display.setTextSize(1);
  display.setCursor(0, 30);
  display.print("TYPE: ");
  display.print(type);

  display.setCursor(0, 50);
  display.print("WAITING ACK...");

  display.display();
}

void displayAck() {

  display.clearDisplay();

  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print("SENT");

  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("HELP IS COMING");

  display.display();

  delay(2000);

  showReadyScreen();
}

void displayRetry() {

  display.clearDisplay();

  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print("NO RESP");

  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("TRY AGAIN");

  display.display();

  delay(2000);

  showReadyScreen();
}

void displayWarning() {

  display.clearDisplay();

  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print("WARNING");

  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("FLOOD DETECTED");

  display.display();
}