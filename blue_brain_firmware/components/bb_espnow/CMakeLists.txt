idf_component_register(SRCS "src/bb_espnow.c"
                       INCLUDE_DIRS "include"
                       REQUIRES driver esp_wifi bb_connect bb_config)