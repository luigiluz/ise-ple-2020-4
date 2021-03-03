static const char *TAG = "APPLICATION";

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "driver/hw_timer.h"

#include "esp_log.h"
#include "esp_system.h"

#include "keypad.h"
#include "send_to_uart.h"
#include "display.h"
#include "get_from_uart.h"
#include "read_adc.h"
#include "mqtt_task.h"

BaseType_t xKeypadReturned;
BaseType_t xSendToUartReturned;
BaseType_t xDisplayReturned;
BaseType_t xGetFromUartReturned;
BaseType_t xReadAdcReturned;
BaseType_t xMQTTReturned;

TaskHandle_t xKeypadHandle = NULL;
TaskHandle_t xSendToUartHandle = NULL;
TaskHandle_t xDisplayHandle = NULL;
TaskHandle_t xGetFromUartHandle = NULL;
TaskHandle_t xReadAdcHandle = NULL;
TaskHandle_t xMQTTHandle = NULL;

/*
 * @brief: app_main contem o core da aplicacao
 *
 */
void app_main(void)
{   
    ESP_LOGI(TAG, "Inicio da aplicacao");
    display_semph_init();
    mqtt_task_semph_init();
    display_message_queue_init();
    mqtt_task_message_queue_init();
	keypad_init();
    uart_init();
    read_adc_init();
	
    xKeypadReturned = xTaskCreate(keypad_task, "KeypadTask", 1024 + 256, NULL, 1, &xKeypadHandle);
    if (xKeypadReturned == pdPASS) {
    	ESP_LOGI(TAG, "KeypadTask foi criada com sucesso");
    } else {
        ESP_LOGI(TAG, "KeypadTask nao foi inicializada");
    }

    xDisplayReturned = xTaskCreate(display_task, "DisplayTask", 2*1024 + 256, NULL, 1, &xDisplayHandle);
    if (xDisplayReturned == pdPASS) {
        ESP_LOGI(TAG, "DisplayTask foi criada com sucesso");
    } else {
        ESP_LOGI(TAG, "DisplayTask nao foi inicializada");
    }

    xReadAdcReturned = xTaskCreate(read_adc_task, "ReadAdcTask", 1024, NULL, 2, &xReadAdcHandle);
    if (xReadAdcReturned == pdPASS) {
        ESP_LOGI(TAG, "ReadAdc foi criada com sucesso");
    } else {
        ESP_LOGI(TAG, "ReadAdc nao foi inicializada");
    }

    xMQTTReturned = xTaskCreate(mqtt_task, "MQTT_Task", 2*1024 + 256, NULL, 2, &xMQTTHandle);
    if (xMQTTReturned == pdPASS) {
        ESP_LOGI(TAG, "MQTT_Task foi criada com sucesso");
    } else {
        ESP_LOGI(TAG, "MQTT_Task nao foi inicializada");
    }

    while(1) {
    	vTaskDelay(1000 / portTICK_RATE_MS);
    }
    
}


