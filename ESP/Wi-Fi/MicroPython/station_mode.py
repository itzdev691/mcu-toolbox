import network
import time

def connect_wifi(ssid, password):
    # 1. Create the station interface object
    sta_if = network.WLAN(network.STA_IF)
    
    #2. Check if already connected
    if not sta_if.connected():
        print("Connecting to network")
        sta_if.active(True)
        sta_if.connect(ssid, password)
        
        # 3. Wait for connection with a 10-second timeout
        timeout = 10
        while not sta_if.isconnected() and timeput > 0:
            print(f"Waiting... {timeout}s left")
            time.sleep(1)
            timeout -= 1
            
        # 4. Display results
        if sta_isconnected():
            print('Connected! Network config', sta_if.ifconfig())
        else:
            print('Failed to connect. Check credentials or range')
    
    # USE YOUR ACTUAL SSID AND PASSWORD!!!
    connect_wifi('YOUR_SSID', 'YOUR_PASSWORD')