#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"

#include "esp_log.h"
#include "esp_system.h"

#include "keypad.h"

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

void send_to_uart_task(void)
{
    // Configure a temporary buffer for the incoming data
    char receive_buffer[BUFFER_SIZE];
    BaseType_t KeypadQueueReturn;

    while(1) {
        KeypadQueueReturn = keypad_read_from_queue(receive_buffer);
        char json_receive_buffer[32];

        if (KeypadQueueReturn == pdPASS) {
            ESP_LOGI(TAG, "informacao lida da queue do keypad");

            sprintf(json_receive_buffer, "{ \"key_1\": \"%c\", \"key_2\": \"%c\" }\n", receive_buffer[0], receive_buffer[1]);
            uart_write_bytes(UART_NUM_0, (const char *) json_receive_buffer, sizeof(json_receive_buffer));
        }

        // TODO: fazer o parse dessas para o formato json
        // especificar o formato dessas mensagens: nome dos key e values
        // verificar o tamanho dessas mensagens
        // TODO: enviar a mensagem parseada via uart
        // uart_write_bytes(UART_NUM_0, (const char *) data, len);
    }
}
