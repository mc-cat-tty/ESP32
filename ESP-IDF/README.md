# ESP-IDF

Code examples of ESP-IDF framework, used in combination with FreeRTOS

## Setting up

### Add to _platformio.ini_

```
[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = espidf
monitor_speed = 115200
monitor_flags = --raw
```

### Change TICK_RATE
File: .pio/build/esp32doit-devkit-v1/config/sdkconfig.h
Change _CONFIG_FREERTOS_HZ_ from 100 to 1000