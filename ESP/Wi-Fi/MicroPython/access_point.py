import network
import time


def start_access_point(ssid="ESP32_Setup", password="esp32setup", channel=6):
    ap = network.WLAN(network.AP_IF)
    ap.active(True)

    if password and len(password) >= 8:
        ap.config(
            essid=ssid,
            password=password,
            authmode=network.AUTH_WPA_WPA2_PSK,
            channel=channel,
        )
        security = "WPA/WPA2"
    else:
        ap.config(essid=ssid, authmode=network.AUTH_OPEN, channel=channel)
        security = "OPEN"

    while not ap.active():
        time.sleep_ms(100)

    print("Access point ready")
    print("SSID:", ssid)
    print("Security:", security)
    print("Connect to: http://192.168.4.1")
    print("Network config:", ap.ifconfig())
    return ap


if __name__ == "__main__":
    # Change the SSID/password to fit your project.
    start_access_point()
