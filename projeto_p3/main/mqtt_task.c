#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "mqtt_client.h"

#include "wifi_config.h"
#include "mqtt_task.h"

static const char *TAG = "MQTT_TASK";

#define MQTT_HOST                   "192.168.1.112"
#define MQTT_PORT                   1883

#define MQTT_INIT_TOPIC             "/mqtt_init_topic"
#define READ_ADC_TOPIC              "/read_adc"
#define KEYPAD_TOPIC                "/keypad"

#define MQTT_INIT_MSG               "Connection stablished"

static QueueHandle_t MQTTQueueHandle = NULL;
static SemaphoreHandle_t MQTTSemphrHandle = NULL;

esp_mqtt_client_handle_t client;

void mqtt_task_semph_init(void)
{
    if (MQTTSemphrHandle == NULL)
        MQTTSemphrHandle = xSemaphoreCreateMutex();
}

void mqtt_semphr_take(void)
{
    xSemaphoreTake(MQTTSemphrHandle, portMAX_DELAY);
}

void mqtt_semphr_give(void)
{
    xSemaphoreGive(MQTTSemphrHandle);
}

void mqtt_task_message_queue_init(void)
{
	if (MQTTQueueHandle == NULL)
		MQTTQueueHandle = xQueueCreate(10, sizeof(mqtt_msg_t));
};

BaseType_t mqtt_task_append_to_message_queue(mqtt_msg_t *msg)
{
	BaseType_t MQTTQueueReturn = errQUEUE_FULL;

    mqtt_semphr_take();

    if (MQTTQueueHandle != NULL) {
        MQTTQueueReturn = xQueueSend(MQTTQueueHandle, (void *)msg, portMAX_DELAY);
    }

    mqtt_semphr_give();

    return MQTTQueueReturn;
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t tmp_client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_publish(tmp_client, MQTT_INIT_TOPIC, MQTT_INIT_MSG, 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

            // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            // msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            // ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            // msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        // case MQTT_EVENT_UNSUBSCRIBED:
        //     ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        //     break;
        // case MQTT_EVENT_PUBLISHED:
        //     ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        //     break;
        // case MQTT_EVENT_DATA:
        //     ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        //     printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        //     printf("DATA=%.*s\r\n", event->data_len, event->data);
        //     break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .host = MQTT_HOST,
        .port = MQTT_PORT,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void mqtt_task(void *pvParameters)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(nvs_flash_init());

    wifi_config_init();
    mqtt_app_start();
    // set callback to process subscribed events
    // this callback will only process subscribed events
    // this will come in /msg topic

    while(1) {
        int msg_id;

        BaseType_t MQTTQueueReturn;
        mqtt_msg_t received_msg;

        MQTTQueueReturn = xQueueReceive(MQTTQueueHandle, (void *)&received_msg, portMAX_DELAY);

        if (MQTTQueueReturn == pdPASS) {
            switch(received_msg.tsk_id){
                case READ_ADC_TASK_ID:
                    msg_id = esp_mqtt_client_publish(client, READ_ADC_TOPIC, received_msg.msg, 0, 1, 0);
                    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
                break;

                case KEYPAD_TASK_ID:
                    msg_id = esp_mqtt_client_publish(client, KEYPAD_TOPIC, received_msg.msg, 0, 1, 0);
                    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
                break;

                default:
                ESP_LOGI(TAG, "Unknown task id");
                break;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}