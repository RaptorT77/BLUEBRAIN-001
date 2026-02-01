/**
 * @file icm42688.h
 * @brief Driver simplificado para IMU ICM-42688-P (SPI)
 */

#ifndef ICM42688_H
#define ICM42688_H

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"

// Registros Clave
#define ICM42688_REG_DEVICE_CONFIG 0x11
#define ICM42688_REG_DRIVE_CONFIG 0x13
#define ICM42688_REG_INT_CONFIG 0x14
#define ICM42688_REG_PWR_MGMT0 0x4E
#define ICM42688_REG_WHO_AM_I 0x75

// Valor esperado en WHO_AM_I
#define ICM42688_WHO_AM_I_VAL 0x47

// Estructura del driver
typedef struct {
  spi_device_handle_t spi_handle;
  gpio_num_t cs_pin;
  gpio_num_t int_pin;
} icm42688_dev_t;

/**
 * @brief Inicializa el bus SPI y configura el ICM42688
 * @return ESP_OK si el sensor responde correctamente (WHO_AM_I)
 */
esp_err_t icm42688_init(void);

/**
 * @brief Lee el registro WHO_AM_I
 */
uint8_t icm42688_read_whoami(void);

/**
 * @brief Inicia una lectura DMA asíncrona (Non-Blocking)
 * @param buffer Puntero a memoria DMA-capable
 * @param len Longitud de datos a leer
 * @return ESP_OK si se encoló correctamente
 */
esp_err_t icm42688_start_dma_read(uint8_t *buffer, size_t len);

/**
 * @brief Espera a que termine la transferencia DMA
 * @return ESP_OK al finalizar
 */
esp_err_t icm42688_wait_dma_read(void);

#endif // ICM42688_H
