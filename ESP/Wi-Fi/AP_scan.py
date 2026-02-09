import network
import time

# Creates Wi-Fi interface in station mode (scans and connects to APs)
def scan_aps():
    wlan = netowrk.WLAN(network.STA_IF)
    wlan.active(True)
    time.sleep(0.2)
    # Performs a scan and returns a list of tuples
    aps = wlan.scan() # list of tuples
    print("Found {} APs".format(len(aps)))
    for ap in aps:
        ssid = ap[0].decode("utf-8", "ignore")
        bssid = ":".join("{:02x}".format(b_) for b in ap[1])
        channel = ap[2]
        rssi = ap[3]
        authmode = ap[4]
        hidden = ap[5]
        print("SSID {:<32} RSSI: {:>4} dBm  CH: {:>2}  AUTH: {}  HIDDEN: {}".format(
            ssid, rssi, channel, authmode, bool(hidden)
        ))
        # Runs "scan_aps()" when file is executed directly (not imported)
if __name__ == "__main__":
    scan_aps()