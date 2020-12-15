//
// File DimmerPWM.cpp
// Author: Francesco Mecatti
// Blue led (LED 2) dimmering through PWM - Pulse Width Modulation -. Use BUTTON 0 to control led brightness.
//

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
 
#define BLUELED (gpio_num_t)    2
#define BUTTON (gpio_num_t)     0
#define PERIOD                  16  // ms
#define NUM_STR_LEN             10

typedef enum {PRESSED, RELEASED} State;

double duty_cycle = 1;  // always between 0 and 1
int direction = 1;

void updateMonitor(double val){
    static bool firstRun = true;
    char newString[NUM_STR_LEN];
    itoa((int) ((float)val*100), newString, 10);
    for (int i = 0; i < NUM_STR_LEN and not firstRun; i++) printf("\b");
    printf(newString);
    for (int i = 0; i < NUM_STR_LEN-(int)strlen(newString); i++) printf(" ");
    firstRun = false;
}

void buttonTask(void *pvParameter){
    gpio_pad_select_gpio(BUTTON);
    gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
    printf("Duty cycle [%%]: ");  // "%%" is an escaped "%"
    while(1) {
        int button = gpio_get_level(BUTTON);
        if (button == PRESSED) {
            if (duty_cycle <= 0 || duty_cycle >= 1)
                direction *= -1;
            if (direction == 1)
                duty_cycle += 0.01;
            if (direction == -1)
                duty_cycle -= 0.01;
            updateMonitor(duty_cycle);
            fflush(stdout);
        }
        vTaskDelay(10 / portTICK_RATE_MS);
    }
}

void ledTask(void *pvParameter){
    gpio_pad_select_gpio(BLUELED);
    gpio_set_direction(BLUELED, GPIO_MODE_OUTPUT);
    while(1) {
        gpio_set_level(BLUELED, 1);
        vTaskDelay((double)duty_cycle*PERIOD / portTICK_RATE_MS);
        gpio_set_level(BLUELED, 0);
        vTaskDelay((1-(double)duty_cycle)*PERIOD / portTICK_RATE_MS);
  }
}
 
extern "C" {
    void app_main(void);
}

void app_main(void){
    xTaskCreate(&buttonTask, "buttonTask", 2048, NULL, 1, NULL);
    xTaskCreate(&ledTask, "ledTask", 1024, NULL, 1, NULL);
}