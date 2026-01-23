/**
 * @file main.c
 * @brief Main Orchestrator for Blue Brain Firmware - Real Sensor Mode (3-Axis + Temp)
 * @author Blue Brain Team / Tony
 */

#include <stdio.h>
#include <string.h>
#include <math.h> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "esp_random.h"
#include "driver/i2c.h"
#include "esp_rom_sys.h" // Necesario para delays de microsegundos exactos

// --- CONFIGURACIÓN HARDWARE (XIAO ESP32-S3) ---
#define I2C_MASTER_SCL_IO           6     // Pin D5
#define I2C_MASTER_SDA_IO           5     // Pin D4
#define DS18B20_GPIO                4     // Pin D3 (GPIO 4)
#define I2C_MASTER_NUM              0     
#define I2C_MASTER_FREQ_HZ          100000 

// --- REGISTROS MPU6050 ---
#define MPU6050_PWR_MGMT_1          0x6B  
#define MPU6050_ACCEL_XOUT_H        0x3B  
#define MPU6050_WHO_AM_I            0x75  
#define ACCEL_SENSITIVITY           16384.0f // Sensibilidad para +/- 2g

// --- Constants & Macros ---
#define TAG "BLUE_BRAIN"
#define PIN_BOOT_BTN                0

// --- Data Structures ---
typedef struct __attribute__((packed)) {
    uint8_t magic;           
    uint8_t device_mac[6];   
    uint32_t packet_id;      
    uint32_t timestamp;      
    float accel_x;           // Agregado: Eje X
    float accel_y;           // Agregado: Eje Y
    float accel_z;           // Agregado: Eje Z
    float temp_c;            
    uint8_t battery_pct;     
    uint8_t ai_status;       
    uint16_t checksum;       
} bb_packet_t;

// --- Funciones DS18B20 (1-Wire) ---

static esp_err_t ds18b20_reset() {
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DS18B20_GPIO, 0);
    esp_rom_delay_us(480);
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_INPUT);
    esp_rom_delay_us(70);
    int presence = gpio_get_level(DS18B20_GPIO);
    esp_rom_delay_us(410);
    return (presence == 0) ? ESP_OK : ESP_FAIL;
}

static void ds18b20_write_byte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        gpio_set_direction(DS18B20_GPIO, GPIO_MODE_OUTPUT);
        gpio_set_level(DS18B20_GPIO, 0);
        if (data & (1 << i)) {
            esp_rom_delay_us(2);
            gpio_set_direction(DS18B20_GPIO, GPIO_MODE_INPUT);
            esp_rom_delay_us(60);
        } else {
            esp_rom_delay_us(60);
            gpio_set_direction(DS18B20_GPIO, GPIO_MODE_INPUT);
            esp_rom_delay_us(2);
        }
    }
}

static uint8_t ds18b20_read_byte() {
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        gpio_set_direction(DS18B20_GPIO, GPIO_MODE_OUTPUT);
        gpio_set_level(DS18B20_GPIO, 0);
        esp_rom_delay_us(2);
        gpio_set_direction(DS18B20_GPIO, GPIO_MODE_INPUT);
        esp_rom_delay_us(10);
        if (gpio_get_level(DS18B20_GPIO)) data |= (1 << i);
        esp_rom_delay_us(50);
    }
    return data;
}

static float ds18b20_read_temp() {
    if (ds18b20_reset() != ESP_OK) return -99.0f;
    ds18b20_write_byte(0xCC); // Skip ROM
    ds18b20_write_byte(0x44); // Iniciar conversión
    vTaskDelay(pdMS_TO_TICKS(750)); // Esperar conversión
    
    ds18b20_reset();
    ds18b20_write_byte(0xCC);
    ds18b20_write_byte(0xBE); // Leer Scratchpad
    
    uint8_t low = ds18b20_read_byte();
    uint8_t high = ds18b20_read_byte();
    int16_t raw = (high << 8) | low;
    return (float)raw / 16.0f;
}

// --- Funciones MPU6050 ---

static esp_err_t mpu6050_init(uint8_t addr) {
    uint8_t data[] = {MPU6050_PWR_MGMT_1, 0x00}; 
    return i2c_master_write_to_device(I2C_MASTER_NUM, addr, data, sizeof(data), 1000 / portTICK_PERIOD_MS);
}

static void mpu6050_read_3axis(uint8_t addr, float *x, float *y, float *z) {
    uint8_t raw_data[6];
    uint8_t reg = MPU6050_ACCEL_XOUT_H;
    
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, addr, &reg, 1, raw_data, 6, 1000 / portTICK_PERIOD_MS);

    if (ret == ESP_OK) {
        int16_t ix = (raw_data[0] << 8) | raw_data[1];
        int16_t iy = (raw_data[2] << 8) | raw_data[3];
        int16_t iz = (raw_data[4] << 8) | raw_data[5];

        *x = (float)ix / ACCEL_SENSITIVITY;
        *y = (float)iy / ACCEL_SENSITIVITY;
        *z = (float)iz / ACCEL_SENSITIVITY;
    }
}

// --- Task Definitions ---

void Task_Acquisition(void *pvParameters) {
    ESP_LOGI("TASK_ACQ", "Iniciando Adquisición Real...");
    
    uint8_t real_addr = 0x68; // Dirección detectada previamente
    mpu6050_init(real_addr);

    float ax, ay, az, temp;

    while (1) {
        // 1. Lectura REAL de Acelerómetro (3 Ejes)
        mpu6050_read_3axis(real_addr, &ax, &ay, &az);

        // 2. Lectura REAL de Temperatura
        temp = ds18b20_read_temp();

        // 3. Log Consolidado (Formato Profesional G)
        if (temp == -99.0f) {
            ESP_LOGE("TASK_ACQ", "ERROR SENSOR TEMP: Revisa Pin D3 y Resistencia 4.7k");
        } else {
            ESP_LOGI("TASK_ACQ", "TELEMETRÍA -> Accel[G]: X:%.2f Y:%.2f Z:%.2f | Temp: %.2f °C", 
                     ax, ay, az, temp);
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Adquisición cada 1 segundo
    }
}

void Task_Comms(void *pvParameters) {
    while (1) {
        ESP_LOGI("TASK_COMMS", "Esperando datos en cola...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void run_measurement_mode(void) {
    xTaskCreatePinnedToCore(Task_Acquisition, "Task_Acq", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(Task_Comms, "Task_Comms", 4096, NULL, 3, NULL, 0);
    ESP_LOGI(TAG, "Measurement Mode Started on Dual Core");
}

void app_main(void) {
    nvs_flash_init();
    
    // Configurar I2C
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &i2c_conf);
    i2c_driver_install(I2C_MASTER_NUM, i2c_conf.mode, 0, 0, 0);

    run_measurement_mode();
}