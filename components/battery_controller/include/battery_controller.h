#pragma once

void battery_monitor_init(void);
int battery_monitor_read_voltage_mv(void);
int battery_monitor_read_percentage(int voltage_mv);
