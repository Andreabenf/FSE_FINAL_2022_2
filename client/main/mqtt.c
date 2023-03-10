#include "mqtt.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "mqtt_client.h"
#include "parser.h"

#define TAG "MQTT"

esp_mqtt_client_handle_t client;
extern int flag_run;
extern char central_path[100];

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        if (flag_run == 0)
        {
            esp_mqtt_client_publish(client, central_path,
                                    define_modo_json(), 0, 1, 0);
        }
        // esp_mqtt_client_subscribe(client, central_path, 0);
        esp_mqtt_client_subscribe(client, "v1/devices/me/rpc/request/+", 0);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d, %s", event->msg_id, central_path);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        const char *res = event->data;
        parse_json(res);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base,
             event_id);
    mqtt_event_handler_cb(event_data);
}

void mqtt_start()
{
#ifdef CONFIG_PLACA_1
   esp_mqtt_client_config_t mqtt_config = {
        .uri = "mqtt://164.41.98.25",
        .username = "cUNn5ysSxbGiO0vIaiwQ"};
#elif CONFIG_PLACA_2
    esp_mqtt_client_config_t mqtt_config = {
        .uri = "mqtt://164.41.98.25",
        .username = "XDJBSU5cJK2veAQrUc5y"};
#elif CONFIG_PLACA_3
    esp_mqtt_client_config_t mqtt_config = {
        .uri = "mqtt://164.41.98.25",
        .username = "yDVGkx4DGG6AjncWEtuy"};
#endif
    client = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler,
                                   client);
    esp_mqtt_client_start(client);
}

void mqtt_envia_mensagem(char *topico, char *mensagem)
{
    int message_id = esp_mqtt_client_publish(client, topico, mensagem, 0, 1, 0);
    ESP_LOGI(TAG, "Mensagem enviada, ID: %d, %s", message_id, mensagem);
}