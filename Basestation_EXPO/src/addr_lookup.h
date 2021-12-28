#include <Arduino.h>
#include <stdint.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Wire.h>
#include <SPI.h>            
#include <LoRa.h>

bool lookup_addr(uint8_t table[], uint8_t addr);
