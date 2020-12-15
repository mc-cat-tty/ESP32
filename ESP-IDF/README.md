# ESP-IDF

Code examples of ESP-IDF framework, used in combination with FreeRTOS

## Setting up

### _platformio.ini_ configuration

```
[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = espidf
monitor_speed = 115200
monitor_flags = --raw
build_flags =
    -std=c++17
    -std=gnu++17
build_unflags =
    -std=gnu++11
```

### Change TICK_RATE
File: .pio/build/esp32doit-devkit-v1/config/sdkconfig.h
Change _CONFIG_FREERTOS_HZ_ from 100 to 1000