#ifndef HARDWARE_DEFS_H
#define HARDWARE_DEFS_H

// XIAO ESP32-S3 Pin Definitions

// I2C (MPU6050, etc.)
#define PIN_I2C_SDA 5
#define PIN_I2C_SCL 6

// SPI (ST7789 Display)
#define PIN_SPI_MOSI 9
#define PIN_SPI_SCK 7
#define PIN_SPI_CS 3 // Chip Select for Display
#define PIN_SPI_DC 4 // Data/Command
#define PIN_SPI_RST 1 // Reset
// BL is often controlled too if needed

// OneWire (DS18B20)
#define PIN_ONE_WIRE 2 

// Other
#define PIN_BAT_VOLT 8 // ADC pin for battery voltage (check schematic/docs if it's A0/D0 etc)

#endif // HARDWARE_DEFS_H
