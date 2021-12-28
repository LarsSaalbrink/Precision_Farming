#include <Arduino.h>
#include <Wire.h>
#include <stdio.h>
#include <hp_BH1750.h>  //Light intensity sensor
#include <SHT2x.h>      //Air temperature & humidity sensor
#include <SPI.h>              
#include <LoRa.h>

//LoRa related   /////////////////////////////////////////////////////////////
String outgoing;                                      // outgoing message
uint8_t msgCount = 0;                                 // count of outgoing messages
const uint8_t localAddress = 0xEF;                    // address of this device
const uint8_t destination = 0xDE;                     // destination to send to
bool confirmed = false;
long lastSendTime = 0;                                // last send time
int interval = 2000;                                  // interval between sends

void sendMessage(String outgoing, uint8_t addr) {
  LoRa.beginPacket();                                 // start packet
  LoRa.write(addr);
  LoRa.write(localAddress);
  LoRa.write(msgCount);                               // add message ID
  LoRa.write(outgoing.length());                      // add payload length
  LoRa.print(outgoing);                               // add payload
  LoRa.endPacket();                                   // finish packet and send it
  msgCount++;                                         // increment message ID
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;                        // if there's no packet, return

  uint8_t recipient = LoRa.read();                    // read packet header uint8_ts:

  uint8_t sender = LoRa.read();                       // sender address

  uint8_t incomingMsgId = LoRa.read();                // incoming msg ID
  uint8_t incomingLength = LoRa.read();               // incoming msg length

  String incoming = "";                               // payload of packet

  while (LoRa.available()) {                          // can't use readString() in callback, so
    incoming += (char)LoRa.read();                    // add uint8_ts one by one
  }

  if (incomingLength != incoming.length()) {          // check length for error
    return;                                           // skip rest of function
  }

  if (recipient != localAddress && sender != 0xFF) {  // if the recipient isn't this device or broadcast,
    return;                                           // skip rest of function
  }

  if(incoming == String("CONFDREC")) {
    confirmed = true;
  }
}
//LoRa related   /////////////////////////////////////////////////////////////

//Sensor related /////////////////////////////////////////////////////////////
#define LoRaPacketsPerDay 3
#define MeasurementsPerPacket 4

//Automatic calculations
#define DeepSleepTime_Hrs  (24/(LoRaPacketsPerDay*MeasurementsPerPacket))
#define DeepSleepTime_uS (DeepSleepTime_Hrs*3600000000)

//Create sensor objects
hp_BH1750 BH1750;  //Light intensity sensor
SHT2x SHT;         //Air temperature & humidity sensor

//Struct containing a single set of measurements from all our sensors
struct SensorData
{
  float Lux = 7000;
  float AirTemp = 60;
  float AirHum = 52;
  uint8_t SoilHumIO34 = 1;
  uint8_t SoilHumIO35 = 2;
};

//Array containing several sets of sensor measurements
//Size defined by amount of sensor reading sets sent per LoRa packet
RTC_DATA_ATTR SensorData ReadingsPackage[MeasurementsPerPacket];

//Declaration of functions handling the sensor data package
SensorData GetSensorData(void);
SensorData ClearSensorData(void);
uint8_t ReadSoilHum(uint8_t IO_pin);
//Sensor related /////////////////////////////////////////////////////////////


void setup() {
  /////Sensor initialization/////
  BH1750.begin(BH1750_TO_GROUND);               //Use i2c address of a light sensor module with ADDR pin grounden
  SHT.begin();                                  //Connect to air temp. & hum. sensor using default i2c address
  ///////////////////////////////

  //-----LoRa Init Section-----
  LoRa.setPins(18, 23, 26);                     // override the default CS, reset, and IRQ pins
  if (!LoRa.begin(433E6)) {                     // initialize ratio at 433 MHz
    while (true);                               // if failed, do nothing
  }

  LoRa.onReceive(onReceive);
  LoRa.receive();
  //-----LoRa Init Section-----

  ReadingsPackage[0] = GetSensorData();         //Read sensors
  delay(100);

  String message = "ERROR                                                   ";   
  sprintf(&message[0], "%da%db%dc%dd%de", ((int)(floor(ReadingsPackage[0].Lux))), ((int)(floor(ReadingsPackage[0].AirTemp))), ((int)(floor(ReadingsPackage[0].AirHum))), ReadingsPackage[0].SoilHumIO34, ReadingsPackage[0].SoilHumIO35);

    while(confirmed == false) {
      sendMessage(message, destination);
      LoRa.receive();
      delay(1000);
    }
    confirmed = false;

  ReadingsPackage[0] = ClearSensorData();

  LoRa.end();                                   //Turn off radio module
  delay(100);

  esp_sleep_enable_timer_wakeup(1000000);       //For debugging, sleep for 1 second
  esp_deep_sleep_start();
}

void loop() {
  //Program goes into deep sleep before reaching loop()
}


//Function for reading all sensors once
SensorData GetSensorData() {
  SensorData QuickStorage;

  BH1750.start();
  QuickStorage.Lux = BH1750.getLux();           //Lux

  SHT.read();
  delay(90);                                    //Datasheet specifies that reading can take up to 85 ms
  QuickStorage.AirTemp = SHT.getTemperature();  //Degree C
  QuickStorage.AirHum = SHT.getHumidity();      //Relative humidity %

  QuickStorage.SoilHumIO34 = ReadSoilHum(34);   //Not proper unit
  QuickStorage.SoilHumIO35 = ReadSoilHum(35);   //Not proper unit

  return QuickStorage;
}

//Function resetting readings
SensorData ClearSensorData() {
  SensorData QuickStorage;

  QuickStorage.Lux = 1;          
  QuickStorage.AirTemp = 1; 
  QuickStorage.AirHum = 1;      
  QuickStorage.SoilHumIO34 = 1;    
  QuickStorage.SoilHumIO35 = 1;    
  return QuickStorage;
}

uint8_t ReadSoilHum(uint8_t IO_pin){
  uint16_t ADC_reading = analogRead(IO_pin);

  if(ADC_reading < 1400){                       //Wet soil
    return 1;
  }
  else if(ADC_reading > 2600){                  //Dry soil
    return 3;
  }
  else{                                         //Damp soil
    return 2;
  }
}
