#include <SPI.h>
#include <LoRa.h>

#define LORA_SS 5
#define LORA_RST 17
#define LORA_DIO0 16

void setup() {

  pinMode(0, INPUT_PULLUP);
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("HOST START");

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1)
      ;
  }
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  Serial.println("LoRa ready");
}

void loop() {

  int packetSize = LoRa.parsePacket();

  if (packetSize) {

    String msg = "";

    while (LoRa.available()) {
      msg += (char)LoRa.read();
    }

    Serial.println("LORA:" + msg);

    processMessage(msg);
  }

  if (digitalRead(0) == 0) {
    broadcastAlert();
    delay(1000);
  }
}

void processMessage(String msg) {

  // WATER SENSOR MESSAGE
  if (msg.startsWith("WATER")) {

    Serial.println("EVENT:FLOOD_WARNING");

    broadcastAlert();
  }

  // CLIENT HELP MESSAGE
  if (msg.startsWith("HELP")) {

    String sender = getValue(msg, '|', 1);

    String type = getValue(msg, '|', 3);

    String lat = getValue(msg, '|', 4);

    String lon = getValue(msg, '|', 5);

    Serial.println("HELP," + sender + "," + type + "," + lat + "," + lon);

    sendACK(sender);
  }
}

void broadcastAlert() {

  String packet = "ALERT|HOST|ALL|FLOOD";

  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();

  Serial.println("ALERT_SENT");
}

void sendACK(String mac) {

  String packet = "ACK|HOST|" + mac + "|OK";

  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();

  Serial.println("ACK_SENT:" + mac);
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