#include "driver/i2c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "LiquidCrystal_I2C.h"

#include "display.h"

#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "DISPLAY";

#define DISPLAY_ADDRESS     0x27

static QueueHandle_t DisplayMessageQueueHandle = NULL;
static SemaphoreHandle_t DisplaySemphrHandle = NULL;

void init_display_semph(void)
{
    if (DisplaySemphrHandle == NULL)
        DisplaySemphrHandle = xSemaphoreCreateMutex();
}

void display_semphr_take(void)
{
    xSemaphoreTake(DisplaySemphrHandle, portMAX_DELAY);
}

void display_semphr_give(void)
{
    xSemaphoreGive(DisplaySemphrHandle);
}

void init_display_message_queue(void)
{
	if (DisplayMessageQueueHandle == NULL)
		DisplayMessageQueueHandle = xQueueCreate(2, sizeof(display_params));
}

BaseType_t send_to_display_message_queue(display_params *params_to_send)
{
	BaseType_t DisplayQueueReturn = errQUEUE_FULL;

    if (DisplayMessageQueueHandle != NULL) {
        DisplayQueueReturn = xQueueSend(DisplayMessageQueueHandle, (void *)params_to_send, portMAX_DELAY);
    }

	return DisplayQueueReturn;
}

void display_task(void *pvParameters)
{
    // char ch[11] = "projeto_p2\n";
    LiquidCrystal_I2C(DISPLAY_ADDRESS, 16, 2);
    init();
    init();
    backlight();
    // setCursor(0, 0);

    // int i = 0;
    // while(1) {
    //     if (ch[i] == '\n')
    //         break;
    //     writeLCD(ch[i]);
    //     i++;
    // }

    while(1) {
        BaseType_t GetFromDisplayQueueReturn;
        display_params params;

        GetFromDisplayQueueReturn = xQueueReceive(DisplayMessageQueueHandle, (void *)&params, pdMS_TO_TICKS(100));
        
        ESP_LOGI(TAG, "Messages waiting: %d", uxQueueMessagesWaiting(DisplayMessageQueueHandle));

        if (GetFromDisplayQueueReturn == pdPASS) {
            //clear();
            // ESP_LOGI(TAG, "Mensagem recebida na fila do display");
            // ESP_LOGI(TAG, "params.cursor_row: %d", params.cursor_row);
            // ESP_LOGI(TAG, "params.cursor_col: %d", params.cursor_col);
            // ESP_LOGI(TAG, "params.msg: %s", params.msg);
            // ESP_LOGI(TAG, "params.msg_len: %d", params.msg_len);

            setCursor(params.cursor_col, params.cursor_row);

            for (int i=0; i < params.msg_len; i++) {
                // ESP_LOGI(TAG, "params.msg[%d] = %c", i, params.msg[i]);
                if (params.msg[i] == '\0')
                    break;
                writeLCD(params.msg[i]);
            }
        }

        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    i2c_driver_delete(I2C_MASTER_NUM);
}

