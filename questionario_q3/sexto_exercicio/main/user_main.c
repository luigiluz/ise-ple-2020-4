/* gpio example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "driver/i2c.h"

#include "esp_log.h"
#include "esp_system.h"

#include "LiquidCrystal_I2C.h"

static const char *TAG = "main";

#define DISPLAY_ADDRESS     0x27

typedef struct {
    char *msg;
    uint32_t msg_len;
    uint32_t cursor_row;
    uint32_t delay_period;
} task_param;

static task_param display_params;

SemaphoreHandle_t MutexHandle;

void init_display()
{
    LiquidCrystal_I2C(DISPLAY_ADDRESS, 16, 2);
    init();
    init();
    backlight();
    setCursor(0, 0);
}

void print_display(task_param *parameters) 
{
    int i;
    setCursor(0, parameters->cursor_row);

    for (i = 0; i < parameters->msg_len; i++) {
        writeLCD(parameters->msg[i]);
    }
}

void set_display_params(task_param *parameters)
{
    display_params.msg = parameters->msg;
    display_params.cursor_row = parameters->cursor_row;
    display_params.msg_len = parameters->msg_len;
    display_params.delay_period = parameters->delay_period;
}

static void display_task(void *pvParameters)
{
    task_param *parameters;
    parameters = (task_param *)(pvParameters);

    init_display();

    while(1) {
        ESP_LOGI(TAG, "Display task sendo executada");
        print_display(parameters);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    i2c_driver_delete(I2C_MASTER_NUM);
}

static void task_priority_example(void *pvParameters)
{
    task_param *param;

    param = (task_param *)(pvParameters); 

    while(1) {
        xSemaphoreTake(MutexHandle, portMAX_DELAY);
    
        ESP_LOGI(TAG, "%s", param->msg);
        vTaskDelay(pdMS_TO_TICKS(param->delay_period));

        set_display_params(param);

        xSemaphoreGive(MutexHandle);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    static const task_param task_1_params = {
        .msg = "Task 1 sendo executada",
        .msg_len = 16,
        .cursor_row = 0,
        .delay_period = 250
    };
    static const task_param task_2_params = {
        .msg = "Task 2 sendo executada",
        .msg_len = 16,
        .cursor_row = 0,
        .delay_period = 500
    };

    MutexHandle = xSemaphoreCreateMutex();

    xTaskCreate(task_priority_example, "task_priority_example1", 2048, (void *)&task_1_params, 2, NULL);
    xTaskCreate(task_priority_example, "task_priority_example2", 2048, (void *)&task_2_params, 2, NULL);
    xTaskCreate(display_task, "display_task", 2048, (void *)&display_params, 2, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


