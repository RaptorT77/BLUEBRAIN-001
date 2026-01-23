/**
 * @file hardware_defs.h
 * @brief Hardware Abstraction Layer (HAL) Pin Definitions
 * @board Seeed Studio XIAO ESP32-S3
 * @project Blue Brain Sensor Node (Phase 1)
 */

#ifndef HARDWARE_DEFS_H
#define HARDWARE_DEFS_H

#include "driver/gpio.h"

// --- I2C Bus (MPU6050) ---
#define PIN_I2C_SDA      GPIO_NUM_6
#define PIN_I2C_SCL      GPIO_NUM_7

// --- SPI Bus (ST7789 Display) ---
#define PIN_SPI_MOSI     GPIO_NUM_10
#define PIN_SPI_CLK      GPIO_NUM_8
#define PIN_DISP_CS      GPIO_NUM_2
#define PIN_DISP_DC      GPIO_NUM_3
#define PIN_DISP_RST     GPIO_NUM_4
// Note: Backlight usually connected to 3.3V or controlled via transistor if needed.

// --- 1-Wire (DS18B20) ---
#define PIN_ONE_WIRE     GPIO_NUM_5

// --- System Inputs ---
#define PIN_BOOT_BTN     GPIO_NUM_0

// --- Debug UART ---
#define PIN_UART_TX      GPIO_NUM_43
#define PIN_UART_RX      GPIO_NUM_44

#endif // HARDWARE_DEFS_H
