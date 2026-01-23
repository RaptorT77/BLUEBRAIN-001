#include "esp_log.h"

static const char *TAG = "BB_SENSORS";

void bb_sensors_init(void)
{
    ESP_LOGI(TAG, "Initializing Sensors (MPU6050, DS18B20)");
    // TODO: Init I2C, OneWire
}

void bb_sensors_read_all(void)
{
    // TODO: Read sensors
}
