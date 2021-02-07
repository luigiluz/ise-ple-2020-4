// Copyright 2018-2025 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/uart.h"

static const char *TAG = "UART_QUEUE";
#define UART_QUEUE_LEN         3
#define BUF_SIZE               1024

typedef struct {
    uint8_t id;
    char msg[16];
} queue_msg;

QueueHandle_t UartQueueHandle = NULL;

void init_uart()
{
    // Configure parameters of an UART driver,
    // communication pins and install the driver
    uart_config_t uart_config = {
        .baud_rate = 74880,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, 0);
}

static void uart_task(void *pvParameters)
{
    while (1) {
        ESP_LOGI(TAG, "Executando a uart_task");
        char temp[43];
        int n;
        queue_msg queue;
        BaseType_t UartQueueReceive = pdFAIL;
        
        n = strlen(temp);
    
        UartQueueReceive = xQueueReceive(UartQueueHandle, (void *)&queue, 0);

        if (UartQueueReceive == pdPASS) {
            ESP_LOGI(TAG, "queue.id: %d", queue.id);
            ESP_LOGI(TAG, "queue.msg: %s", queue.msg);
            sprintf(temp, "queue.id: %d, queue.msg: %s", queue.id, queue.msg);
            uart_write_bytes(UART_NUM_0, (const char *) temp, n);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void default_task(void *pvParameters)
{
    queue_msg *queue;
    queue = (queue_msg *)(pvParameters);

    while(1) {
        ESP_LOGI(TAG, "Executando a default_task");
        if (UartQueueHandle != NULL) {
            xQueueSend(UartQueueHandle, (void *)queue, pdMS_TO_TICKS(100));
        }
        vTaskDelay(pdMS_TO_TICKS    (1000));
    }
}

void app_main()
{
    init_uart();

    static const queue_msg queue1 = {
        .id = 1,
        .msg = "Lendo da task 1\n"
    };
    static const queue_msg queue2 = {
        .id = 2,
        .msg = "Lendo da task 2\n"
    };
    static const queue_msg queue3 = {
        .id = 3,
        .msg = "Lendo da task 3\n"
    };

    UartQueueHandle = xQueueCreate(UART_QUEUE_LEN, sizeof(queue_msg));

    xTaskCreate(uart_task, "uart_task", 1024, NULL, 1, NULL);
    xTaskCreate(default_task, "default_task_1", 1024, (void *)&queue1, 1, NULL);
    xTaskCreate(default_task, "default_task_2", 1024, (void *)&queue2, 1, NULL);
    xTaskCreate(default_task, "default_task_3", 1024, (void *)&queue3, 1, NULL);
}
