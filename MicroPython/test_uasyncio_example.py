"""
uasyncio example from documentation, but adapted for ESP32
"""

__author__ = "Francesco Mecatti"

import uasyncio
from machine import Pin

LED_PIN = 21  # Change this value, it may be different for your board

async def blink(led, period_ms):
    while True:
        led.on()
        await uasyncio.sleep_ms(5)
        led.off()
        await uasyncio.sleep_ms(period_ms)

async def main(led):
    await blink(led, 700)  # Do task forever


if __name__ == "__main__":
    p = Pin(LED_PIN, Pin.OUT)
    uasyncio.run(main(p))
