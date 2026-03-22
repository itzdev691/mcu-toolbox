#if defined(ARDUINO)
#include <string.h>

#include <WiFi.h>

const char* AP_SSID = "ESP32_Setup";
const char* AP_PASS = "esp32setup";  // Use 8+ chars for WPA2, or leave empty for open AP.

static void startAccessPoint() {
  WiFi.mode(WIFI_AP);

  bool started = false;
  if (strlen(AP_PASS) >= 8) {
    started = WiFi.softAP(AP_SSID, AP_PASS);
  } else {
    started = WiFi.softAP(AP_SSID);
  }

  if (!started) {
    Serial.println("Failed to start access point.");
    return;
  }

  Serial.println("Access point ready");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Use this as a base for a setup portal or local control page.");
}

void setup() {
  Serial.begin(115200);
  delay(200);
  startAccessPoint();
}

void loop() {
  delay(1000);
}

#elif defined(ESP_PLATFORM)
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

const char* AP_SSID = "ESP32_Setup";
const char* AP_PASS = "esp32setup";  // Use 8+ chars for WPA2, or leave empty for open AP.

static const char* kTag = "wifi_ap";
static esp_netif_t* s_ap_netif = nullptr;

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,
                               void* event_data) {
  (void)arg;

  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
    esp_netif_ip_info_t ip_info;
    if (s_ap_netif && esp_netif_get_ip_info(s_ap_netif, &ip_info) == ESP_OK) {
      ESP_LOGI(kTag, "Access point ready");
      ESP_LOGI(kTag, "SSID: %s", AP_SSID);
      ESP_LOGI(kTag, "IP: " IPSTR, IP2STR(&ip_info.ip));
    }
    return;
  }

  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t* event =
        static_cast<wifi_event_ap_staconnected_t*>(event_data);
    ESP_LOGI(kTag, "Client connected: " MACSTR ", aid=%d", MAC2STR(event->mac), event->aid);
    return;
  }

  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t* event =
        static_cast<wifi_event_ap_stadisconnected_t*>(event_data);
    ESP_LOGI(kTag, "Client disconnected: " MACSTR ", aid=%d", MAC2STR(event->mac), event->aid);
  }
}

static void startAccessPoint() {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  s_ap_netif = esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(
      esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr));

  wifi_config_t ap_config = {};
  strncpy(reinterpret_cast<char*>(ap_config.ap.ssid), AP_SSID, sizeof(ap_config.ap.ssid) - 1);
  ap_config.ap.ssid[sizeof(ap_config.ap.ssid) - 1] = '\0';
  ap_config.ap.ssid_len = static_cast<uint8_t>(strlen(AP_SSID));
  ap_config.ap.channel = 1;
  ap_config.ap.max_connection = 4;

  if (strlen(AP_PASS) >= 8) {
    strncpy(reinterpret_cast<char*>(ap_config.ap.password), AP_PASS,
            sizeof(ap_config.ap.password) - 1);
    ap_config.ap.password[sizeof(ap_config.ap.password) - 1] = '\0';
    ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
  } else {
    ap_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
  ESP_ERROR_CHECK(esp_wifi_start());
}

extern "C" void app_main(void) {
  startAccessPoint();

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
#else
#error "Unsupported platform: define ARDUINO or ESP_PLATFORM"
#endif
