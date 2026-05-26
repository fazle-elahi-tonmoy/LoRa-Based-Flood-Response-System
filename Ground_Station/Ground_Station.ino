#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

#define LORA_NSS 5
#define LORA_RST 16
#define LORA_DIO0 4

void setup() {
  Serial.begin(115200);

  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);

  while (!LoRa.begin(433E6)) {
    Serial.println("LoRa Init Failed");
    delay(1000);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.enableCrc();

  Serial.println("Ground Station Ready");
}

void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    String incoming = LoRa.readString();
    Serial.println(incoming);   // 👉 send to WebApp via UART

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, incoming);
    if (err) return;

    String type = doc["type"];

    // ✅ Send ACK for client requests
    if (type == "Rescue" || type == "Supply") {
      String id = doc["id"];
      
      StaticJsonDocument<128> ack;
      ack["type"] = "ack";
      ack["id"] = id;

      String ackPacket;
      serializeJson(ack, ackPacket);

      delay(100); // required delay

      LoRa.beginPacket();
      LoRa.print(ackPacket);
      LoRa.endPacket();
      LoRa.receive();
    }
  }
}