idf_component_register(SRCS "src/bb_sensors.c"
                             "src/i2c_scanner.c"
                             "src/icm42688.c"
                       INCLUDE_DIRS "include"
                       REQUIRES driver esp_adc bb_config)