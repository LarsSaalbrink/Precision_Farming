#include <Arduino.h>
#include <Wire.h>
#include <hp_BH1750.h>  //Light intensity sensor
#include <SHT2x.h>      //Air temperature & humidity sensor

/////Settings for frequency of performing measurements & transmitting data/////
#define LoRaPacketsPerDay 3
#define MeasurementsPerPacket 4

//Automatic calculations
#define DeepSleepTime_Hrs  (24/(LoRaPacketsPerDay*MeasurementsPerPacket))
#define DeepSleepTime_uS (DeepSleepTime_Hrs*3600000000)
///////////////////////////////////////////////////////////////////////////////

//Create sensor objects
hp_BH1750 BH1750;  //Light intensity sensor
SHT2x SHT;         //Air temperature & humidity sensor

//Struct containing a single set of measurements from all our sensors
struct SensorData
{
  float Lux = 1;
  float AirTemp = 1;
  float AirHum = 1;
  uint8_t SoilHumIO34 = 1;
  uint8_t SoilHumIO35 = 1;
};

//Array containing several sets of sensor measurements
//Size defined by amount of sensor reading sets sent per LoRa packet
RTC_DATA_ATTR SensorData ReadingsPackage[MeasurementsPerPacket];

//Declaration of functions handling the sensor data package
SensorData GetSensorData(void);
SensorData ClearSensorData(void);
uint8_t ReadSoilHum(uint8_t IO_pin);

void setup() {
  /////Sensor initialization/////
  BH1750.begin(BH1750_TO_GROUND);  //Use i2c address of a light sensor module with ADDR pin grounden
  SHT.begin();                     //Connect to air temp. & hum. sensor using default i2c address
  ///////////////////////////////

  //Read sensors
  ReadingsPackage[0] = GetSensorData();

  //---Direct connection to PC UI, no LoRa---
  Serial.begin(115200);

  Serial.println(ReadingsPackage[0].Lux + 5000000);
  Serial.println(ReadingsPackage[0].AirTemp + 4000000);
  Serial.println(ReadingsPackage[0].AirHum + 3000000);
  Serial.println(float(ReadingsPackage[0].SoilHumIO34) + 2000000);
  Serial.println(float(ReadingsPackage[0].SoilHumIO35) + 1000000);
  //---Direct connection to PC UI, no LoRa---

  ReadingsPackage[0] = ClearSensorData();

  //Go into deep sleep until next measurement needs to be taken
  esp_sleep_enable_timer_wakeup(1000000);        //For debugging, sleep for 1 second
  esp_deep_sleep_start();
}


void loop() {
  //Loop() is never reached, esp goes into sleep mode before this point
}


//Function for reading all sensors once
SensorData GetSensorData() {
  SensorData QuickStorage;

  BH1750.start();
  QuickStorage.Lux = BH1750.getLux();            //Lux

  SHT.read();
  QuickStorage.AirTemp = SHT.getTemperature();   //Degree C
  QuickStorage.AirHum = SHT.getHumidity();       //Relative humidity %

  QuickStorage.SoilHumIO34 = ReadSoilHum(34);    //Not proper unit
  QuickStorage.SoilHumIO35 = ReadSoilHum(35);    //Not proper unit

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

  if(ADC_reading < 1400){                        //Wet soil
    return 1;
  }
  else if(ADC_reading > 2600){                   //Dry soil
    return 3;
  }
  else{                                          //Damp soil
    return 2;
  }
}