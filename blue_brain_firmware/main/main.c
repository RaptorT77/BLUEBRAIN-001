/**
 * @file main.c
 * @brief Blue Brain Orchestrator v5.0 - Modular Architecture
 * @author Blue Brain Team
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <stdint.h> // Required for uint8_t
#include <stdio.h>
#include <stdlib.h> // Required for malloc/free
#include <string.h>

// --- COMPONENTES PROPIOS ---
#include "bb_config.h"
#include "bb_connect.h"
#include "bb_dsp_ai.h"
#include "bb_power.h"
#include "bb_sensors.h"
#include "bb_storage.h"
#include "bb_web_ui.h"

static const char *TAG = "BLUE_BRAIN_MAIN";

// --- TAREA DE ANÁLISIS DE VIBRACIÓN (CORE 1) ---
void Task_Vibration_Analysis(void *pvParameters) {
  bb_telemetry_t report;

  // Buffer en Heap para no desbordar el stack
  // Allocate MAX size (defined in bb_config.h)
  size_t buffer_size =
      BB_N_SAMPLES * 6; // 6 bytes por muestra (int16_t x, y, z)
  uint8_t *raw_data = (uint8_t *)calloc(1, buffer_size);

  if (raw_data == NULL) {
    ESP_LOGE(TAG,
             "FATAL: No hay memoria para el buffer de vibración (%d bytes)",
             buffer_size);
    vTaskDelete(NULL);
    return;
  }

  ESP_LOGI(TAG, "Tarea de Análisis Iniciada. Buffer de %d bytes reservado.",
           buffer_size);

  while (1) {
    // Leer configuración actual
    const bb_config_t *cfg = bb_config_get();
    int current_samples = cfg->n_samples;

    // Sanity check
    if (current_samples > BB_N_SAMPLES)
      current_samples = BB_N_SAMPLES;
    if (current_samples < 64)
      current_samples = 64;

    ESP_LOGI(TAG, "Iniciando ráfaga de %d muestras...", current_samples);

    // 1. Adquisición de Datos (Bloqueante/Precisa)
    bb_sensors_read_accel_burst(raw_data, current_samples);

    // 2. Procesamiento DSP (Cálculo de RMS, Peak, etc)
    bb_dsp_ai_process_vibration(raw_data, current_samples, &report);

    // 3. Adquisición de Temperatura
    report.temp_c = bb_sensors_get_temp();

    // 4. Telemetría Local y Remota
    ESP_LOGW(TAG, "==== REPORTE BLUE BRAIN ====");
    ESP_LOGI(TAG, "RMS: %.3f G | Peak: %.3f G | CF: %.2f", report.vib_rms,
             report.vib_peak, report.crest_factor);
    ESP_LOGI(TAG, "Temp: %.2f C", report.temp_c);

    // Enviar a la cola para que bb_connect lo procese (MQTT/ESP-NOW)
    if (xQueueTelemetry != NULL) {
      if (xQueueSend(xQueueTelemetry, &report, 0) != pdPASS) {
        ESP_LOGW(TAG, "Cola llena, telemetría descartada");
      } else {
        ESP_LOGD(TAG, "Datos enviados a Task_Comms");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(5000)); // Intervalo entre reportes
  }
}

// --- PUNTO DE ENTRADA PRINCIPAL ---
void app_main(void) {
  // 1. Inicializar NVS (Requerido por WiFi/OTA/MQTT/Config)
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // 1.1 Cargar Configuración del Sistema (NVS)
  ESP_ERROR_CHECK(bb_config_init());

  // 1.2 Inicializar SPIFFS (Almacenamiento Local)
  bb_storage_init();

  // 2. Inicializar Sensores (I2C + MPU6050 + DS18B20 GPIO)
  ESP_ERROR_CHECK(bb_sensors_init());

  // 3. Inicializar DSP
  bb_dsp_ai_init();

  // 4. Lanzar Conectividad (WiFi + MQTT + OTA + Queue + Power) en CORE 0
  bb_connect_init();

  // 4.5 Iniciar Web UI (Dashboard + API + Captive Portal)
  bb_web_ui_start();

  // 5. Lanzar Análisis en CORE 1 (Máxima prioridad para muestreo)
  xTaskCreatePinnedToCore(Task_Vibration_Analysis, "Vib_Analyst", 8192, NULL, 5,
                          NULL, 1);
}