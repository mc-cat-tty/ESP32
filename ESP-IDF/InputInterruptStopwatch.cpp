//
// InputInterruptStopwatch.cpp
// Author: Francesco Mecatti
// Stopwatch able to distinguish between short and long touch. 
// This program provides a wide variety of input systems: button, touch pin and Hall-effect sensor. Interrupt-driven events management
// Additional feature: ANSI/VT100 formatting
//

#include <string>
#include <stdio.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "driver/touch_pad.h"
#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_intr_alloc.h"

// Configuration section. Set to 1 if you want to enable that input device, 0 otherwise
#define USE_BUTTON      (1)
#define USE_TOUCHPAD    (1)
#define USE_HALLSENSOR  (0)

// Touchpad configuration parameters
#define TOUCHPAD_FILTER_PERIOD          (10)
// #define TOUCHPAD_THRESH_NO_USE       (150)
#define TOUCHPAD_THRESH_NO_USE_HIGH     (550)
#define TOUCHPAD_THRESH_NO_USE_LOW      (150)

// Hall-effect sensor config parameters
#define HALL_MIN_THRESH_NO_USE (10)
#define HALL_MAX_THRESH_NO_USE (40)

// Pin definition
#define BUTTON_PIN  (gpio_num_t)    (0)
#define LED_PIN     (gpio_num_t)    (2)
#define TOUCH_PIN   (touch_pad_t)   (4)  // Touch0

// True if one of the input is true
#define eval_input(button_query, touchpad_query, hallsensor_query)  (   (USE_BUTTON && buttonState == button_query) or \
                                                                        (USE_TOUCHPAD && touchState == touchpad_query) or \
                                                                        (USE_HALLSENSOR && (hall_sensor_read() < HALL_MIN_THRESH_NO_USE || hall_sensor_read() > HALL_MAX_THRESH_NO_USE) == hallsensor_query)    )

using namespace std;

typedef enum {PRESSED, RELEASED} InputState;
typedef enum {  FIRST_PRESS,
                WAITING_RELEASE
                } FSMState;
typedef enum {OFF, ON} LedState;

typedef unsigned long int ctime_t;

SemaphoreHandle_t xSemaphore = nullptr;


class Time {
    public:
        static const unsigned int CS_FACTOR = 100;
        static const unsigned int SS_FACTOR = 60;
        static const unsigned int MM_FACTOR = 60;
        static inline ctime_t centiseconds = 0;
        static inline unsigned int hh = 0, mm = 0, ss = 0, cs = 0;
        TaskHandle_t xCounterTaskHandle = NULL;
        pair<uint8_t, uint8_t> lastLapPosition {0, 10};  // Cursor position of last printed lap (row, col)
        bool stopped = true;

        // Instance constructor
        Time() {
            printf("\e[2J\e[H");  // ANSI Escape sequence to erase display and move the cursor to the home position
        }

        // Get centiseconds attribute
        ctime_t getCentiseconds() {
            return centiseconds;
        }

        // Print new time value over the old one
        static void updateTime(void) {
            for (int i = 0; i < 6+2; i++) printf("\b");
            computeTime();
            printf("\e[?25l\e[104m%02u:%02u:%02u\e[0m", hh, mm, ss);  // ANSI Escape characters hide cursor and change background color; after time print restore default graphics style
            fflush(stdout);
        }

        // Prettify laps visualization
        void addLap(void) {
            printf("\e[s");  // Save cursor position
            for (int i = 0; i < lastLapPosition.second; i++) printf("\e[1C");  // Move the cursor forward by 10 columns (keeping cursor hide)
            for (int i = 0; i < lastLapPosition.first+1; i++) printf("\e[1B");  // Move the cursor down by N rows (keeping cursor hide)
            computeTime();
            printf("\e[?25l(%d)\t%02u:%02u:%02u.%02u", lastLapPosition.first+1, hh, mm, ss, cs);  // Hide cursor and print lap time
            printf("\e[u");  // Restore cursor position
            fflush(stdout);
            lastLapPosition.first++;
        }

        // This method turns centiSeconds into hh, mm, ss and cs
        static void computeTime(void) {
            hh = centiseconds / (CS_FACTOR*SS_FACTOR*MM_FACTOR);
            mm = (centiseconds - hh*(CS_FACTOR*SS_FACTOR*MM_FACTOR)) / (CS_FACTOR*SS_FACTOR);
            ss = (centiseconds - hh*(CS_FACTOR*SS_FACTOR*MM_FACTOR) - mm*(CS_FACTOR*SS_FACTOR)) / (CS_FACTOR);
            cs = (centiseconds - hh*(CS_FACTOR*SS_FACTOR*MM_FACTOR) - mm*(CS_FACTOR*SS_FACTOR) - ss*(CS_FACTOR));
        }

        // Task called every centisecond
        static void counterTask(void *pvParameters) {
            while (true) {
                centiseconds++;
                if (centiseconds % 100 == 0) {  // Print the time every 10 centiceconds
                    updateTime();
                }
                vTaskDelay(10 / portTICK_PERIOD_MS);  // 10 ms, namely 1 cs
            }
        }

        // Run this method to start counter
        void start(void) {
            stopped = false;
            xTaskCreate(&counterTask, "counterTask", 2048, NULL, 1, &xCounterTaskHandle);
        }

        void stop(void) {
            vTaskDelete(xCounterTaskHandle);
            stopped = true;
        }

        bool isStopped(void) {
            return stopped;
        }

        void reset(void) {  // Reset counter and show update time (00:00:00)
            centiseconds = 0;
            computeTime();
            updateTime();
        }

        void clearLaps(void) {
            printf("\e[s");  // Save cursor position
            for (int i = 0; i < lastLapPosition.second; i++) printf("\e[1C");  // Move the cursor forward by 10 columns (keeping cursor hide)
            for (int i = 0; i < lastLapPosition.first+1; i++) {
                printf("\e[1B");  // Move the cursor down by N rows (keeping cursor hide)
                printf("\e[K");  // Erase line
            }
            printf("\e[u");  // Restore cursor position
            fflush(stdout);
            lastLapPosition.first = 0;
        }

        // Destructor
        ~ Time() {
            printf("\e[0m");  // ANSI Escape sequence to set all graphics attributes off
            stop();
        }
};

typedef struct {
    Time t;
    SemaphoreHandle_t semaphore;
} task_data_t;

#if USE_BUTTON
InputState buttonState;

// Negative and positive edge interrupt handler (triggered when pressed or released)
void buttonIsrHandler(void *pvParameters) {
    buttonState = (InputState) gpio_get_level(BUTTON_PIN);
    xSemaphoreGiveFromISR(xSemaphore, NULL);
}
#endif

#if USE_TOUCHPAD
InputState touchState;

// Negative edge interrupt handler (triggered while pressing)
void touchIsrHandler(void *pvParameters) {
    touchState = (InputState) ((touch_pad_get_status() & BIT4) >> 4);
    touch_pad_clear_status();
    xSemaphoreGiveFromISR(xSemaphore, NULL);
}
#endif

// FSM to detect long and short press
void buttonTask(void *pvParameters) {
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

#if USE_BUTTON  // Button configuration
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_intr_type(BUTTON_PIN, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    gpio_isr_handler_add(BUTTON_PIN, buttonIsrHandler, NULL);
#endif

#if USE_TOUCHPAD  // Touchpad configuration
    touch_pad_init();
    touch_pad_config(TOUCH_PIN, TOUCHPAD_THRESH_NO_USE_LOW);
    touch_pad_filter_start(TOUCHPAD_FILTER_PERIOD);
    touch_pad_set_trigger_mode(TOUCH_TRIGGER_BELOW);
    touch_pad_isr_register(touchIsrHandler, NULL);
    touch_pad_intr_enable();
#endif

    FSMState state = FIRST_PRESS;
    ctime_t startCentiseconds;
    task_data_t *data = (task_data_t *) pvParameters;
    Time t = data->t;
    SemaphoreHandle_t xSemaphore = data->semaphore;

    gpio_set_level(LED_PIN, (int) OFF);

    // puts("Entered buttonTask");
    while (true) {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
            switch (state) {
                case FIRST_PRESS:
                    if (eval_input(PRESSED, 1, 1)) {
                        gpio_set_level(LED_PIN, (int) ON);
                        if (t.isStopped()) {
                            t.start();
                            t.clearLaps();
                        }
                        else {
                            t.addLap();
                        }
                        startCentiseconds = t.getCentiseconds();
                        state = WAITING_RELEASE;
#if USE_TOUCHPAD
                        touch_pad_config(TOUCH_PIN, TOUCHPAD_THRESH_NO_USE_HIGH);
                        touch_pad_set_trigger_mode(TOUCH_TRIGGER_ABOVE);
#endif
                    }
                    else {
                        ;;  // Do nothing
                    }
                    break;
                case WAITING_RELEASE:
                    if (eval_input(RELEASED, 0, 0)) {
                        gpio_set_level(LED_PIN, (int) OFF);
                        // Long press branch
                        if ((t.getCentiseconds() - startCentiseconds) >= 0.5*100) {  // 0.5 secs; comparison in centisecs
                            t.stop(); t.reset();
                        }
                        state = FIRST_PRESS;
#if USE_TOUCHPAD
                        touch_pad_config(TOUCH_PIN, TOUCHPAD_THRESH_NO_USE_LOW);
                        touch_pad_set_trigger_mode(TOUCH_TRIGGER_BELOW);
#endif
                    }
                    else {
                        ;;  // Do nothing
                    }
                    break;
            }
        }
    }
}


extern "C" {
    void app_main(void);
}

void app_main(void) {
    Time t = Time();
    xSemaphore = xSemaphoreCreateBinary();
    task_data_t data = {t, xSemaphore};
    xTaskCreate(&buttonTask, "buttonTask", 2048, (void *) &data, 1, NULL);
}