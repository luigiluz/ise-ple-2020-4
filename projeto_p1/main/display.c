#include "driver/i2c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "LiquidCrystal_I2C.h"

#include "get_from_uart.h"

#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "DISPLAY";

#define DISPLAY_ADDRESS     0x27

void display_task(void *pvParameters)
{
    char ch[13] = "application\n";
    LiquidCrystal_I2C(DISPLAY_ADDRESS, 16, 2);
    init();
    init();
    backlight();
    setCursor(0, 0);

    int i = 0;
    while(1) {
        if (ch[i] == '\n')
            break;
        writeLCD(ch[i]);
        i++;
    }

    while(1) {
        BaseType_t GetFromUartQueueReturn;
        display_msg received_msg;

        GetFromUartQueueReturn = get_from_uart_read_queue(&received_msg);
        
        if (GetFromUartQueueReturn == pdPASS) {
            clear();
            ESP_LOGI(TAG, "Mensagem recebida da get_from_uart_read_queue");
            ESP_LOGI(TAG, "received_msg.cursor: %d", received_msg.cursor);
            ESP_LOGI(TAG, "received_msg.msg: %s", received_msg.msg);
            setCursor(received_msg.cursor, 0);
            for (int i=0; i < DISPLAY_MSG_MAX_LEN; i++) {
                ESP_LOGI(TAG, "received_msg.msg[%d] = %c", i, received_msg.msg[i]);
                if (received_msg.msg[i] == '\0')
                    break;
                writeLCD(received_msg.msg[i]);
            }
        }

        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    i2c_driver_delete(I2C_MASTER_NUM);
}

