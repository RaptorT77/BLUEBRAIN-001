#ifndef I2C_SCANNER_H
#define I2C_SCANNER_H

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief Escanea el bus I2C y lista todos los dispositivos encontrados
 */
void i2c_scanner_scan(void);

/**
 * @brief Prueba específicamente el MPU6050 en una dirección
 * @param addr Dirección I2C a probar (típicamente 0x68 o 0x69)
 * @return ESP_OK si el sensor responde correctamente
 */
esp_err_t i2c_scanner_test_mpu6050(uint8_t addr);

#endif // I2C_SCANNER_H
