import network

# 1. Create the access point interface object
ap = network.WLAN(network.AP_IF)

# 2. Activate the interface
ap.active(True)

# 3. Configure the netowrk credentials and security
# Replace 'MyESP32' and 'mypassword' with your desired settings
ap.config(essid='MyESP32',
          password='mypassword',
          authmode=network.AUTH_WPA_WPA2_PSK)


# 4. Check the ESP's IP address (defauly is usally 192.168.4.1)
print('AP Active. Network config:', ap.ifconfig())