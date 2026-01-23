/**
 * @file main.c
 * @brief Main Orchestrator for Blue Brain Firmware
 * @author Blue Brain Team
 */

#include <stdio.h>
#include <string.h>
#include <math.h>  // Necesario para sqrt()
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "hardware_defs.h"
#include "esp_random.h"
#include "driver/i2c.h"

// --- CONFIGURACIÓN MPU6050 (XIAO ESP32-S3) ---
#define I2C_MASTER_SCL_IO           7     // Pin D5 (GPIO 7) - RSTAURADO SEGUN SPEC
#define I2C_MASTER_SDA_IO           6     // Pin D4 (GPIO 6)
#define I2C_MASTER_NUM              0     // Puerto I2C 0
#define I2C_MASTER_FREQ_HZ          100000 // 100kHz (Estándar)
// La dirección ya no es fija, la detectaremos dinámicamente
#define MPU6050_PWR_MGMT_1          0x6B  // Registro de energía
#define MPU6050_ACCEL_XOUT_H        0x3B  // Registro de datos (Inicio)
#define MPU6050_WHO_AM_I            0x75  // Registro de Identificación

// --- Constants & Macros ---
#define TAG "MAIN_ORCHESTRATOR"
#define CONFIG_MODE_HOLD_SEC 3

// --- Data Structures (from Spec) ---
typedef struct __attribute__((packed)) {
    uint8_t magic;           // 0xBB
    uint8_t device_mac[6];   // Sender MAC
    uint32_t packet_id;      // Counter
    uint32_t timestamp;      // RTC time
    float vib_rms;           // RMS Velocity
    float vib_peak;          // Peak Accel
    float temp_c;            // Temperature
    uint8_t battery_pct;     // Battery %
    uint8_t ai_status;       // 0=OK, 1=WARN, 2=ALARM
    uint16_t checksum;       // CRC16
} bb_packet_t;

// --- Global Handles ---
TaskHandle_t xTaskAcquisition = NULL;
TaskHandle_t xTaskComms = NULL;
QueueHandle_t xQueuePacket = NULL;

// --- Task Definitions ---

/**
 * @brief Busca el sensor MPU6050 en las direcciones 0x68 y 0x69
 * NOTA: Asume que el driver I2C ya fue instalado en app_main()
 * @return La dirección encontrada (0x68 o 0x69) o 0 si falla.
 */
static uint8_t find_mpu6050_address() {
    uint8_t addresses[] = {0x68, 0x69}; // Las dos posibles direcciones
    uint8_t who_am_i;

    // Escanear direcciones
    for (int i = 0; i < 2; i++) {
        esp_err_t ret = i2c_master_write_read_device(
            I2C_MASTER_NUM, 
            addresses[i], 
            (uint8_t[]){MPU6050_WHO_AM_I}, // Leemos registro WHO_AM_I
            1, 
            &who_am_i, 
            1, 
            100 / portTICK_PERIOD_MS
        );

        // El MPU6050 siempre responde 0x68 en este registro, sin importar su dirección I2C
        if (ret == ESP_OK && who_am_i == 0x68) { 
            ESP_LOGW("MPU_DETECT", "¡Sensor ENCONTRADO en dirección: 0x%02X!", addresses[i]);
            return addresses[i];
        } else {
             ESP_LOGI("MPU_DETECT", "No encontrado en 0x%02X (Err: %d, ID: 0x%02X)", addresses[i], ret, who_am_i);
        }
    }
    return 0; // No encontrado
}

// Inicializar (Despertar) al sensor usando la dirección detectada
static esp_err_t mpu6050_init(uint8_t addr) {
    if (addr == 0) return ESP_FAIL;

    // Despertar al sensor (sacarlo del modo sleep escribiendo 0 en PWR_MGMT_1)
    uint8_t data[] = {MPU6050_PWR_MGMT_1, 0x00}; 
    return i2c_master_write_to_device(I2C_MASTER_NUM, addr, data, sizeof(data), 1000 / portTICK_PERIOD_MS);
}

// Leer Aceleración (X, Y, Z) usando la dirección detectada
static void mpu6050_read_accel(uint8_t addr, float *vib_val) {
    if (addr == 0) return;

    uint8_t raw_data[6];
    // Leer 6 bytes comenzando desde ACCEL_XOUT_H
    uint8_t reg = MPU6050_ACCEL_XOUT_H;
    
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, addr, &reg, 1, raw_data, 6, 1000 / portTICK_PERIOD_MS);

    if (ret == ESP_OK) {
        // Convertir bytes a enteros con signo (16 bits)
        int16_t acc_x = (raw_data[0] << 8) | raw_data[1];
        int16_t acc_y = (raw_data[2] << 8) | raw_data[3];
        int16_t acc_z = (raw_data[4] << 8) | raw_data[5];

        // Calcular magnitud del vector (Vibración total) y convertir a G (aprox /16384.0)
        float x_g = acc_x / 16384.0f;
        float y_g = acc_y / 16384.0f;
        float z_g = acc_z / 16384.0f;
        
        // Calculamos vibración RMS aproximada (Magnitud del vector)
        *vib_val = sqrt(x_g*x_g + y_g*y_g + z_g*z_g);
    } else {
        ESP_LOGE("MPU", "Fallo lectura sensor en 0x%02X", addr);
    }
}


/**
 * @brief Acquisition Task (Core 1 - High Priority)
 * Reads MPU6050 sample buffers and DS18B20 temperature.
 */
void Task_Acquisition(void *pvParameters) {
    ESP_LOGI("TASK_ACQ", "Iniciando Autodetección de Sensor...");
    
    // 1. Encontrar la dirección real (0x68 o 0x69)
    uint8_t real_addr = find_mpu6050_address();
    
    if (real_addr != 0) {
        // 2. Inicializar con la dirección encontrada
        if (mpu6050_init(real_addr) == ESP_OK) {
            ESP_LOGI("TASK_ACQ", "MPU6050 Inicializado y Despierto en 0x%02X", real_addr);
        } else {
            ESP_LOGE("TASK_ACQ", "Fallo al despertar el sensor");
        }
    } else {
        ESP_LOGE("TASK_ACQ", "¡ERROR FATAL! Sensor no detectado. Revisa cables SDA/SCL.");
    }

    float temp_c = 25.0f; // (Aún simulado hasta que integremos DS18B20)
    float vib_rms = 0.0f;

    while (1) {
        if (real_addr != 0) {
            // 2. Leemos la vibración REAL usando la dirección correcta
            mpu6050_read_accel(real_addr, &vib_rms);
            ESP_LOGI("TASK_ACQ", "SENSOR REAL -> Vib: %.2f G (Dir: 0x%02X) | Temp: %.2f C", vib_rms, real_addr, temp_c);
        } else {
            // Si falló al inicio, reintentamos buscar cada 2 segundos
            ESP_LOGW("TASK_ACQ", "Buscando sensor...");
            real_addr = find_mpu6050_address();
            if(real_addr != 0) mpu6050_init(real_addr);
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        // Simulamos temperatura por ahora (siguiente paso: DS18B20)
        temp_c += 0.1f; if(temp_c > 30) temp_c = 25;

        vTaskDelay(pdMS_TO_TICKS(500)); // Lectura cada 0.5 segundos
    }
}

/**
 * @brief Communications Task (Core 0 - Low Priority)
 * Handles WiFi/ESP-NOW transmission.
 */
void Task_Comms(void *pvParameters) {
    ESP_LOGI("TASK_COMMS", "Task Started on Core %d", xPortGetCoreID());
    
    while (1) {
        // Placeholder Logic
        ESP_LOGI("TASK_COMMS", "Esperando datos en cola...");
        
        // Simulating packet processing
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// --- Mode Functions ---

void run_config_mode(void) {
    ESP_LOGI(TAG, "--------------------------------");
    ESP_LOGI(TAG, ">>> ENTRANDO A MODO CONFIG <<<");
    ESP_LOGI(TAG, "AP: BlueBrain_Setup | IP: 192.168.4.1");
    ESP_LOGI(TAG, "--------------------------------");
    
    // Future: Start WiFi AP, WebServer, Captive Portal
    // bb_web_ui_start(); 
    
    while(1) {
        // Blink LED or Maintain Watchdog
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void run_measurement_mode(void) {
    ESP_LOGI(TAG, "Starting Measurement Mode...");

    // 1. Create Queue
    xQueuePacket = xQueueCreate(5, sizeof(bb_packet_t));
    if (xQueuePacket == NULL) {
        ESP_LOGE(TAG, "Error creating Packet Queue!");
        return;
    }

    // 2. Create Tasks
    // Task: Acquisition -> Core 1, Priority 5 (High)
    BaseType_t ret_acq = xTaskCreatePinnedToCore(
        Task_Acquisition,
        "Task_Acq",
        4096,
        NULL,
        5,
        &xTaskAcquisition,
        1
    );

    if (ret_acq != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Acquisition Task");
    }

    // Task: Comms -> Core 0, Priority 3 (Lower)
    BaseType_t ret_comms = xTaskCreatePinnedToCore(
        Task_Comms,
        "Task_Comms",
        4096,
        NULL,
        3,
        &xTaskComms,
        0
    );

    if (ret_comms != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Comms Task");
    }
    
    ESP_LOGI(TAG, "System Running in Multi-Core Mode");
}

// --- Main Entry Point ---

void app_main(void) {
    // 1. Initialize NVS (Non-Volatile Storage)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Check Reset Reason
    esp_reset_reason_t reason = esp_reset_reason();
    ESP_LOGI(TAG, "Boot Reason: %d", reason);

    // 3. Configure Input GPIO (BOOT Button)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_BOOT_BTN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, // Internal pull-up often needed for boot button
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Short delay to stabilize voltage
    vTaskDelay(pdMS_TO_TICKS(100));

    // 4. Initialize I2C Driver (SOLO UNA VEZ)
    ESP_LOGI(TAG, "Inicializando bus I2C (SDA: GPIO%d, SCL: GPIO%d)", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, i2c_conf.mode, 0, 0, 0));

    ESP_LOGI(TAG, "Bus I2C instalado correctamente");

    // --- I2C SCANNER (DEBUG) ---
    ESP_LOGW(TAG, "Iniciando ESCANEO I2C...");
    int devices_found = 0;
    for (int i = 1; i < 127; i++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 50 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            ESP_LOGW(TAG, "-> Dispositivo encontrado en: 0x%02X", i);
            devices_found++;
        }
    }
    if (devices_found == 0) ESP_LOGE(TAG, "¡NO SE ENCONTRARON DISPOSITIVOS I2C! Revisa cableado (SDA/SCL) y alimentación.");
    else ESP_LOGI(TAG, "Escaneo completado. Dispositivos: %d", devices_found);
    // ---------------------------

    // 5. Logic: Config vs Measurement
    int boot_btn_state = gpio_get_level(PIN_BOOT_BTN);
    
    // XIAO BOOT Button is typically Active Low (0 = Pressed)
    // Check if button is pressed
    if (boot_btn_state == 0) {
        // Debounce / Hold Check could happen here
        ESP_LOGI(TAG, "Boot Button Pressed detected.");
        run_config_mode();
    } else {
        run_measurement_mode();
    }
}