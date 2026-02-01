idf_component_register(SRCS "src/bb_dsp_ai.c"
                       INCLUDE_DIRS "include"
                       REQUIRES driver bb_sensors bb_connect esp-dsp bb_config)