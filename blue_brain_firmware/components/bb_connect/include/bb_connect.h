#ifndef BB_CONNECT_H
#define BB_CONNECT_H

#include <stdbool.h>

void bb_connect_init(void);
void bb_connect_wifi_start(void);
void bb_connect_check_update(void);
void bb_connect_run_ota(const char *url);

#endif // BB_CONNECT_H
