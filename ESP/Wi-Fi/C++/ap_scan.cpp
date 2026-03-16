#include <stdint.h>

#if defined(ARDUINO)

#include <WiFi.h>

void setup() {
    Serial.begin(115200);
    delay(1000);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(100);

    Serial.println("Scanning...");
    int n = WiFi.scanNetworks(false, true); // sync, include hidden
    if (n <= 0) {
        Serial.println("No networks found");
        return;
    }

    for (int i = 0; i < n; ++i) {
        Serial.printf("%2d | %-32s | RSSI %4d | CH %2d | ENC %d\n",
                  i + 1,
                  WiFi.SSID(i).c_str(),
                  WiFi.RSSI(i),
                  WiFi.channel(i),
                  WiFi.encryptionType(i));
    }
}

void loop() {}

#elif defined(ESP_PLATFORM)

#include <vector>

extern "C" {
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
}

static const char *TAG = "ap_scan";

static void wifi_init_sta() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void scan_and_print() {
    wifi_scan_config_t scan_cfg = {};
    scan_cfg.ssid = nullptr;
    scan_cfg.bssid = nullptr;
    scan_cfg.channel = 0;       // 0 = all channels
    scan_cfg.show_hidden = true;

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_cfg, true)); // block until scan done
    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

    std::vector<wifi_ap_record_t> aps(ap_count);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, aps.data()));   

    for (int i = 0; i < ap_count; ++i) {
        const auto &ap = aps[i];
        ESP_LOGI(TAG, "%2d | %-32s | RSSI %4d | CH %2d | AUTH %d",
            i + 1,
            reinterpret_cast<const char*>(ap.ssid),
            ap.rssi,
            ap.primary,
            ap.authmode);
    }
}

extern "C" void app_main() {
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_sta();
    scan_and_print();
}

#else
#error "Unsupported platform: define ARDUINO or ESP_PLATFORM"
#endif
