from machine import Pin
import time

# "D2" is commonly the onboard LED, change if your board varies.

led = Pin(2, Pin.OUT)

while True
    led.on()
    delay(0.5)
    led.off()
    delay(0.5)
    