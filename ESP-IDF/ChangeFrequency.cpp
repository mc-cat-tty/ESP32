//
// File ChangeFrequency.cpp
// Author: Francesco Mecatti
// FSM - Finite State Machine - to change blu led (LED 2) blink frequency with BOOT button (BUTTON 0)
//

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
 
#define BLUELED (gpio_num_t) 2
#define BUTTON (gpio_num_t) 0
#define PAUSE_MAX 4096
#define PAUSE_MIN 1
#define NUM_STR_LEN 10

typedef enum {PRESSED, RELEASED} State;

int pause = 2;
int direction = 1;
State state = RELEASED;

void updateMonitor(int val){
    static bool firstRun = true;
    char newString[NUM_STR_LEN];
    itoa(val, newString, 10);
    for (int i = 0; i < NUM_STR_LEN and not firstRun; i++) printf("\b");
    printf(newString);
    for (int i = 0; i < NUM_STR_LEN-(int)strlen(newString); i++) printf(" ");
    firstRun = false;
}

void buttonTask(void *pvParameter){
    gpio_pad_select_gpio(BUTTON);
    gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
    printf("Pause: ");
    while(1) {
        int button = gpio_get_level(BUTTON);
        switch (state) {
            case RELEASED:
                if (button == PRESSED)
                    state = PRESSED;
                break;
            case PRESSED:
                if (button == RELEASED) {
                    if (pause == PAUSE_MAX || pause == PAUSE_MIN)
                        direction *= -1;
                    if (direction > 0)
                        pause *= 2;
                    else
                        pause /= 2;
                    updateMonitor(pause);
                    state = RELEASED;
                }
                break;
        }
        fflush(stdout);
        vTaskDelay(10 / portTICK_RATE_MS);
    }
}

void ledTask(void *pvParameter){
    gpio_pad_select_gpio(BLUELED);
    gpio_set_direction(BLUELED, GPIO_MODE_OUTPUT);
    while(1) {
        gpio_set_level(BLUELED, 0);
        vTaskDelay(pause / portTICK_RATE_MS);
        gpio_set_level(BLUELED, 1);
        vTaskDelay(pause / portTICK_RATE_MS);
  }
}
 
extern "C" {
    void app_main(void);
}

void app_main(void){
    xTaskCreate(&buttonTask, "buttonTask", 2048, NULL, 1, NULL);
    xTaskCreate(&ledTask, "ledTask", 1024, NULL, 1, NULL );
}