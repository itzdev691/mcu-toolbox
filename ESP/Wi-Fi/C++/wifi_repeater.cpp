#if defined(ARDUINO)
#include <WiFi.h>

// Upstream WiFi network (the one you want to extend)
const char* UPSTREAM_SSID = "SSID";
const char* UPSTREAM_PASS = "PASSWORD";

// Local AP (the network your devices connect to)
const char* REPEATER_SSID = "ESP32-C5_Repeater";
const char* REPEATER_PASS = "repeatpass"; // 8+ chars for WPA2

static void startRepeater() {
  WiFi.mode(WIFI_AP_STA);

  Serial.println("Starting AP...");
  bool ap_ok = WiFi.softAP(REPEATER_SSID, REPEATER_PASS);
  if (!ap_ok) {
    Serial.println("Failed to start AP");
  } else {
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  }

  Serial.println("Connecting to upstream WiFi...");
  WiFi.begin(UPSTREAM_SSID, UPSTREAM_PASS);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  startRepeater();

  // NOTE: This example does not configure NAT/forwarding for Arduino builds.
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Upstream WiFi disconnected. Reconnecting...");
    WiFi.reconnect();
  }

  delay(5000);
}

#elif defined(ESP_PLATFORM)
#include <string.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

// Upstream WiFi network (the one you want to extend)
const char* UPSTREAM_SSID = "SSID";
const char* UPSTREAM_PASS = "PASSWORD";

// Local AP (the network your devices connect to)
const char* REPEATER_SSID = "ESP32-C5_Repeater";
const char* REPEATER_PASS = "repeatpass"; // 8+ chars for WPA2

static const char* kTag = "wifi_repeater";
static EventGroupHandle_t s_wifi_event_group;
static const int kStaConnectedBit = BIT0;
static esp_netif_t* s_ap_netif = nullptr;
static esp_netif_t* s_sta_netif = nullptr;
static bool s_napt_enabled = false;
static constexpr uint8_t kDhcpsOfferDns = 0x02;
static constexpr uint8_t kDefault2gApChannel = 1;
static constexpr uint8_t kDefault5gApChannel = 36;

#if CONFIG_SOC_WIFI_SUPPORT_5G
#if CONFIG_WIFI_REPEATER_BAND_MODE_2G_ONLY
static constexpr wifi_band_mode_t kConfiguredBandMode = WIFI_BAND_MODE_2G_ONLY;
#elif CONFIG_WIFI_REPEATER_BAND_MODE_5G_ONLY
static constexpr wifi_band_mode_t kConfiguredBandMode = WIFI_BAND_MODE_5G_ONLY;
#else
static constexpr wifi_band_mode_t kConfiguredBandMode = WIFI_BAND_MODE_AUTO;
#endif
#else
static constexpr wifi_band_mode_t kConfiguredBandMode = WIFI_BAND_MODE_2G_ONLY;
#endif

static const char* band_mode_name(wifi_band_mode_t band_mode) {
  switch (band_mode) {
    case WIFI_BAND_MODE_2G_ONLY:
      return "2.4 GHz only";
    case WIFI_BAND_MODE_5G_ONLY:
      return "5 GHz only";
    case WIFI_BAND_MODE_AUTO:
      return "Auto (2.4 GHz + 5 GHz)";
    default:
      return "Unknown";
  }
}

static bool is_valid_5g_channel(uint8_t channel) {
  switch (channel) {
    case 36:
    case 40:
    case 44:
    case 48:
    case 52:
    case 56:
    case 60:
    case 64:
    case 100:
    case 104:
    case 108:
    case 112:
    case 116:
    case 120:
    case 124:
    case 128:
    case 132:
    case 136:
    case 140:
    case 149:
    case 153:
    case 157:
    case 161:
    case 165:
    case 169:
    case 173:
    case 177:
      return true;
    default:
      return false;
  }
}

static bool is_valid_channel_for_band(uint8_t channel, wifi_band_mode_t band_mode) {
  switch (band_mode) {
    case WIFI_BAND_MODE_2G_ONLY:
      return channel >= 1 && channel <= 14;
    case WIFI_BAND_MODE_5G_ONLY:
      return is_valid_5g_channel(channel);
    case WIFI_BAND_MODE_AUTO:
      return (channel >= 1 && channel <= 14) || is_valid_5g_channel(channel);
    default:
      return false;
  }
}

static uint8_t default_ap_channel_for_band(wifi_band_mode_t band_mode) {
  return band_mode == WIFI_BAND_MODE_5G_ONLY ? kDefault5gApChannel : kDefault2gApChannel;
}

static uint8_t configured_ap_channel() {
  const uint8_t configured_channel = CONFIG_WIFI_REPEATER_AP_CHANNEL;
  if (is_valid_channel_for_band(configured_channel, kConfiguredBandMode)) {
    return configured_channel;
  }

  const uint8_t fallback_channel = default_ap_channel_for_band(kConfiguredBandMode);
  ESP_LOGW(kTag, "Configured AP channel %u is not valid for %s; falling back to %u",
           configured_channel, band_mode_name(kConfiguredBandMode), fallback_channel);
  return fallback_channel;
}

static bool has_valid_dns(const esp_netif_dns_info_t& dns_info) {
  return dns_info.ip.type == ESP_IPADDR_TYPE_V4 && dns_info.ip.u_addr.ip4.addr != 0;
}

static void sync_ap_dns_from_sta() {
  if (!s_ap_netif || !s_sta_netif) {
    return;
  }

  esp_netif_dns_info_t dns_info = {};
  esp_err_t err = esp_netif_get_dns_info(s_sta_netif, ESP_NETIF_DNS_MAIN, &dns_info);
  if (err != ESP_OK || !has_valid_dns(dns_info)) {
    err = esp_netif_get_dns_info(s_sta_netif, ESP_NETIF_DNS_BACKUP, &dns_info);
  }

  if (err != ESP_OK || !has_valid_dns(dns_info)) {
    ESP_LOGW(kTag, "No upstream DNS available for AP clients");
    return;
  }

  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(s_ap_netif));

  uint8_t dhcps_offer_dns = kDhcpsOfferDns;
  err = esp_netif_dhcps_option(s_ap_netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER,
                               &dhcps_offer_dns, sizeof(dhcps_offer_dns));
  if (err != ESP_OK) {
    ESP_LOGE(kTag, "Failed to enable DHCP DNS offer: %d", static_cast<int>(err));
  } else {
    err = esp_netif_set_dns_info(s_ap_netif, ESP_NETIF_DNS_MAIN, &dns_info);
    if (err != ESP_OK) {
      ESP_LOGE(kTag, "Failed to set AP DNS info: %d", static_cast<int>(err));
    } else {
      ESP_LOGI(kTag, "AP clients will use upstream DNS: " IPSTR,
               IP2STR(&dns_info.ip.u_addr.ip4));
    }
  }

  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(s_ap_netif));
}

static void try_enable_napt() {
  if (s_napt_enabled || !s_ap_netif) {
    return;
  }

  esp_err_t err = esp_netif_napt_enable(s_ap_netif);
  if (err == ESP_OK) {
    s_napt_enabled = true;
    ESP_LOGI(kTag, "NAPT enabled on AP interface");
  } else if (err == ESP_ERR_NOT_SUPPORTED) {
    ESP_LOGW(kTag, "NAPT not supported (enable IP forwarding + IPv4 NAPT in menuconfig)");
  } else {
    ESP_LOGE(kTag, "NAPT enable failed: %d", static_cast<int>(err));
  }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,
                               void* event_data) {
  (void)arg;
  (void)event_data;

  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
    return;
  }

  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    ESP_LOGW(kTag, "Upstream WiFi lost. Reconnecting...");
    esp_wifi_connect();
    xEventGroupClearBits(s_wifi_event_group, kStaConnectedBit);
    return;
  }

  if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(event_data);
    ESP_LOGI(kTag, "Upstream IP: " IPSTR, IP2STR(&event->ip_info.ip));
    xEventGroupSetBits(s_wifi_event_group, kStaConnectedBit);
    sync_ap_dns_from_sta();
    try_enable_napt();
    return;
  }

  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
    if (s_ap_netif) {
      esp_netif_ip_info_t ip_info;
      if (esp_netif_get_ip_info(s_ap_netif, &ip_info) == ESP_OK) {
        ESP_LOGI(kTag, "AP IP: " IPSTR, IP2STR(&ip_info.ip));
      }
    }
    return;
  }
}

static void start_repeater() {
  const uint8_t ap_channel = configured_ap_channel();

  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  s_sta_netif = esp_netif_create_default_wifi_sta();
  s_ap_netif = esp_netif_create_default_wifi_ap();
  ESP_ERROR_CHECK(esp_netif_set_default_netif(s_sta_netif));

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  s_wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr));

  wifi_config_t sta_config = {};
  strncpy(reinterpret_cast<char*>(sta_config.sta.ssid), UPSTREAM_SSID, sizeof(sta_config.sta.ssid) - 1);
  sta_config.sta.ssid[sizeof(sta_config.sta.ssid) - 1] = '\0';
  strncpy(reinterpret_cast<char*>(sta_config.sta.password), UPSTREAM_PASS,
          sizeof(sta_config.sta.password) - 1);
  sta_config.sta.password[sizeof(sta_config.sta.password) - 1] = '\0';
  sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  sta_config.sta.pmf_cfg.capable = true;
  sta_config.sta.pmf_cfg.required = false;

  wifi_config_t ap_config = {};
  strncpy(reinterpret_cast<char*>(ap_config.ap.ssid), REPEATER_SSID, sizeof(ap_config.ap.ssid) - 1);
  ap_config.ap.ssid[sizeof(ap_config.ap.ssid) - 1] = '\0';
  ap_config.ap.ssid_len = static_cast<uint8_t>(strlen(REPEATER_SSID));
  strncpy(reinterpret_cast<char*>(ap_config.ap.password), REPEATER_PASS,
          sizeof(ap_config.ap.password) - 1);
  ap_config.ap.password[sizeof(ap_config.ap.password) - 1] = '\0';
  ap_config.ap.channel = ap_channel;
  ap_config.ap.max_connection = 4;
  ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

  if (strlen(REPEATER_PASS) < 8) {
    ap_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_band_mode(kConfiguredBandMode));

  ESP_LOGI(kTag, "AP+STA started with %s, initial AP channel %u. Waiting for upstream connection...",
           band_mode_name(kConfiguredBandMode), ap_channel);

  // NOTE: NAT/forwarding is enabled by NAPT when supported by the ESP-IDF build.
}

extern "C" void app_main(void) {
  start_repeater();

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
#else
#error "Unsupported platform: define ARDUINO or ESP_PLATFORM"
#endif
