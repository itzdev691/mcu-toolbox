import network
import time
from machine import Pin

# Creates Wi-Fi interface in station mode (scans and connects to APs)
def scan_aps():
    # ESP32 "D2" is commonly GPIO2; adjust if your board maps D2 differently.
    # "D2" is the onboard LED
    scan_led = Pin(2, Pin.OUT)
    scan_led.on()
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    time.sleep(0.2)
    try:
        # Performs a scan and returns a list of tuples
        aps = wlan.scan() # list of tuples
        print("Found {} APs".format(len(aps)))
        for ap in aps:
            ssid = ap[0].decode("utf-8", "ignore")
            # Build BSSID string from bytes
            bssid = ":".join("{:02x}".format(b) for b in ap[1])
            channel = ap[2]
            rssi = ap[3]
            authmode = ap[4]
            hidden = ap[5]
            print("SSID {:<32} RSSI: {:>4} dBm  CH: {:>2}  AUTH: {}  HIDDEN: {}".format(
                ssid, rssi, channel, authmode, bool(hidden)
            ))
    finally:
        scan_led.off()
        # Runs "scan_aps()" when file is executed directly (not imported)
if __name__ == "__main__":
    scan_aps()
