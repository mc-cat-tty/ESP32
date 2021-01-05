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
COUNTER_RUNNING: bool = False

def printTime() -> None:
    global CENTS
    t = utime.localtime(int(CENTS/100))
    print("\rTime: %02d:%02d:%02d" % (t[3], t[4], t[5]), end="")

def printAddCents() -> None:  # Add centiseconds to the previous timestamp
    print(".%02d" % (CENTS%100), end="")

async def counterCoro() -> None:
    global CENTS
    print("Entered counterCoro()")
    while True:
        CENTS += 1
        if CENTS % 100 == 0:
            printTime()
        await uasyncio.sleep_ms(10)  # 10 ms, namely 1 cs

async def buttonCoro(button: Pin, led: Pin) -> None:
    global CENTS, COUNTER_RUNNING
    print("Entered buttonTask()")
    print("Start stopwatch by pressing BOOT button")
    startCents: int = 0
    fms_state: State = State.IDLE
    counterCoroObj: uasyncio.Task = None
    while True:
        buttonValue = button.value()
        if fms_state == State.IDLE:
            led.off()
            if buttonValue == 0:  # Pressed
                startCents = CENTS
                fms_state = State.LAP
            elif buttonValue == 1:  # Released
                pass
        elif fms_state == State.LAP:
            led.on()
            if not COUNTER_RUNNING:  # Start stopwatch
                counterCoroObj = uasyncio.create_task(counterCoro())
                COUNTER_RUNNING = True
            else:  # Get lap
                printAddCents()
                print("\t<-\tLap")
                printTime()
            if buttonValue == 0:  # Pressed
                fms_state = State.SHORT_PRESS
            elif buttonValue == 1:  # Released
                fms_state = State.IDLE
        elif fms_state == State.SHORT_PRESS:
            led.on()
            if buttonValue == 0:  # Pressed
                if (CENTS - startCents) < 0.5*100:
                    pass
                elif (CENTS - startCents) >= 0.5*100:
                    fms_state = State.STOP_AND_RESET
            elif buttonValue == 1:  # Released
                fms_state = State.IDLE
        elif fms_state == State.STOP_AND_RESET:
            led.off()  # Quick blink to confirm reset
            await uasyncio.sleep_ms(50)
            led.on()
            counterCoroObj.cancel()
            CENTS = 0
            COUNTER_RUNNING = False
            print("\nSTOPPED AND RESETTED")
            print("Restart by pressing BOOT button")
            if buttonValue == 0:  # Pressed
                fms_state = State.LONG_PRESS
            elif buttonValue == 1:  # Released
                fms_state = State.IDLE
        elif fms_state == State.LONG_PRESS:
            led.on()
            if buttonValue == 0:  # Pressed
                pass
            elif buttonValue == 1:  # Released
                fms_state = State.IDLE
        await uasyncio.sleep_ms(5)

async def main(buttonPin: Pin, ledPin: Pin) -> None:
    print("Entered main()")
    uasyncio.create_task(buttonCoro(buttonPin, ledPin))
    loop = uasyncio.get_event_loop()  # Loop forever, otherwise the program would exit
    loop.run_forever()
    print("Created coroutines")

if __name__ == "__main__":
    print("Setting up pins")
    ledPin = Pin(LED_PIN, Pin.OUT)
    buttonPin = Pin(BUTTON_PIN, Pin.IN, Pin.PULL_UP)
    uasyncio.run(main(buttonPin, ledPin))
