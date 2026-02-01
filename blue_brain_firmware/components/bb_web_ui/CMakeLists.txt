idf_component_register(SRCS "src/bb_web_ui.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_http_server
                       PRIV_REQUIRES bb_config esp_wifi nvs_flash json bb_dsp_ai bb_connect
                       EMBED_TXTFILES "index.html" "style.css" "app.js")