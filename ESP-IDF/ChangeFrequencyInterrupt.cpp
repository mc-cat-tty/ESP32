//
// File ChangeFrequencyInterrupt.cpp
// Author: Francesco Mecatti
// Button press is managed through an ISR triggered by a falling (negative = while pressing) edge
//

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_intr_alloc.h"
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
SemaphoreHandle_t xSemaphore = nullptr;

void updateMonitor(int val){
    static bool firstRun = true;
    char newString[NUM_STR_LEN];
    itoa(val, newString, 10);
    for (int i = 0; i < NUM_STR_LEN and not firstRun; i++) printf("\b");
    printf(newString);
    for (int i = 0; i < NUM_STR_LEN-(int)strlen(newString); i++) printf(" ");
    firstRun = false;
}

void buttonIsrHandler (void *pvParameters) {  // Negative edge interrupt handler (triggered while pressing)
    xSemaphoreGiveFromISR(xSemaphore, NULL);
}

void buttonTask(void *pvParameters){
    gpio_pad_select_gpio(BUTTON);
    gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
    gpio_set_intr_type(BUTTON, GPIO_INTR_NEGEDGE);
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    gpio_isr_handler_add(BUTTON, buttonIsrHandler, NULL);
    printf("Pause: "); fflush(stdout);
    while(1) {
        if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
            if (pause == PAUSE_MAX || pause == PAUSE_MIN)
                direction *= -1;
            if (direction > 0)
                pause *= 2;
            else
                pause /= 2;
            updateMonitor(pause);
            fflush(stdout);
        }
    }
}

void ledTask(void *pvParameters){
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
    xSemaphore = xSemaphoreCreateBinary();
    xTaskCreate(&buttonTask, "buttonTask", 2048, NULL, 1, NULL);
    xTaskCreate(&ledTask, "ledTask", 1024, NULL, 1, NULL );
}