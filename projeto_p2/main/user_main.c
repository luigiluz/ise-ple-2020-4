/* Projeto envolvendo comunicacao serial 
 * Data: 23/01/2020
 * Autor: Luigi Luz
 */

/* Arquitetura do sistema
 * O sistema ira possuir 4 tarefas que sao:
 * - DisplayTask
 *   Objetivo: 
 *             Apresentar no display as informacoes recebidas
 *
 * - KeyboardTask
 *   Objetivo: 
 *             Ler as informacoes digitadas no teclado
 *
 * - GetUartTask
 *   Objetivo: 
 *             Processar as informacoes recebidas pela UART
 *             Recuperar a informacao formata em formato JSON
 *             Enviar a informacao para o display
 *
 * - SendUartTask
 *   Objetivo: 
 *             Construir mensagem em formato JSON com os dados do keypad
 *             Enviar a mensagem via UART
 */

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

BaseType_t xKeypadReturned;
BaseType_t xSendToUartReturned;
BaseType_t xDisplayReturned;
BaseType_t xGetFromUartReturned;
BaseType_t xReadAdcReturned;

TaskHandle_t xKeypadHandle = NULL;
TaskHandle_t xSendToUartHandle = NULL;
TaskHandle_t xDisplayHandle = NULL;
TaskHandle_t xGetFromUartHandle = NULL;
TaskHandle_t xReadAdcHandle = NULL;

/*
 * @brief: app_main contem o core da aplicacao
 *
 */
void app_main(void)
{   
    ESP_LOGI(TAG, "Inicio da aplicacao");
	keypad_init();
    uart_init();
    display_message_queue_init();
    read_adc_init();
    send_to_uart_message_queue_init();
    display_semph_init();
    send_to_uart_semph_init();
	
    xKeypadReturned = xTaskCreate(keypad_task, "KeypadTask", 1024 + 256, NULL, 1, &xKeypadHandle);
    if (xKeypadReturned == pdPASS) {
    	ESP_LOGI(TAG, "KeypadTask foi criada com sucesso");
    } else {
        ESP_LOGI(TAG, "KeypadTask nao foi inicializada");
    }
    
    xSendToUartReturned = xTaskCreate(send_to_uart_task, "SendToUartTask", 1024, NULL, 2, &xSendToUartHandle);
    if (xSendToUartReturned == pdPASS) {
        ESP_LOGI(TAG, "SendToUartTask foi criada com sucesso");
    } else {
        ESP_LOGI(TAG, "SendToUartTask nao foi inicializada");
    }

    xDisplayReturned = xTaskCreate(display_task, "DisplayTask", 1024, NULL, 1, &xDisplayHandle);
    if (xDisplayReturned == pdPASS) {
        ESP_LOGI(TAG, "DisplayTask foi criada com sucesso");
    } else {
        ESP_LOGI(TAG, "DisplayTask nao foi inicializada");
    }

    xGetFromUartReturned = xTaskCreate(get_from_uart_task, "GetFromUartTask", 2*1024, NULL, 1, &xDisplayHandle);
    if (xGetFromUartReturned == pdPASS) {
        ESP_LOGI(TAG, "GetFromUart foi criada com sucesso");
    } else {
        ESP_LOGI(TAG, "GetFromUart nao foi inicializada");
    }

    xReadAdcReturned = xTaskCreate(read_adc_task, "ReadAdcTask", 1024, NULL, 2, &xReadAdcHandle);
    if (xReadAdcReturned == pdPASS) {
        ESP_LOGI(TAG, "ReadAdc foi criada com sucesso");
    } else {
        ESP_LOGI(TAG, "ReadAdc nao foi inicializada");
    }

    while(1) {
    	vTaskDelay(1000 / portTICK_RATE_MS);
    }
    
}


