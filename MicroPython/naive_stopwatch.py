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

class State:
    IDLE = 1,
    LAP = 2,
    SHORT_PRESS = 3,
    STOP_AND_RESET = 4,
    LONG_PRESS = 5

LED_PIN: int = 21  # Change this value, it may be different for your board
BUTTON_PIN: int = 0  # Change this value, it may be different for your board
CENTS: int = 0
FMS_STATE: State = State.IDLE

def printTime() -> None:
    global CENTS
    t = utime.localtime(int(CENTS/100))
    print("Time: %02d:%02d:%02d" % (t[3], t[4], t[5]), end="\r")

async def counterCoro() -> None:
    global CENTS
    print("Entered counterCoro()")
    while True:
        CENTS += 1
        if CENTS % 100 == 0:
            printTime()
        await uasyncio.sleep_ms(10)  # 10 ms, namely 1 cs

async def buttonTask(button: Pin, led: Pin) -> None:
    global FMS_STATE
    print("Entered buttonTask()")
    startCents: int = 0
    while True:
        buttonValue = button.value
        if FMS_STATE is State.IDLE:
            led.off()
            if buttonValue == 0:  # Pressed
                startCents = CENTS
                FMS_STATE = State.LAP
            elif buttonValue == 1:  # Released
                pass
        elif FMS_STATE is State.LAP:
            led.on()
            print("\nLap\t->\t")
            printTime()
            if buttonValue == 0:  # Pressed
                FMS_STATE = State.SHORT_PRESS
            elif buttonValue == 1:  # Released
                FMS_STATE = State.IDLE
        elif FMS_STATE is State.SHORT_PRESS:
            led.on()
            if buttonValue == 0:  # Pressed
                if (CENTS - startCents) < 0.5*100:
                    pass
                elif (CENTS - startCents) >= 0.5*100:
                    FMS_STATE = State.STOP_AND_RESET
            elif buttonValue == 1:  # Released
                FMS_STATE = State.IDLE
        elif FMS_STATE is State.STOP_AND_RESET:
            led.off()  # Quick blink to confirm reset
            await uasyncio.sleep_ms(50)
            lef.on()
            if buttonValue == 0:  # Pressed
                FMS_STATE = State.LONG_PRESS
            elif buttonValue == 1:  # Released
                FMS_STATE = State.IDLE
        elif FMS_STATE is State.LONG_PRESS:
            led.on()
            if buttonValue == 0:  # Pressed
                pass
            elif buttonValue == 1:  # Released
                FMS_STATE = State.IDLE
        await uasyncio.sleep_ms(1)

async def main(buttonPin: Pin, ledPin: Pin) -> None:
    print("Entered main()")
    uasyncio.create_task(buttonTask(buttonPin, ledPin))
    await counterCoro()
    print("Created tasks")

if __name__ == "__main__":
    print("Setting up pins")
    ledPin = Pin(LED_PIN, Pin.OUT)
    buttonPin = Pin(BUTTON_PIN, Pin.IN, Pin.PULL_UP)
    uasyncio.run(main(buttonPin, ledPin))
