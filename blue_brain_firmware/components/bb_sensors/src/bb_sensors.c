#include "bb_sensors.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_scanner.h"

static const char *TAG = "bb_sensors";

// --- DRIVER DS18B20 (1-Wire Bit-Bang) ---
// Adaptado del original main.c

static esp_err_t ds18b20_reset() {
  gpio_set_direction(BB_DS18B20_GPIO, GPIO_MODE_OUTPUT);
  gpio_set_level(BB_DS18B20_GPIO, 0);
  esp_rom_delay_us(480);
  gpio_set_direction(BB_DS18B20_GPIO, GPIO_MODE_INPUT);
  esp_rom_delay_us(70);
  int level = gpio_get_level(BB_DS18B20_GPIO);
  // ESP_LOGD(TAG, "DS18B20 Reset: Presence=%d (0=OK)", level);
  esp_rom_delay_us(410);
  return (level == 0) ? ESP_OK : ESP_FAIL;
}

static void ds18b20_write_byte(uint8_t data) {
  for (int i = 0; i < 8; i++) {
    gpio_set_direction(BB_DS18B20_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BB_DS18B20_GPIO, 0);
    esp_rom_delay_us((data & (1 << i)) ? 2 : 60);
    gpio_set_direction(BB_DS18B20_GPIO, GPIO_MODE_INPUT);
    esp_rom_delay_us((data & (1 << i)) ? 60 : 2);
  }
}

static uint8_t ds18b20_read_byte() {
  uint8_t data = 0;
  for (int i = 0; i < 8; i++) {
    gpio_set_direction(BB_DS18B20_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BB_DS18B20_GPIO, 0);
    esp_rom_delay_us(2);
    gpio_set_direction(BB_DS18B20_GPIO, GPIO_MODE_INPUT);
    esp_rom_delay_us(10);
    if (gpio_get_level(BB_DS18B20_GPIO))
      data |= (1 << i);
    esp_rom_delay_us(50);
  }
  return data;
}

float bb_sensors_get_temp(void) {
  // Step 1: Start Converson
  if (ds18b20_reset() != ESP_OK) {
    ESP_LOGW(TAG, "DS18B20: No presence pulse detected on conversion start");
    return -99.0f;
  }
  ds18b20_write_byte(0xCC); // Skip ROM
  ds18b20_write_byte(0x44); // Convert T

  // Wait for conversion (blocking)
  vTaskDelay(pdMS_TO_TICKS(750));

  // Step 2: Read Scratchpad
  if (ds18b20_reset() != ESP_OK) {
    ESP_LOGW(TAG, "DS18B20: No presence pulse detected on read");
    return -99.0f;
  }
  ds18b20_write_byte(0xCC); // Skip ROM
  ds18b20_write_byte(0xBE); // Read Scratchpad

  uint8_t low = ds18b20_read_byte();
  uint8_t high = ds18b20_read_byte();
  int16_t raw = (high << 8) | low;

  ESP_LOGI(TAG, "DS18B20 Raw: 0x%04X (L:%02X H:%02X) -> %.2f C", raw, low, high,
           (float)raw / 16.0f);

  return (float)raw / 16.0f;
}

// --- MPU6050 & I2C ---

esp_err_t bb_sensors_init(void) {
  // 1. Iniciar I2C
  i2c_config_t conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = BB_I2C_MASTER_SDA_IO,
      .scl_io_num = BB_I2C_MASTER_SCL_IO,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master.clk_speed = BB_I2C_MASTER_FREQ_HZ,
  };
  i2c_param_config(BB_I2C_MASTER_NUM, &conf);
  i2c_driver_install(BB_I2C_MASTER_NUM, conf.mode, 0, 0, 0);

  // DIAGNÓSTICO: Escanear bus I2C antes de configurar MPU6050
  ESP_LOGI(TAG, "=== DIAGNÓSTICO I2C ===");
  i2c_scanner_scan();

  // DIAGNÓSTICO: Probar MPU6050 específicamente
  esp_err_t mpu_test = i2c_scanner_test_mpu6050(BB_MPU6050_ADDR);
  if (mpu_test != ESP_OK) {
    ESP_LOGW(TAG, "Probando dirección alternativa 0x69...");
    i2c_scanner_test_mpu6050(0x69);
  }
  ESP_LOGI(TAG, "=======================");

  // 2. Inicializar MPU6050 (I2C)
  // Nota: Ya se inicializó el driver I2C arriba.
  // Aquí podemos poner lógica adicional si fuera necesaria.

  // 3. Inicializar ICM-42688-P (SPI)
  // Nota: Si el sensor no está conectado, fallará pero no detendremos el flujo
  // a menos que sea crítico. El usuario dijo "por ahora no lo usaremos".
  esp_err_t ret = icm42688_init(); // Assuming this is a new function
  if (ret != ESP_OK) {
    ESP_LOGW(TAG,
             "ICM-42688 Init Failed (Maybe not connected?) - Continuing...");
    // return ret; // No retornar error fatal
  }

  // 2. Configurar MPU6050
  uint8_t cmds[][2] = {
      {0x6B, 0x00}, // Despertar
      {0x1A, 0x00}, // DLPF: 260Hz
      {0x1C, 0x18}, // Rango: +/- 16g
      {0x19, 0x00}  // Sample Rate: 1kHz
  };

  bool mpu_ok = true;
  for (int i = 0; i < 4; i++) {
    esp_err_t ret = i2c_master_write_to_device(
        BB_I2C_MASTER_NUM, BB_MPU6050_ADDR, cmds[i], 2, 100);
    if (ret != ESP_OK) {
      ESP_LOGW(TAG, "MPU6050 no responde (cmd %d): %s - Hardware no conectado?",
               i, esp_err_to_name(ret));
      mpu_ok = false;
      break;
    }
  }

  if (mpu_ok) {
    ESP_LOGI(TAG, "Sensores Inicializados: I2C + MPU6050 OK");
  } else {
    ESP_LOGW(TAG, "Sensores Inicializados: I2C OK, MPU6050 NO DETECTADO");
    ESP_LOGW(TAG, "El firmware continuara sin sensores - Solo para pruebas!");
  }
  return ESP_OK;
}

void bb_sensors_read_accel_burst(uint8_t *raw_data, int len) {
  uint8_t reg = 0x3B;
  for (int i = 0; i < len; i++) {
    i2c_master_write_read_device(BB_I2C_MASTER_NUM, BB_MPU6050_ADDR, &reg, 1,
                                 &raw_data[i * 6], 6, 10);
    esp_rom_delay_us(1000);
  }
}

esp_err_t bb_sensors_read_accel_single(float *ax, float *ay, float *az) {
  uint8_t raw[6];
  uint8_t reg = 0x3B;
  esp_err_t ret = i2c_master_write_read_device(
      BB_I2C_MASTER_NUM, BB_MPU6050_ADDR, &reg, 1, raw, 6, 10);

  if (ret == ESP_OK) {
    *ax = (int16_t)((raw[0] << 8) | raw[1]) / BB_ACCEL_SENS_16G;
    *ay = (int16_t)((raw[2] << 8) | raw[3]) / BB_ACCEL_SENS_16G;
    *az = (int16_t)((raw[4] << 8) | raw[5]) / BB_ACCEL_SENS_16G;
  }
  return ret;
}
