#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>            
#include <LoRa.h>
#include <addr_lookup.h>

int sensorData[5];

uint8_t table_lol[4] = {
  0xDE,
  0xAD,
  0xBE,
  0xEF
};

const int csPin = 7;                       // LoRa radio chip select
const int resetPin = 6;                    // LoRa radio reset
const int irqPin = 1;                      // change for your board; must be a hardware interrupt pin

String outgoing;                           // outgoing message
uint8_t msgCount = 0;                      // count of outgoing messages
const uint8_t localAddress = 0xDE;         // address of this device
const uint8_t destination = 0xEF;          // destination to send to
bool confirmed = false;

uint32_t lastRecAddr = 0;
bool sendConfirm = false;

void sendMessage(String outgoing, uint8_t addr) {
  LoRa.beginPacket();                      // start packet
  LoRa.write(addr);
  LoRa.write(localAddress);
  LoRa.write(msgCount);                    // add message ID
  LoRa.write(outgoing.length());           // add payload length
  LoRa.print(outgoing);                    // add payload
  LoRa.endPacket();                        // finish packet and send it
  msgCount++;                              // increment message ID
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;             // if there's no packet, return

  uint8_t recipient = LoRa.read();         // read packet header uint8_ts:

  uint8_t sender = LoRa.read();            // sender address

  uint8_t incomingMsgId = LoRa.read();     // incoming msg ID
  uint8_t incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";                    // payload of packet

  while (LoRa.available()) {               // can't use readString() in callback, so
    incoming += (char)LoRa.read();         // add uint8_ts one by one
  }

  lastRecAddr = sender;
  sendConfirm = true;

  String alphabet = "abcde";

  int previousIndex = 0;
  for(int i = 0; i < 5; i++){
    int currentIndex = incoming.indexOf(alphabet[i]);

    String incoming_hold = "\0";

    for(int h = (previousIndex + ((bool)i)); h < currentIndex; h++){
      incoming_hold += incoming[h];
    };
    
    sensorData[i] = incoming_hold.toInt();
    incoming_hold = "\0";

    sensorData[i] += (5000000-(1000000*i));

    Serial.println((sensorData[i]) );

    previousIndex = incoming.indexOf(alphabet[i]);
    delay(200);
  };

  if (incomingLength != incoming.length()) {   // check length for error
    return;                                    // skip rest of function in case of error
  }

  
  if (!lookup_addr(table_lol, recipient) && recipient != 0xFF) {  // if the recipient isn't this device or broadcast,
    return;                                                       // skip rest of function in case of error
  }
}


void setup() {
  Serial.begin(115200);                        // initialize serial
  while (!Serial);

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(18, 23, 26);
  if (!LoRa.begin(433E6)) {                    // initialize ratio at 915 MHzs
    while (true);                              // if failed, do nothing
  }

  LoRa.onReceive(onReceive);
  LoRa.receive();
}

void loop() {
  LoRa.receive();
  if(sendConfirm) {
    sendMessage("CONFDREC", lastRecAddr);
    sendConfirm = false;
  }
  delay(1000);
}
