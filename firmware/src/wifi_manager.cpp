#include "wifi_manager.h"
#include "config.h"
#include <ESPmDNS.h>
#include <WiFi.h>
#include <time.h>

static bool connected = false;
static bool mdns_started = false;
static bool ntp_started = false;
static char ip_str[16] = "0.0.0.0";
static int last_rssi = 0;

static void start_ntp(void) {
    if (ntp_started) return;
    configTime(TZ_OFFSET_SEC, 0, NTP_SERVER1, NTP_SERVER2);
    ntp_started = true;
}

static void start_mdns(void) {
    if (mdns_started || WiFi.status() != WL_CONNECTED) return;

    if (MDNS.begin(WIFI_HOSTNAME)) {
        mdns_started = true;
        Serial.printf("mDNS: responder started at http://%s.local/\n", WIFI_HOSTNAME);
    } else {
        Serial.printf("mDNS: failed to start for %s.local\n", WIFI_HOSTNAME);
    }
}

static void stop_mdns(void) {
    if (!mdns_started) return;
    MDNS.end();
    mdns_started = false;
}

void wifi_init(void) {
    Serial.printf("WiFi: connecting to %s...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(WIFI_HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        connected = true;
        snprintf(ip_str, sizeof(ip_str), "%s", WiFi.localIP().toString().c_str());
        last_rssi = WiFi.RSSI();
        Serial.printf("WiFi: connected, IP=%s, RSSI=%d dBm\n", ip_str, last_rssi);
        start_mdns();
        start_ntp();
    } else {
        connected = false;
        Serial.println("WiFi: connection failed, will retry in loop");
    }
}

void wifi_check_connection(void) {
    static unsigned long last_check = 0;
    unsigned long now = millis();
    if (now - last_check < 5000) return;
    last_check = now;

    if (WiFi.status() != WL_CONNECTED) {
        if (connected) {
            connected = false;
            Serial.println("WiFi: disconnected, reconnecting...");
            stop_mdns();
        }
        WiFi.reconnect();
        // Wait a bit for reconnect
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 5000) {
            delay(100);
        }
        if (WiFi.status() == WL_CONNECTED) {
            connected = true;
            snprintf(ip_str, sizeof(ip_str), "%s", WiFi.localIP().toString().c_str());
            last_rssi = WiFi.RSSI();
            Serial.printf("WiFi: reconnected, IP=%s, RSSI=%d dBm\n", ip_str, last_rssi);
            start_mdns();
            start_ntp();
        }
    } else {
        if (!connected) {
            connected = true;
            snprintf(ip_str, sizeof(ip_str), "%s", WiFi.localIP().toString().c_str());
            last_rssi = WiFi.RSSI();
            Serial.printf("WiFi: connected, IP=%s, RSSI=%d dBm\n", ip_str, last_rssi);
            start_mdns();
            start_ntp();
        }
        last_rssi = WiFi.RSSI();
    }
}

bool wifi_is_connected(void) {
    return WiFi.status() == WL_CONNECTED;
}

const char* wifi_get_ip(void) {
    if (WiFi.status() == WL_CONNECTED) {
        snprintf(ip_str, sizeof(ip_str), "%s", WiFi.localIP().toString().c_str());
    }
    return ip_str;
}

int wifi_get_rssi(void) {
    if (WiFi.status() == WL_CONNECTED) {
        last_rssi = WiFi.RSSI();
    }
    return last_rssi;
}

const char* wifi_get_ssid(void) {
    return WIFI_SSID;
}

const char* wifi_get_hostname(void) {
    return WIFI_HOSTNAME;
}

bool wifi_get_time_str(char* buf, size_t len) {
    if (!ntp_started) return false;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) return false;
    strftime(buf, len, "%H:%M", &timeinfo);
    return true;
}

static const char* const es_weekday[] = {
    "Domingo", "Lunes", "Martes", "Mi\xc3\xa9rcoles", "Jueves", "Viernes", "S\xc3\xa1bado"
};
static const char* const es_month[] = {
    "Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio",
    "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"
};

bool wifi_get_date_str(char* buf, size_t len) {
    if (!ntp_started) return false;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) return false;
    snprintf(buf, len, "%s, %d de %s de %d",
              es_weekday[timeinfo.tm_wday], timeinfo.tm_mday,
              es_month[timeinfo.tm_mon], timeinfo.tm_year + 1900);
    return true;
}

static const char* const es_weekday_caps[] = {
    "DOMINGO", "LUNES", "MARTES", "MI\xc3\x89RCOLES", "JUEVES", "VIERNES", "S\xc3\x81BADO"
};
static const char* const es_month_caps[] = {
    "ENERO", "FEBRERO", "MARZO", "ABRIL", "MAYO", "JUNIO",
    "JULIO", "AGOSTO", "SEPTIEMBRE", "OCTUBRE", "NOVIEMBRE", "DICIEMBRE"
};

bool wifi_get_date_str_caps(char* buf, size_t len) {
    if (!ntp_started) return false;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) return false;
    snprintf(buf, len, "%s\n%d DE %s DE %d",
              es_weekday_caps[timeinfo.tm_wday], timeinfo.tm_mday,
              es_month_caps[timeinfo.tm_mon], timeinfo.tm_year + 1900);
    return true;
}
