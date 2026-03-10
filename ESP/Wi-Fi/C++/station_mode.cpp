#include <WiFi.h>

// Change to your credentials
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASS = "YOUR_PASSWORD";

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to WiFi");
  unsigned long start = millis();
  const unsigned long timeoutMs = 20000; // 20s

  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    delay(500);
    Serial.print(".");
  }
// Show connection status
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect.");
  }
}
void setup() {
  Serial.begin(115200);
  delay(100);
  connectWiFi();

  // put your setup code here, to run once:
}

void loop() {
  // Keep connection alive
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost. Recconecting...");
    connectWiFi();
  }
  delay(5000);
}