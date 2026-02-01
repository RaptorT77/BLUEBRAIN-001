/**
 * @file i2c_scanner.c
 * @brief I2C Bus Scanner para diagnosticar MPU6050
 *
 * Este componente escanea el bus I2C para detectar todos los dispositivos
 * conectados y ayudar a diagnosticar problemas de conexión.
 */

#include "i2c_scanner.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "I2C_SCANNER";

void i2c_scanner_scan(void) {
  ESP_LOGI(TAG, "====================================");
  ESP_LOGI(TAG, "Escaneando Bus I2C...");
  ESP_LOGI(TAG, "SDA: GPIO 6 (D4), SCL: GPIO 7 (D5)");
  ESP_LOGI(TAG, "====================================");

  int devices_found = 0;

  for (uint8_t addr = 1; addr < 127; addr++) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
      ESP_LOGI(TAG, "  -> Dispositivo encontrado en 0x%02X", addr);
      devices_found++;

      // Identificar dispositivos conocidos
      if (addr == 0x68 || addr == 0x69) {
        ESP_LOGI(TAG, "     (Probablemente MPU6050/MPU9250)");
      } else if (addr == 0x3C || addr == 0x3D) {
        ESP_LOGI(TAG, "     (Probablemente OLED SSD1306)");
      }
    }
  }

  ESP_LOGI(TAG, "====================================");
  if (devices_found == 0) {
    ESP_LOGW(TAG, "❌ NO se encontraron dispositivos I2C");
    ESP_LOGW(TAG, "Posibles causas:");
    ESP_LOGW(TAG, "  1. Pines SDA/SCL intercambiados");
    ESP_LOGW(TAG, "  2. Sin pull-up resistors (necesitas 4.7kΩ)");
    ESP_LOGW(TAG, "  3. Sensor sin alimentación (3.3V)");
    ESP_LOGW(TAG, "  4. Conexiones sueltas");
  } else {
    ESP_LOGI(TAG, "✅ Total: %d dispositivo(s) encontrado(s)", devices_found);
  }
  ESP_LOGI(TAG, "====================================");
}

esp_err_t i2c_scanner_test_mpu6050(uint8_t addr) {
  ESP_LOGI(TAG, "Probando MPU6050 en dirección 0x%02X...", addr);

  // Intentar leer el registro WHO_AM_I (0x75)
  uint8_t who_am_i_reg = 0x75;
  uint8_t who_am_i_val = 0;

  esp_err_t ret = i2c_master_write_read_device(
      I2C_NUM_0, addr, &who_am_i_reg, 1, &who_am_i_val, 1, pdMS_TO_TICKS(100));

  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "  WHO_AM_I = 0x%02X (esperado: 0x68)", who_am_i_val);
    if (who_am_i_val == 0x68 || who_am_i_val == 0x70) {
      ESP_LOGI(TAG, "  ✅ MPU6050 identificado correctamente!");
      return ESP_OK;
    } else {
      ESP_LOGW(TAG, "  ⚠️  Respuesta inesperada del sensor");
      return ESP_FAIL;
    }
  } else {
    ESP_LOGE(TAG, "  ❌ Error al leer MPU6050: %s", esp_err_to_name(ret));
    return ret;
  }
}
