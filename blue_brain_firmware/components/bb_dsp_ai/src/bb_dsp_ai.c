/**
 * @file bb_dsp_ai.c
 * @brief Blue Brain DSP & AI Engine - Hardware Optimized Signal Processing
 * @note Uses ESP-DSP library for optimized calculations
 */

#include "bb_dsp_ai.h"
#include "bb_sensors.h"
#include "esp_log.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// ESP-DSP Library - Main header
#include "esp_dsp.h"

#include "bb_config.h"

static const char *TAG = "BB_DSP_AI";

// Shareable telemetry storage
static bb_telemetry_t g_last_report = {0};

void bb_dsp_ai_get_latest(bb_telemetry_t *out) {
  if (out) {
    memcpy(out, &g_last_report, sizeof(bb_telemetry_t));
  }
}

// FFT Configuration (Loaded from bb_config.h)
#define N_SAMPLES BB_N_SAMPLES
#define FFT_SIZE BB_FFT_SIZE
#define SAMPLE_RATE_HZ BB_SAMPLE_RATE_HZ

// Static buffers to save stack size (1024 * 4 * 2 = 8KB for complex input)
// Check if these fit in RAM (we have plenty of heap/bss)
static float wind_hann[N_SAMPLES];
static float fft_input[N_SAMPLES * 2]; // Real + Imag interleaved

void bb_dsp_ai_init(void) {
  // Initialize DSP library
  esp_err_t ret = dsps_fft2r_init_fc32(NULL, FFT_SIZE);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Not possible to initialize FFT. Error = %i", ret);
    return;
  }

  // Generate Hann Window
  dsps_wind_hann_f32(wind_hann, N_SAMPLES);

  ESP_LOGI(TAG, "DSP Engine Initialized: FFT Size=%d, Window=Hann", FFT_SIZE);
}

// Suppress false positive from GCC 14.2.0's aggressive flow analysis
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

void bb_dsp_ai_process_vibration(uint8_t *raw_data, int sample_count,
                                 bb_telemetry_t *report) {
  if (sample_count > N_SAMPLES) {
    ESP_LOGW(TAG, "Sample count %d > FFT Size %d, truncating", sample_count,
             N_SAMPLES);
    sample_count = N_SAMPLES;
  }

  // Allocate working buffer for magnitude time-domain calculations
  float *magnitude = (float *)malloc(sample_count * sizeof(float));
  if (magnitude == NULL) {
    ESP_LOGE(TAG, "Failed to allocate memory for DSP processing");
    return;
  }

  // Initialize tracking variables
  float max_value = 0.0f;
  float min_value = 100.0f;

  // Step 1: Convert raw sensor data to G-force magnitudes
  for (int i = 0; i < sample_count; i++) {
    uint8_t *sample = &raw_data[i * 6];

    // Convert 16-bit raw values to float G-forces
    float ax = (int16_t)((sample[0] << 8) | sample[1]) / BB_ACCEL_SENS_16G;
    float ay = (int16_t)((sample[2] << 8) | sample[3]) / BB_ACCEL_SENS_16G;
    float az = (int16_t)((sample[4] << 8) | sample[5]) / BB_ACCEL_SENS_16G;

    // Calculate magnitude vector
    magnitude[i] = sqrtf(ax * ax + ay * ay + az * az);

    // Track max and min (manual loops - esp-dsp doesn't have these)
    if (magnitude[i] > max_value) {
      max_value = magnitude[i];
    }
    if (magnitude[i] < min_value) {
      min_value = magnitude[i];
    }
  }

  // Step 2: Time-Domain Metrics (RMS, Peak, CF)
  // RMS = sqrt(dot(x, x) / n)
  float dot_result = 0.0f;
  esp_err_t ret =
      dsps_dotprod_f32_ansi(magnitude, magnitude, &dot_result, sample_count);

  if (ret == ESP_OK) {
    report->vib_rms = sqrtf(dot_result / sample_count);
  } else {
    // Fallback manual calculation
    float sum_sq = 0.0f;
    for (int i = 0; i < sample_count; i++) {
      sum_sq += magnitude[i] * magnitude[i];
    }
    report->vib_rms = sqrtf(sum_sq / sample_count);
  }

  report->vib_peak = max_value;
  report->vib_p2p = max_value - min_value;
  report->crest_factor =
      (report->vib_rms > 0.05f) ? (report->vib_peak / report->vib_rms) : 0.0f;

  // Step 3: Frequency-Domain Analysis (FFT)
  // 3.1 Apply Hann Window and Prepare Complex Input
  for (int i = 0; i < sample_count; i++) {
    // Remove DC component (Gravity) effectively by windowing?
    // Ideally we should subtract mean first, but Hann window tapers edges to
    // zero anyway. Let's subtract 1.0G (Gravity) approx or just let FFT handle
    // DC at index 0. Windowing the total magnitude (which includes 1G DC) will
    // create a large logic DC lobe. Better to remove mean first.

    // float val = magnitude[i] - 1.0f; // Approx gravity removal
    float val = magnitude[i]; // Raw magnitude

    fft_input[i * 2 + 0] = val * wind_hann[i]; // Real part
    fft_input[i * 2 + 1] = 0.0f;               // Imag part
  }

  // Zero pad if necessary (though our burst is 1024)
  for (int i = sample_count; i < FFT_SIZE; i++) {
    fft_input[i * 2 + 0] = 0.0f;
    fft_input[i * 2 + 1] = 0.0f;
  }

  // 3.2 Execute FFT (Radix-2)
  dsps_fft2r_fc32(fft_input, FFT_SIZE);

  // 3.3 Bit Reversal (required for standard frequency order)
  dsps_bit_rev_fc32(fft_input, FFT_SIZE);

  // 3.4 Find Dominant Frequency & Calculate Band Energies
  // Search from index 1 to N/2 (Ignore DC at index 0)
  float max_fft_mag = 0.0f;
  int max_fft_idx = 0;

  // Band Energy Accumulators (Sum of magnitude squared = Energy-ish)
  // Or just Sum of magnitudes (as user requests "Energy bands" usually RMS in
  // band) Let's use Sum of Magnitudes for simplicity unless RMS is strictly
  // required. Standard ISO is RMS of velocity in band. Here we have
  // acceleration. Let's return Average Magnitude in Band for now.
  float sum_mag_low = 0.0f;
  float sum_mag_high = 0.0f;
  int count_low = 0;
  int count_high = 0;

  // Bin 0 = 0Hz. Bin 1 = ~1Hz (0.97Hz to be precise)
  // Low Band: 10Hz - 100Hz -> Index ~10 to ~102
  // High Band: 100Hz - 500Hz -> Index ~102 to ~512

  for (int i = 1; i < FFT_SIZE / 2; i++) {
    float re = fft_input[i * 2 + 0];
    float im = fft_input[i * 2 + 1];
    float mag = sqrtf(re * re + im * im);

    // 1. Dominant Frequency Logic
    if (mag > max_fft_mag) {
      max_fft_mag = mag;
      max_fft_idx = i;
    }

    // 2. Band Energy Logic
    // Detailed Bands for Training
    // Low: 10-100Hz (5 sub-bands of ~18Hz)
    // Mid: 100-500Hz (5 sub-bands of ~80Hz)
    // High: 500-800Hz (5 sub-bands of ~60Hz) - Capped at Nyquist

    float freq = (float)i * SAMPLE_RATE_HZ / FFT_SIZE;

    // Low Band: 10-100Hz
    if (freq >= 10 && freq < 100) {
      int sub_idx = (int)((freq - 10) / 18.0f);
      if (sub_idx >= 0 && sub_idx < 5)
        report->fft_bands_low[sub_idx] += mag;
      sum_mag_low += mag;
      count_low++;
    }
    // Mid Band: 100-500Hz
    else if (freq >= 100 && freq < 500) {
      int sub_idx = (int)((freq - 100) / 80.0f);
      if (sub_idx >= 0 && sub_idx < 5)
        report->fft_bands_mid[sub_idx] += mag;
      sum_mag_high +=
          mag; // Using high accumulator for legacy compatibility/simplicity
      count_high++;
    }
    // High Band: 500-800Hz
    else if (freq >= 500 && freq < 800) {
      int sub_idx = (int)((freq - 500) / 60.0f);
      if (sub_idx >= 0 && sub_idx < 5)
        report->fft_bands_high[sub_idx] += mag;
    }
  }

  // Normalize bands (Average Magnitude in Band)
  report->vib_band_low = (count_low > 0) ? (sum_mag_low / count_low) : 0.0f;
  report->vib_band_high = (count_high > 0) ? (sum_mag_high / count_high) : 0.0f;

  // Calculate Hz
  // Freq = Index * Fs / N
  float dom_freq = (float)max_fft_idx * SAMPLE_RATE_HZ / (float)FFT_SIZE;
  report->vib_dom_freq = dom_freq;

  // Cleanup
  free(magnitude);

  ESP_LOGD(TAG,
           "DSP: RMS=%.3f, Peak=%.3f, Freq=%.1fHz, LowBand=%.3f, HighBand=%.3f",
           report->vib_rms, report->vib_peak, report->vib_dom_freq,
           report->vib_band_low, report->vib_band_high);

  // Update shared telemetry for Web UI
  memcpy(&g_last_report, report, sizeof(bb_telemetry_t));
}

#pragma GCC diagnostic pop
