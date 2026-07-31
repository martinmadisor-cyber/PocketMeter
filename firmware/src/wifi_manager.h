#pragma once
#include <Arduino.h>

void wifi_init(void);
void wifi_check_connection(void);
bool wifi_is_connected(void);
const char* wifi_get_ip(void);
int wifi_get_rssi(void);
const char* wifi_get_ssid(void);
const char* wifi_get_hostname(void);
// Writes "HH:MM" into buf. Returns false (buf untouched) until NTP has synced.
bool wifi_get_time_str(char* buf, size_t len);
// Writes "Miércoles, 7 de Mayo de 2026" (Spanish) into buf. Returns false until NTP has synced.
bool wifi_get_date_str(char* buf, size_t len);
// Writes "MIÉRCOLES\n7 DE MAYO DE 2026" (Spanish, uppercase, two lines). Returns false until NTP has synced.
bool wifi_get_date_str_caps(char* buf, size_t len);
