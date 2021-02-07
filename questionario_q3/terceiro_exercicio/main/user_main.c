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
#include "freertos/queue.h"

#include "driver/gpio.h"

#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "main";

typedef struct {
    char *msg;
    uint32_t delay_period;
} task_param;

static void task_priority_example(void *pvParameters)
{
    task_param *param;

    param = (task_param *)(pvParameters); 

    while(1) {
        ESP_LOGI(TAG, "%s", param->msg);
        vTaskDelay(pdMS_TO_TICKS(param->delay_period));
    }
}

void app_main(void)
{
    static const task_param task_1_params = {
        .msg = "Task 1 sendo executada",
        .delay_period = 250
    };
    static const task_param task_2_params = {
        .msg = "Task 2 sendo executada",
        .delay_period = 500
    };
    static const task_param task_3_params = {
        .msg = "Task 3 sendo executada",
        .delay_period = 1000
    };

    xTaskCreate(task_priority_example, "task_priority_example1", 2048, (void *)&task_1_params, 2, NULL);
    xTaskCreate(task_priority_example, "task_priority_example2", 2048, (void *)&task_2_params, 2, NULL);
    xTaskCreate(task_priority_example, "task_priority_example3", 2048, (void *)&task_3_params, 1, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


