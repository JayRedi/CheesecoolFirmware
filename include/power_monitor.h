#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H
#include <stdbool.h>
void power_monitor_init(void); void power_monitor_update(void); bool power_monitor_has_fault(void);
#endif
