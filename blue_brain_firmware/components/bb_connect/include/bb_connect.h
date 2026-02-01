#ifndef BB_CONNECT_H
#define BB_CONNECT_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// --- Estructura de Telemetría Industrial ---
typedef struct {
  float vib_rms;
  float vib_peak;
  float vib_p2p;
  float crest_factor;
  float temp_c;
  float vib_dom_freq;  // Frecuencia dominante (Hz)
  float vib_band_low;  // Energía 10-100 Hz
  float vib_band_high; // Energía 100-500 Hz

  // Advanced Features for Training
  float fft_bands_low[5];  // Detailed Low Band
  float fft_bands_mid[5];  // Detailed Mid Band
  float fft_bands_high[5]; // Detailed High Band

  int ai_class;  // 0=Sano, 1=Desbalance, 2=Falla Rodamiento
  float ai_conf; // Confianza (0.0 - 1.0)
  float batt_v;  // Battery Voltage (V)
} bb_telemetry_t;

// Handle de la cola (Visible para main.c)
extern QueueHandle_t xQueueTelemetry;

void bb_connect_init(void);
void Task_Comms(void *pvParameters); // Declarar la tarea

#endif