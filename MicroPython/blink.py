from machine import Pin
import time

PIN_NUM = 21

p = Pin(PIN_NUM, Pin.OUT)

for _ in range(20):
    p.on()
    time.sleep(1)
    p.off()
    time.sleep(1)

print("Done!")
