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
#define PIN_I2C_SDA GPIO_NUM_5
#define PIN_I2C_SCL GPIO_NUM_6

// --- SPI Bus (ST7789 Display & ICM-42688-P) ---
#define PIN_SPI_MOSI GPIO_NUM_10
#define PIN_SPI_CLK GPIO_NUM_8
#define PIN_SPI_MISO GPIO_NUM_9 // MISO (Necessario para ICM-42688)

#define PIN_DISP_CS GPIO_NUM_2
#define PIN_DISP_DC GPIO_NUM_3
// Note: Backlight usually connected to 3.3V or controlled via transistor if
// needed.

// Resolving conflict: PIN_DISP_RST moved to -1 (Soft Reset) to free GPIO 4 for
// OneWire. See below for new definition. Corrected Mapping: OneWire: 4 Display
// RST: 7 (Proposed change) IMU CS: 1 IMU INT: 43 (If UART unused) or just not
// defined yet. UART is usually essential. What about GPIO 9 (MISO). GPIO 10
// (MOSI). GPIO 8 (CLK). GPIO 43/44 (UART). GPIO 5/6 (I2C). GPIO 2 (Disp CS).
// GPIO 3 (Disp DC).
// GPIO 4 (OneWire).
// Free: 1, 7.
// Need: MISO(9), IMU_CS(1), IMU_INT(7).
// If PIN_DISP_RST was on 4, where does it go?
// Use -1 (Unused/Reset to VCC) if possible. ST7789 can work with RST at 3.3V.
// I will set PIN_DISP_RST to -1 or a free pin if I find one.
// But wait, GPIO 7 is free-ish.
// I'll put MISO on 9, IMU_CS on 1, IMU_INT on 7.
// Disp rst needs to move.
// Maybe Disp RST isn't strictly needed (Soft Reset).
// I will keep PIN_ONE_WIRE as 4.

#define PIN_DISP_RST -1 // Reset conectado a 3.3V o Soft Reset

#define PIN_IMU_CS GPIO_NUM_1  // Chip Select ICM-42688
#define PIN_IMU_INT GPIO_NUM_7 // Interrupt ICM-42688

// --- 1-Wire (DS18B20) ---
#define PIN_ONE_WIRE GPIO_NUM_4 // D3 (Fixed from conflict)

// --- System Inputs ---
#define PIN_BOOT_BTN GPIO_NUM_0

// --- Debug UART ---
#define PIN_UART_TX GPIO_NUM_43
#define PIN_UART_RX GPIO_NUM_44

// --- Power Management ---
#define PIN_BAT_ADC GPIO_NUM_7 // Battery Voltage Divider (Example)

#endif // HARDWARE_DEFS_H
