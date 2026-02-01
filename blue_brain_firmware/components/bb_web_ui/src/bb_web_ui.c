#include "bb_web_ui.h"
#include "bb_config.h"
#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"        // For esp_restart
#include "freertos/FreeRTOS.h" // For vTaskDelay
#include "freertos/task.h"
#include <string.h>
#include <sys/time.h>
#include <time.h>

static const char *TAG = "BB_WEB_UI";

// External Assets (Embedded)
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t style_css_start[] asm("_binary_style_css_start");
extern const uint8_t style_css_end[] asm("_binary_style_css_end");
extern const uint8_t app_js_start[] asm("_binary_app_js_start");
extern const uint8_t app_js_end[] asm("_binary_app_js_end");

// --- HANDLERS ---

// GET /
static esp_err_t root_get_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, (const char *)index_html_start,
                  index_html_end - index_html_start);
  return ESP_OK;
}

// GET /style.css
static esp_err_t style_get_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/css");
  httpd_resp_send(req, (const char *)style_css_start,
                  style_css_end - style_css_start);
  return ESP_OK;
}

// GET /app.js
static esp_err_t app_js_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/javascript");
  httpd_resp_send(req, (const char *)app_js_start, app_js_end - app_js_start);
  return ESP_OK;
}

// GET /api/v1/status
// Returns Mock Data or Real Data if available
#include "bb_dsp_ai.h"

// Training State
static bool g_training_mode = false;
static bool g_capture_active = false;
static int g_capture_target = 0;
static int g_capture_count = 0;
static int g_capture_label = 0;
static float g_capture_freq = 1.0;
static long g_capture_next_ts = 0; // ms

// Helper to get time in ms
static int64_t get_time_ms() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}

// POST /api/v1/command
static esp_err_t api_command_handler(httpd_req_t *req) {
  char buf[256];
  int ret, remaining = req->content_len;

  if (remaining >= sizeof(buf)) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  ret = httpd_req_recv(req, buf, remaining);
  if (ret <= 0)
    return ESP_FAIL;
  buf[ret] = '\0';

  cJSON *root = cJSON_Parse(buf);
  if (!root) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
  if (cJSON_IsString(cmd)) {
    if (strcmp(cmd->valuestring, "set_mode") == 0) {
      cJSON *mode = cJSON_GetObjectItem(root, "mode");
      if (mode && strcmp(mode->valuestring, "training") == 0) {
        g_training_mode = true;
        ESP_LOGI(TAG, "Mode: TRAINING");
      } else {
        g_training_mode = false;
        g_capture_active = false;
        ESP_LOGI(TAG, "Mode: PRODUCTION");
      }
      httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    } else if (strcmp(cmd->valuestring, "start_capture") == 0) {
      g_capture_label = cJSON_GetObjectItem(root, "label")->valueint;
      g_capture_target = cJSON_GetObjectItem(root, "samples")->valueint;
      g_capture_freq = (float)cJSON_GetObjectItem(root, "freq_hz")->valuedouble;

      g_capture_count = 0;
      g_capture_active = true;
      g_capture_next_ts = get_time_ms();
      ESP_LOGI(TAG, "Capture Started: L=%d N=%d F=%.2f", g_capture_label,
               g_capture_target, g_capture_freq);

      httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    } else if (strcmp(cmd->valuestring, "clear_dataset") == 0) {
      remove("/spiffs/training_data.csv"); // Use standard remove
      ESP_LOGI(TAG, "Dataset Cleared");
      httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    } else {
      httpd_resp_send_500(req);
    }
  }
  cJSON_Delete(root);
  return ESP_OK;
}

// GET /download_dataset
static esp_err_t download_dataset_handler(httpd_req_t *req) {
  FILE *f = fopen("/spiffs/training_data.csv", "r");
  if (!f) {
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, "text/csv");
  httpd_resp_set_hdr(req, "Content-Disposition",
                     "attachment; filename=\"training_data.csv\"");

  char chunk[256];
  size_t chunksize;
  while ((chunksize = fread(chunk, 1, sizeof(chunk), f)) > 0) {
    if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
      fclose(f);
      return ESP_FAIL;
    }
  }
  fclose(f);
  httpd_resp_send_chunk(req, NULL, 0); // End
  return ESP_OK;
}

// Capture Logic (Simulated hook called from status to progress)
// In real system, this would be a task. Here we 'tick' it when status is polled
// or via a timer. For robust demo, we put it in status handler just to advance
// basic counter if we can't spawn task. Or better: Checking if we can actually
// save data.
static void training_tick() {
  if (!g_capture_active)
    return;

  int64_t now = get_time_ms();

  // Clamp max capture rate to 10Hz (100ms) even if user selects high freq
  // (Sensor Rate)
  float effective_freq = (g_capture_freq > 10.0f) ? 10.0f : g_capture_freq;
  int interval_ms = (int)(1000.0 / effective_freq);

  if (now >= g_capture_next_ts) {
    // Capture Sample
    bb_telemetry_t report = {0};
    bb_dsp_ai_get_latest(&report);

    // Append to file
    FILE *f = fopen("/spiffs/training_data.csv", "a");
    if (f) {
      // Header if new?
      if (ftell(f) == 0) {
        fprintf(f, "rms,");
        for (int i = 0; i < 5; i++)
          fprintf(f, "fft_low_%d,", i);
        for (int i = 0; i < 5; i++)
          fprintf(f, "fft_mid_%d,", i);
        for (int i = 0; i < 5; i++)
          fprintf(f, "fft_high_%d,", i);
        fprintf(f, "peak_freq,label\n");
      }

      // Data
      fprintf(f, "%.4f,", report.vib_rms);
      for (int i = 0; i < 5; i++)
        fprintf(f, "%.4f,", report.fft_bands_low[i]);
      for (int i = 0; i < 5; i++)
        fprintf(f, "%.4f,", report.fft_bands_mid[i]);
      for (int i = 0; i < 5; i++)
        fprintf(f, "%.4f,", report.fft_bands_high[i]);
      fprintf(f, "%.2f,%d\n", report.vib_dom_freq, g_capture_label);

      fclose(f);
      g_capture_count++;

      if (g_capture_count >= g_capture_target) {
        g_capture_active = false;
        ESP_LOGI(TAG, "Capture Complete!");
      }
    }
    g_capture_next_ts = now + interval_ms;
  }
}
// Background Task Entry
static void training_task_entry(void *arg) {
  while (1) {
    training_tick();
    vTaskDelay(pdMS_TO_TICKS(10)); // 10ms resolution
  }
}

// POST /api/v1/time
static esp_err_t api_time_post_handler(httpd_req_t *req) {
  // ... (Previous implementation)
  char buf[128];
  int ret, remaining = req->content_len;

  if (remaining >= sizeof(buf)) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  ret = httpd_req_recv(req, buf, remaining);
  if (ret <= 0)
    return ESP_FAIL;
  buf[ret] = '\0';

  cJSON *root = cJSON_Parse(buf);
  if (root) {
    cJSON *epoch = cJSON_GetObjectItem(root, "epoch");
    if (epoch) {
      struct timeval tv;
      tv.tv_sec = (time_t)epoch->valuedouble; // Use double for large ints in JS
      tv.tv_usec = 0;
      settimeofday(&tv, NULL);
      ESP_LOGI(TAG, "Time updated to: %ld", tv.tv_sec);
      httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    } else {
      httpd_resp_send_500(req);
    }
    cJSON_Delete(root);
  } else {
    httpd_resp_send_500(req);
  }
  return ESP_OK;
}

// GET /api/v1/status
// Returns Real Telemetry Data
static esp_err_t api_status_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");

  // Get Telemetry
  bb_telemetry_t report = {0};
  bb_dsp_ai_get_latest(&report);

  // Create JSON response
  cJSON *root = cJSON_CreateObject();

  // Time
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char strftime_buf[64];
  strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  cJSON_AddStringToObject(root, "timestamp", strftime_buf);

  // Telemetry
  cJSON_AddNumberToObject(root, "rms", report.vib_rms);
  cJSON_AddNumberToObject(root, "peak", report.vib_peak);
  cJSON_AddNumberToObject(root, "crest", report.crest_factor);
  cJSON_AddNumberToObject(root, "temp", report.temp_c);

  // AI Result
  cJSON_AddNumberToObject(root, "ai_class", report.ai_class);
  cJSON_AddNumberToObject(root, "ai_conf", report.ai_conf);

  // Training Status (Added)
  cJSON_AddBoolToObject(root, "train_active", g_capture_active);
  cJSON_AddNumberToObject(root, "train_count", g_capture_count);
  cJSON_AddNumberToObject(root, "train_target", g_capture_target);

  // Training Status
  cJSON_AddBoolToObject(root, "train_active", g_capture_active);
  cJSON_AddNumberToObject(root, "train_count", g_capture_count);
  cJSON_AddNumberToObject(root, "train_target", g_capture_target);

  // Training Status (Added)
  cJSON_AddBoolToObject(root, "train_active", g_capture_active);
  cJSON_AddNumberToObject(root, "train_count", g_capture_count);
  cJSON_AddNumberToObject(root, "train_target", g_capture_target);

  const char *sys_info = cJSON_PrintUnformatted(root);
  httpd_resp_send(req, sys_info, HTTPD_RESP_USE_STRLEN);

  free((void *)sys_info);
  cJSON_Delete(root);
  return ESP_OK;
}

// GET /api/v1/config
static esp_err_t api_config_get_handler(httpd_req_t *req) {
  const bb_config_t *cfg = bb_config_get();

  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "wifi_ssid", cfg->wifi_ssid);
  // Password usually masked
  cJSON_AddStringToObject(root, "mqtt_uri", cfg->mqtt_broker_uri);
  cJSON_AddNumberToObject(root, "mqtt_port", cfg->mqtt_port);
  cJSON_AddStringToObject(root, "mqtt_user", cfg->mqtt_user);
  cJSON_AddStringToObject(root, "mqtt_topic", cfg->mqtt_topic_telemetry);
  cJSON_AddNumberToObject(root, "sample_rate", cfg->sample_rate_hz);
  cJSON_AddNumberToObject(root, "n_samples", cfg->n_samples);
  cJSON_AddBoolToObject(root, "espnow_en", cfg->espnow_enabled);

  // Thresholds
  cJSON_AddNumberToObject(root, "rms_warn", cfg->rms_alert_warn);
  cJSON_AddNumberToObject(root, "rms_crit", cfg->rms_alert_crit);
  cJSON_AddNumberToObject(root, "temp_warn", cfg->temp_alert_warn);
  cJSON_AddNumberToObject(root, "temp_crit", cfg->temp_alert_crit);

  const char *res = cJSON_PrintUnformatted(root);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, res, HTTPD_RESP_USE_STRLEN);

  free((void *)res);
  cJSON_Delete(root);
  return ESP_OK;
}

// POST /api/v1/config
static esp_err_t api_config_post_handler(httpd_req_t *req) {
  char buf[1024];
  int ret, remaining = req->content_len;

  if (remaining >= sizeof(buf)) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  ret = httpd_req_recv(req, buf, remaining);
  if (ret <= 0)
    return ESP_FAIL;
  buf[ret] = '\0';

  cJSON *root = cJSON_Parse(buf);
  if (root == NULL) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  // Update Config struct
  bb_config_t new_cfg = *bb_config_get(); // Copy current

  cJSON *item = cJSON_GetObjectItem(root, "wifi_ssid");
  if (item)
    strlcpy(new_cfg.wifi_ssid, item->valuestring, sizeof(new_cfg.wifi_ssid));

  item = cJSON_GetObjectItem(root, "wifi_pass");
  if (item && strlen(item->valuestring) > 0) {
    strlcpy(new_cfg.wifi_pass, item->valuestring, sizeof(new_cfg.wifi_pass));
  }

  item = cJSON_GetObjectItem(root, "mqtt_uri");
  if (item)
    strlcpy(new_cfg.mqtt_broker_uri, item->valuestring,
            sizeof(new_cfg.mqtt_broker_uri));

  item = cJSON_GetObjectItem(root, "mqtt_port");
  if (item)
    new_cfg.mqtt_port = item->valueint;

  item = cJSON_GetObjectItem(root, "mqtt_user");
  if (item)
    strlcpy(new_cfg.mqtt_user, item->valuestring, sizeof(new_cfg.mqtt_user));

  item = cJSON_GetObjectItem(root, "mqtt_topic");
  if (item)
    strlcpy(new_cfg.mqtt_topic_telemetry, item->valuestring,
            sizeof(new_cfg.mqtt_topic_telemetry));

  item = cJSON_GetObjectItem(root, "sample_rate");
  if (item)
    new_cfg.sample_rate_hz = item->valueint;

  item = cJSON_GetObjectItem(root, "n_samples");
  if (item)
    new_cfg.n_samples = item->valueint;

  item = cJSON_GetObjectItem(root, "espnow_en");
  if (item)
    new_cfg.espnow_enabled = cJSON_IsTrue(item);

  // Thresholds Parsing
  item = cJSON_GetObjectItem(root, "rms_warn");
  if (item)
    new_cfg.rms_alert_warn = (float)item->valuedouble;

  item = cJSON_GetObjectItem(root, "rms_crit");
  if (item)
    new_cfg.rms_alert_crit = (float)item->valuedouble;

  item = cJSON_GetObjectItem(root, "temp_warn");
  if (item)
    new_cfg.temp_alert_warn = (float)item->valuedouble;

  item = cJSON_GetObjectItem(root, "temp_crit");
  if (item)
    new_cfg.temp_alert_crit = (float)item->valuedouble;

  // Save
  if (bb_config_set(&new_cfg) == ESP_OK) {
    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
  } else {
    httpd_resp_send_500(req);
  }

  cJSON_Delete(root);
  return ESP_OK;
}

// POST /api/v1/restart
static esp_err_t restart_handler(httpd_req_t *req) {
  httpd_resp_send(req, "Restarting...", HTTPD_RESP_USE_STRLEN);
  vTaskDelay(pdMS_TO_TICKS(100)); // Allow response to flush
  esp_restart();
  return ESP_OK;
}

// Checksum handler / captive portal related
// For Android Captive Portal, usually handles /generate_204
static esp_err_t captive_handler(httpd_req_t *req) {
  httpd_resp_set_status(req, "302 Found");
  httpd_resp_set_hdr(req, "Location",
                     "http://192.168.4.1/"); // Redirect to root
  httpd_resp_send(req, NULL, 0);
  return ESP_OK;
}

void bb_web_ui_start(void) {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 12;

  ESP_LOGI(TAG, "Starting HTTP Server...");

  if (httpd_start(&server, &config) == ESP_OK) {
    // Assets
    httpd_uri_t root_uri = {.uri = "/",
                            .method = HTTP_GET,
                            .handler = root_get_handler,
                            .user_ctx = NULL};
    httpd_register_uri_handler(server, &root_uri);

    httpd_uri_t style_uri = {.uri = "/style.css",
                             .method = HTTP_GET,
                             .handler = style_get_handler,
                             .user_ctx = NULL};
    httpd_register_uri_handler(server, &style_uri);

    httpd_uri_t app_uri = {.uri = "/app.js",
                           .method = HTTP_GET,
                           .handler = app_js_handler,
                           .user_ctx = NULL};
    httpd_register_uri_handler(server, &app_uri);

    // API
    httpd_uri_t status_uri = {.uri = "/api/v1/status",
                              .method = HTTP_GET,
                              .handler = api_status_handler,
                              .user_ctx = NULL};
    httpd_register_uri_handler(server, &status_uri);

    httpd_uri_t config_get_uri = {.uri = "/api/v1/config",
                                  .method = HTTP_GET,
                                  .handler = api_config_get_handler,
                                  .user_ctx = NULL};
    httpd_register_uri_handler(server, &config_get_uri);

    httpd_uri_t config_post_uri = {.uri = "/api/v1/config",
                                   .method = HTTP_POST,
                                   .handler = api_config_post_handler,
                                   .user_ctx = NULL};
    httpd_register_uri_handler(server, &config_post_uri);

    httpd_uri_t restart_uri = {.uri = "/api/v1/restart",
                               .method = HTTP_POST,
                               .handler = restart_handler,
                               .user_ctx = NULL};
    httpd_register_uri_handler(server, &restart_uri);

    httpd_uri_t time_uri = {.uri = "/api/v1/time",
                            .method = HTTP_POST,
                            .handler = api_time_post_handler,
                            .user_ctx = NULL};
    httpd_register_uri_handler(server, &time_uri);

    // Training APIs
    httpd_uri_t cmd_uri = {.uri = "/api/v1/command",
                           .method = HTTP_POST,
                           .handler = api_command_handler,
                           .user_ctx = NULL};
    httpd_register_uri_handler(server, &cmd_uri);

    httpd_uri_t dl_uri = {.uri = "/download_dataset",
                          .method = HTTP_GET,
                          .handler = download_dataset_handler,
                          .user_ctx = NULL};
    httpd_register_uri_handler(server, &dl_uri);

    // Start Background Training Task
    xTaskCreate(training_task_entry, "train_task", 4096, NULL, 5, NULL);

    // Captive Portal Catch-All (simplified)
    // Note: Proper wildcard needing for Android captive check
    httpd_uri_t captive_uri = {.uri = "/generate_204",
                               .method = HTTP_GET,
                               .handler = captive_handler,
                               .user_ctx = NULL};
    httpd_register_uri_handler(server, &captive_uri);

    ESP_LOGI(TAG, "HTTP Server Running");
  } else {
    ESP_LOGE(TAG, "Error starting server!");
  }
}
