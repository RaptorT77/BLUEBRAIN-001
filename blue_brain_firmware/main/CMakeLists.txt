idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "include"
                    REQUIRES bb_power bb_connect bb_web_ui bb_espnow bb_sensors bb_dsp_ai bb_display bb_storage)
