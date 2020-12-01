"""
Blink LED_PIN every 1 second and then print 'Done!'
"""

__author__ = "Francesco Mecatti"

from machine import Pin
import time

LED_PIN = 21  # Change this value, it may be different for your board

p = Pin(LED_PIN, Pin.OUT)

for _ in range(20):
    p.on()
    time.sleep(1)
    p.off()
    time.sleep(1)

print("Done!")
