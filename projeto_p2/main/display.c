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

void display_semph_init(void)
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

void display_message_queue_init(void)
{
	if (DisplayMessageQueueHandle == NULL)
		DisplayMessageQueueHandle = xQueueCreate(2, sizeof(display_msg_t));
}

BaseType_t display_append_to_message_queue(display_msg_t *display_msg)
{
    display_semphr_take();

	BaseType_t DisplayQueueReturn = errQUEUE_FULL;

    if (DisplayMessageQueueHandle != NULL) {
        DisplayQueueReturn = xQueueSend(DisplayMessageQueueHandle, (void *)display_msg, portMAX_DELAY);
    }

    display_semphr_give();

	return DisplayQueueReturn;
}

void display_task(void *pvParameters)
{
    LiquidCrystal_I2C(DISPLAY_ADDRESS, 16, 2);
    init();
    init();
    backlight();

    while(1) {
        BaseType_t GetFromDisplayQueueReturn;
        display_msg_t display_msg;
        static uint16_t prev_msg_len = 0;

        GetFromDisplayQueueReturn = xQueueReceive(DisplayMessageQueueHandle, (void *)&display_msg, pdMS_TO_TICKS(100));

        if (GetFromDisplayQueueReturn == pdPASS) {
            setCursor(display_msg.cursor_col, display_msg.cursor_row);

            if (display_msg.tsk_id == GET_FROM_UART_TASK_ID) {
                uint16_t msg_len = 1;
                /* Routine to get the msg_len */
                for (int k = 0; k < display_msg.msg_len; k++) {
                    if(display_msg.msg[k] == '\0')
                        break;

                    msg_len++;
                }

                /* Clean display characters if current msg is shorter than the previous one */
                if (msg_len < prev_msg_len) {
                    for (int i = 0; i < display_msg.msg_len; i++) {
                        writeLCD(' ');
                    }
                }
                setCursor(display_msg.cursor_col, display_msg.cursor_row);
                prev_msg_len = msg_len;
            }

            for (int i=0; i < display_msg.msg_len; i++) {
                if (display_msg.msg[i] == '\0')
                    break;
                writeLCD(display_msg.msg[i]);
            }
        }

        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    i2c_driver_delete(I2C_MASTER_NUM);
}

