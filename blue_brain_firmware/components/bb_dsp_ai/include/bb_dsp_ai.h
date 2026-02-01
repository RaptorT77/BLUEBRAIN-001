#ifndef BB_DSP_AI_H
#define BB_DSP_AI_H

#include "bb_connect.h" // Para bb_telemetry_t
#include <stdint.h>

/**
 * @brief Inicializa el motor DSP/AI
 */
void bb_dsp_ai_init(void);

/**
 * @brief Procesa datos crudos de vibración y rellena el reporte
 * @param raw_data Buffer de datos crudos (6 bytes por muestra: HiLo X, HiLo Y,
 * HiLo Z)
 * @param sample_count Número de muestras en el buffer
 * @param report Puntero a la estructura de telemetría a rellenar
 */
void bb_dsp_ai_process_vibration(uint8_t *raw_data, int sample_count,
                                 bb_telemetry_t *report);

/**
 * @brief Obtiene la última telemetría calculada
 * @param out Puntero donde copiar los datos
 */
void bb_dsp_ai_get_latest(bb_telemetry_t *out);

#endif
