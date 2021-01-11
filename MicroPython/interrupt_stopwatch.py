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
from machine import Pin, Timer, enable_irq, disable_irq

class State:
    IDLE = 1,
    LAP = 2,
    SHORT_PRESS = 3,
    STOP_AND_RESET = 4,
    LONG_PRESS = 5

class queue:
    def __init__(self):
        self.items = list()
    def enqueue(self, item):
        self.items.append(item)
    def dequeue(self):
        return self.items.pop(0)
    def is_empty(self):
        return not self.items


LED_PIN: int = 21  # External led. Change this value, it may be different for your board
BUTTON_PIN: int = 0  # Internal BOOT button. Change this value, it may be different for your board
CENTS: int = 0
COUNTER_RUNNING: bool = False
READY_TO_PRINT: bool = False
VALUES_TO_PRINT: queue = queue()

def printTime(on_stdout=True) -> None:
    global CENTS
    t = utime.localtime(int(CENTS/100))
    s: str = "\rTime: %02d:%02d:%02d" % (t[3], t[4], t[5])
    if on_stdout:
        print(s, end="")
    else:
        return s

def printAddCents() -> None:  # Add centiseconds to the previous timestamp
    print(".%02d" % (CENTS%100), end="")

def handleTimerInterrupt(timer: Timer) -> None:
    global CENTS, READY_TO_PRINT, VALUES_TO_PRINT
    CENTS += 1
    if CENTS % 100 == 0:
        VALUES_TO_PRINT.enqueue(printTime(False))
        READY_TO_PRINT = True

async def buttonCoro(button: Pin, led: Pin) -> None:
    global CENTS, COUNTER_RUNNING
    print("Entered buttonTask()")
    print("Start stopwatch by pressing BOOT button")
    startCents: int = 0
    fms_state: State = State.IDLE
    timer: Timer = Timer(0)  # ESP32 has 4 timers: Timer ID ranges from 0 to 3
    while True:
        buttonValue = button.value()
        if fms_state == State.IDLE:
            led.off()
            if buttonValue == 0:  # Pressed
                state = disable_irq()  # Keep global variable access safe
                startCents = CENTS
                enable_irq(state)
                fms_state = State.LAP
            elif buttonValue == 1:  # Released
                pass
        elif fms_state == State.LAP:
            led.on()
            if not COUNTER_RUNNING:  # Start stopwatch
                timer.init(mode=Timer.PERIODIC, period=10, callback=handleTimerInterrupt)  # The timer will fire periodically each 10 ms, namely 1 cs
                COUNTER_RUNNING = True
            else:  # Get lap
                state = disable_irq()
                printAddCents()
                enable_irq(state)
                print("\t<-\tLap")
                printTime()
            if buttonValue == 0:  # Pressed
                fms_state = State.SHORT_PRESS
            elif buttonValue == 1:  # Released
                fms_state = State.IDLE
        elif fms_state == State.SHORT_PRESS:
            led.on()
            if buttonValue == 0:  # Pressed
                state = disable_irq()
                if (CENTS - startCents) < 0.5*100:
                    pass
                elif (CENTS - startCents) >= 0.5*100:
                    fms_state = State.STOP_AND_RESET
                enable_irq(state)
            elif buttonValue == 1:  # Released
                fms_state = State.IDLE
        elif fms_state == State.STOP_AND_RESET:
            led.off()  # Quick blink to confirm reset
            await uasyncio.sleep_ms(50)
            led.on()
            timer.deinit()
            state = disable_irq()
            CENTS = 0
            enable_irq(state)
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

async def printerCoro() -> None:
    global READY_TO_PRINT, VALUES_TO_PRINT
    while True:
        state = disable_irq()
        if READY_TO_PRINT:
            while not VALUES_TO_PRINT.is_empty():
                print(VALUES_TO_PRINT.dequeue(), end="")
        READY_TO_PRINT = False
        enable_irq(state)
        await uasyncio.sleep_ms(1)

async def main(buttonPin: Pin, ledPin: Pin) -> None:
    print("Entered main()")
    uasyncio.create_task(buttonCoro(buttonPin, ledPin))
    uasyncio.create_task(printerCoro())
    loop = uasyncio.get_event_loop()  # Loop forever, otherwise the program would exit
    loop.run_forever()
    print("Created coroutines")

if __name__ == "__main__":
    print("Setting up pins")
    ledPin = Pin(LED_PIN, Pin.OUT)
    buttonPin = Pin(BUTTON_PIN, Pin.IN, Pin.PULL_UP)
    uasyncio.run(main(buttonPin, ledPin))
