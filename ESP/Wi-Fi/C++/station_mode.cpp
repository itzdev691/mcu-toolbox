// ESP32 Wi-Fi station mode (Arduino core).
// Update SSID/PASSWORD, build with Arduino IDE or PlatformIO (Arduino framework).

#include <WiFi.h>

const char* kSsid = "YOUR_SSID";
const char* kPassword = "YOUR_PASSWORD";

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(kSsid, kPassword);

  Serial.print("Connecting");
  const unsigned long start_ms = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
    if (millis() - start_ms > 15000) {  // 15s timeout
      Serial.println("\nFailed to connect");
      return;
    }
  }

  Serial.println();
  Serial.print("Connected, IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Your code here.
}
