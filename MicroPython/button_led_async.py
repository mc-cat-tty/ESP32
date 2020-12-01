"""
Change LED_PIN state every time BUTTON_PIN is pressed
Everything is managed by uasyncio asynchronous scheduler
"""

__author__ = "Francesco Mecatti"

import uasyncio
from machine import Pin

LED_PIN = 21  # Change this value, it may be different for your board
BUTTON_PIN = 0  # Change this value, it may be different for your board
ON = False

async def ledTask(led):
    global ON
    while True:
        if ON:
            led.on()
        else:
            led.off()
        await uasyncio.sleep_ms(10)

async def buttonTask(button):
    global ON
    while True:
        if button.value() == 0:
            ON = True
        else:
            ON = False
        await uasyncio.sleep_ms(5)

async def main(ledPin, buttonPin):
    print("Entered main()")
    uasyncio.create_task(ledTask(ledPin))
    uasyncio.create_task(buttonTask(buttonPin))
    print("Created tasks")  # All print() will be shown when the script terminates
    # Await no more than 10/20 seconds because of the timeout
    await uasyncio.sleep(10)  # Seconds

if __name__ == "__main__":
    print("Setting up pins")
    ledPin = Pin(LED_PIN, Pin.OUT)
    buttonPin = Pin(BUTTON_PIN, Pin.IN, Pin.PULL_UP)
    uasyncio.run(main(ledPin, buttonPin))
