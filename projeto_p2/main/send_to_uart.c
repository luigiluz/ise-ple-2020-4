#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/uart.h"

#include "esp_log.h"
#include "esp_system.h"

#include "send_to_uart.h"

static const char *TAG = "SEND_TO_UART";

#define BUF_SIZE (1024)

static QueueHandle_t SendToUartQueueHandle = NULL;
static SemaphoreHandle_t SendToUartSemphrHandle = NULL;

void send_to_uart_semph_init(void)
{
    if (SendToUartSemphrHandle == NULL)
        SendToUartSemphrHandle = xSemaphoreCreateMutex();
}

void send_to_uart_semphr_take(void)
{
    xSemaphoreTake(SendToUartSemphrHandle, portMAX_DELAY);
}

void send_to_uart_semphr_give(void)
{
    xSemaphoreGive(SendToUartSemphrHandle);
}

void send_to_uart_message_queue_init(void)
{
	if (SendToUartQueueHandle == NULL)
		SendToUartQueueHandle = xQueueCreate(10, sizeof(uart_msg_t));
};

BaseType_t send_to_uart_append_to_message_queue(uart_msg_t *msg)
{
	BaseType_t SendToUartQueueReturn = errQUEUE_FULL;

    send_to_uart_semphr_take();

    if (SendToUartQueueHandle != NULL) {
        SendToUartQueueReturn = xQueueSend(SendToUartQueueHandle, (void *)msg, portMAX_DELAY);
    }

    send_to_uart_semphr_give();

    return SendToUartQueueReturn;
}

void uart_init(void)
{
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

void send_to_uart_task(void *pvParameters)
{
    BaseType_t SendToUartQueueReturn;
    uart_msg_t received_msg;

    while(1) {
        SendToUartQueueReturn = xQueueReceive(SendToUartQueueHandle, (void *)&received_msg, portMAX_DELAY);

        if (SendToUartQueueReturn == pdPASS) {
            uart_write_bytes(UART_NUM_0, (const char *) received_msg.msg, received_msg.msg_len);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
