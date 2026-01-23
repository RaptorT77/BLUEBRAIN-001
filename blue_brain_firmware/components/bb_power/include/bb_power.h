#ifndef BB_POWER_H
#define BB_POWER_H

#include <stdint.h>
#include "esp_sleep.h"

void bb_power_init(void);
void bb_power_enter_deep_sleep(uint64_t sleep_time_us);
float bb_power_get_battery_voltage(void);

#endif // BB_POWER_H
