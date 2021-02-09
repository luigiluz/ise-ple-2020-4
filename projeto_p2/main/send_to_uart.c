#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"

#include "esp_log.h"
#include "esp_system.h"

#include "send_to_uart.h"

static const char *TAG = "SEND_TO_UART";

/**
 * This is an example which echos any data it receives on UART0 back to the sender,
 * with hardware flow control turned off. It does not use UART driver event queue.
 *
 * - Port: UART0
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: off
 */

#define BUF_SIZE (1024)

static QueueHandle_t SendToUartQueueHandle;

void init_send_to_uart_queue(void)
{
	if (SendToUartQueueHandle == NULL)
		SendToUartQueueHandle = xQueueCreate(1, sizeof(uart_msg));
};

void append_to_send_to_uart_queue(uart_msg *msg)
{
    xQueueSend(SendToUartQueueHandle, (void *)msg, 0);
}

void uart_init(void)
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

void send_to_uart_task(void *pvParameters)
{
    BaseType_t SendToUartQueueReturn;
    uart_msg received_msg;

    while(1) {
        // ESP_LOGI(TAG, "Executando a send_to_uart_task");
        // SendToUartQueueReturn = xQueueReceive(SendToUartQueueHandle, (void *)&received_msg, portMAX_DELAY);

        // if (SendToUartQueueReturn == pdPASS) {
        //     ESP_LOGI(TAG, "informacao lida da queue da send_to_uart");

        //     //sprintf(json_receive_buffer, "{ \"key_1\": \"%c\", \"key_2\": \"%c\" }\n", receive_buffer[0], receive_buffer[1]);
        //     uart_write_bytes(UART_NUM_0, (const char *) received_msg.msg, sizeof(received_msg.msg));
        // }

        // TODO: fazer o parse dessas para o formato json
        // especificar o formato dessas mensagens: nome dos key e values
        // verificar o tamanho dessas mensagens
        // TODO: enviar a mensagem parseada via uart
        // uart_write_bytes(UART_NUM_0, (const char *) data, len);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
