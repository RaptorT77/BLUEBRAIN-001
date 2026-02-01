#ifndef BB_SENSORS_H
#define BB_SENSORS_H

#include "esp_err.h"
#include "icm42688.h"

// --- Configuración Hardware ---
// Pines definidos para XIAO ESP32-S3
#define BB_I2C_MASTER_SCL_IO 6 // Pin D5 (GPIO 6)
#define BB_I2C_MASTER_SDA_IO 5 // Pin D4 (GPIO 5)
#define BB_DS18B20_GPIO 4      // Pin D3 (GPIO 4)
#define BB_I2C_MASTER_NUM 0
#define BB_I2C_MASTER_FREQ_HZ 400000
#define BB_MPU6050_ADDR 0x68
#define BB_ACCEL_SENS_16G 2048.0f

// --- Funciones Públicas ---

/**
 * @brief Inicializa el bus I2C y el MPU6050
 * @return ESP_OK si todo es correcto
 */
esp_err_t bb_sensors_init(void);

/**
 * @brief Lee una ráfaga de datos crudos del MPU6050 para análisis
 * @param buffer Buffer donde guardar los datos (X, Y, Z int16_t)
 * @param samples Número de muestras a leer
 */
void bb_sensors_read_accel_burst(uint8_t *raw_data, int len);

/**
 * @brief Lee un solo sample de aceleración (usado internamente o para debug)
 * @param ax Puntero a float para X
 * @param ay Puntero a float para Y
 * @param az Puntero a float para Z
 */
esp_err_t bb_sensors_read_accel_single(float *ax, float *ay, float *az);

/**
 * @brief Obtiene la temperatura del sensor DS18B20
 * @return Temperatura en grados Celsius
 */
float bb_sensors_get_temp(void);

#endif // BB_SENSORS_H
