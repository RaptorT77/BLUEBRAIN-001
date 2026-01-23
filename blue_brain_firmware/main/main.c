/**
 * @file main.c
 * @brief Main Orchestrator for Blue Brain Firmware
 * @author Blue Brain Team
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "hardware_defs.h"

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
 * @brief Acquisition Task (Core 1 - High Priority)
 * Reads MPU6050 sample buffers and DS18B20 temperature.
 */
void Task_Acquisition(void *pvParameters) {
    ESP_LOGI("TASK_ACQ", "Task Started on Core %d", xPortGetCoreID());
    
    while (1) {
        // Placeholder Logic
        ESP_LOGI("TASK_ACQ", "Leyendo Sensores (MPU6050 + DS18B20)...");
        
        // Simulating 1Hz acquisition rate
        vTaskDelay(pdMS_TO_TICKS(1000));
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

    // 4. Logic: Config vs Measurement
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
