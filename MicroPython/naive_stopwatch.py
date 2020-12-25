"""
Stopwatch able to distinguish between short and long press. 
Get lap (time slice) every time BUTTON_PIN is pressed
Reset time counter whenever a long press occurs
Everything is managed by uasyncio asynchronous scheduler
"""

__author__ = "Francesco Mecatti"

import sys
import uasyncio
import utime
from machine import Pin

LED_PIN: int = 21  # Change this value, it may be different for your board
BUTTON_PIN: int = 0  # Change this value, it may be different for your board
ON: bools = False
cents: int = 0

async def counterCoro() -> None:
    global cents
    while True:
        cents += 1
        if cents % 100 == 0:
            t = utime.localtime(int(cents/100))
            print("Time: %02d:%02d:%02d" % (t[3], t[4], t[5]), end="\r")
        await uasyncio.sleep_ms(10)  # 10 ms, namely 1 cs

# async def buttonTask(button):
#     global ON
#     while True:
#         if button.value() == 0:
#             ON = True
#         else:
#             ON = False
#         await uasyncio.sleep_ms(5)

async def main(ledPin: int, buttonPin: int) -> None:
    print("Entered main()")
    await counterCoro()
    # uasyncio.create_task(buttonTask(buttonPin))
    print("Created task")  # All print() will be shown when the script terminates
    # Await no more than 10/20 seconds because of the timeout

if __name__ == "__main__":
    print("Setting up pins")
    ledPin = Pin(LED_PIN, Pin.OUT)
    buttonPin = Pin(BUTTON_PIN, Pin.IN, Pin.PULL_UP)
    uasyncio.run(main(ledPin, buttonPin))
