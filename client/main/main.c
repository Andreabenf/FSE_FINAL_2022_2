#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "dht.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "flash.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "mqtt.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "send.h"
#include "wifi.h"

#include "driver/ledc.h"

xSemaphoreHandle conexaoWifiSemaphore;
xSemaphoreHandle sendDataMQTTSemaphore;

#define GPIO_DHT CONFIG_ESP_DHT_GPIO_NUMBER
#define GPIO_LED CONFIG_ESP_LED_GPIO_NUMBER
#define GPIO_BUTTON CONFIG_ESP_BUTTON_GPIO_NUMBER

char central_path[150], comodo_path[150];
char telemetry_path[150], attr_path[150], state_path[150];
int flag_run = 0;
xQueueHandle filaDeInterrupcao;

static void IRAM_ATTR gpio_isr_handler(void *args) {
    int pino = (int)args;
    xQueueSendFromISR(filaDeInterrupcao, &pino, NULL);
}

void piscaLed() {
    for (int blips = 3; blips >= 0; blips--) {
        gpio_set_level(GPIO_LED, 1);
        vTaskDelay(400 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_LED, 0);
        vTaskDelay(400 / portTICK_PERIOD_MS);
    }
}

void trataInterrupcaoBotao(void *params) {
    int pino;
    int contador = 0;

    while (true) {
        if (xQueueReceive(filaDeInterrupcao, &pino, portMAX_DELAY)) {
            int estado = gpio_get_level(pino);
            if (estado == 0) {
                gpio_isr_handler_remove(pino);
                int contadorPressionado = 0;
                printf("Apertou o botão\n");
                while (gpio_get_level(pino) == estado) {
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                    contadorPressionado++;
                    printf("Manteve o botão pressionado: %d\n",
                           contadorPressionado);
                    if (contadorPressionado == 50) {
                        piscaLed();
                        nvs_flash_erase_partition("DadosNVS");
                        esp_restart();
                        break;
                    }
                }

                contador++;
                printf("O botão foi acionado %d vezes. Botão: %d\n", contador,
                       pino);

                int32_t estado_entrada = le_int32_nvs("estado_entrada");
                estado_entrada = estado_entrada ? 0 : 1;
                grava_int32_nvs("estado_entrada", estado_entrada);

                enviaEstadosCentral();

                // Habilitar novamente a interrupção
                vTaskDelay(50 / portTICK_PERIOD_MS);
                gpio_isr_handler_add(pino, gpio_isr_handler, (void *)pino);
            }
        }
    }
}

void conectadoWifi(void *params) {
    while (true) {
        if (xSemaphoreTake(conexaoWifiSemaphore, portMAX_DELAY)) {
            // Processamento Internet
            mqtt_start();
        }
    }
}

void definePaths() {
    memset(comodo_path, '\0', sizeof comodo_path);
    int result = le_valor_nvs("comodo_path", comodo_path);
    if (result > 0) {
        printf("valor lido da func: %s\n", comodo_path);
    }
    // Define "path" de Umidade
    memset(telemetry_path, '\0', sizeof telemetry_path);
    strcpy(telemetry_path, "v1/devices/me/telemetry");

    // Define "path" de Temperatura
    memset(attr_path, '\0', sizeof attr_path);
    strcpy(attr_path, "v1/devices/me/attributes");

    // Define "path" de Estado
    memset(state_path, '\0', sizeof state_path);
    strcpy(state_path, "v1/devices/me/attributes");
}

void startSleep() {
    // Coloca a ESP no Deep Sleep
    esp_deep_sleep_start();
}

void trataBotaoPressionadoLowPower() {
    // Trata segurar botão para reiniciar
    int estado = gpio_get_level(GPIO_BUTTON);
    if (estado == 0) {
        gpio_isr_handler_remove(GPIO_BUTTON);
        int contadorPressionado = 0;
        printf("Apertou o botão\n");
        while (gpio_get_level(GPIO_BUTTON) == estado) {
            vTaskDelay(50 / portTICK_PERIOD_MS);
            contadorPressionado++;
            printf("Manteve o botão pressionado: %d\n", contadorPressionado);
            if (contadorPressionado == 50) {
                piscaLed();
                nvs_flash_erase_partition("DadosNVS");
                esp_restart();
                break;
            }
        }
        // Habilitar novamente a interrupção
        vTaskDelay(50 / portTICK_PERIOD_MS);
        gpio_isr_handler_add(GPIO_BUTTON, gpio_isr_handler,
                             (void *)GPIO_BUTTON);
    }
}

void enviaDadosServidor(void *params) {
    if (xSemaphoreTake(sendDataMQTTSemaphore, portMAX_DELAY)) {
        // Define os paths
        definePaths();

#ifdef CONFIG_BATERIA
        // Trata Botão pressionadoCONFIG_BATERIA
        trataBotaoPressionadoLowPower();

        cJSON *envio_low_json = cJSON_CreateObject();
        cJSON *estado_json = cJSON_CreateObject();

        float humValue, tempValue;
        dht_read_float_data(DHT_TYPE_DHT11, GPIO_DHT, &humValue, &tempValue);

        cJSON *humidity = cJSON_CreateNumber(humValue);
        cJSON *temperature = cJSON_CreateNumber(tempValue);

        cJSON_AddItemReferenceToObject(estado_json, "umidade", humidity);
        cJSON_AddItemReferenceToObject(estado_json, "temperatura", temperature);
        cJSON_AddItemReferenceToObject(envio_low_json, "estado", estado_json);

        // Trata botão pressionado ao acordar
        if (flag_run) {
            printf("O botão foi acionado. Botão: %d\n", GPIO_BUTTON);
            int32_t estado_entrada = le_int32_nvs("estado_entrada");
            estado_entrada = estado_entrada ? 0 : 1;
            grava_int32_nvs("estado_entrada", estado_entrada);

            mqtt_envia_mensagem(state_path, cJSON_Print(envio_low_json));
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        } else {
            mqtt_envia_mensagem(state_path, cJSON_Print(envio_low_json));
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        cJSON_Delete(estado_json);
        cJSON_Delete(envio_low_json);

        printf("Entrando em modo sleep...\n");
        startSleep();
#else
        enviaEstadosCentral();
#endif

        // Loop da task
        while (true) {
#ifdef CONFIG_ENERGIA
            float humValue, tempValue;
            dht_read_float_data(DHT_TYPE_DHT11, GPIO_DHT, &humValue,
                                &tempValue);

            cJSON *humidity = cJSON_CreateNumber(humValue);
            cJSON *temperature = cJSON_CreateNumber(tempValue);

            cJSON *resHumidity = cJSON_CreateObject();
            cJSON *resTemperature = cJSON_CreateObject();

            cJSON_AddItemReferenceToObject(resHumidity, "umidade", humidity);
            cJSON_AddItemReferenceToObject(resTemperature, "temperatura",
                                           temperature);

            mqtt_envia_mensagem(attr_path, cJSON_Print(resHumidity));
            vTaskDelay(50 / portTICK_PERIOD_MS);
            mqtt_envia_mensagem(telemetry_path, cJSON_Print(resTemperature));
            vTaskDelay(2000 / portTICK_PERIOD_MS);

            cJSON_Delete(resHumidity);
            cJSON_Delete(resTemperature);
#endif
        }
    }
}

void configuraGPIO() {
    // Configuração dos pinos dos LEDs
    // gpio_pad_select_gpio(GPIO_LED);
    // // Configura os pinos dos LEDs como Output
    // gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);

    // Configuração do Timer
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_config);

    // Configuração do Canal
    ledc_channel_config_t channel_config = {
        .gpio_num = GPIO_LED,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channel_config);

    int32_t intensidade_led = le_int32_nvs("intensidade_led");
    if (intensidade_led != -1) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, intensidade_led);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    }

    // Configuração do pino do Botão
    gpio_pad_select_gpio(GPIO_BUTTON);
    // Configura o pino do Botão como Entrada
    gpio_set_direction(GPIO_BUTTON, GPIO_MODE_INPUT);

    int32_t estado_saida = le_int32_nvs("estado_saida");
    gpio_set_level(GPIO_LED, estado_saida);

    // Configura o resistor de Pull-up para o botão (por padrão a entrada
    // estará em Um)
    gpio_pullup_en(GPIO_BUTTON);

    // Desabilita o resistor de Pull-down por segurança.
    gpio_pulldown_dis(GPIO_BUTTON);

    // Configura pino para interrupção
    gpio_set_intr_type(GPIO_BUTTON, GPIO_INTR_NEGEDGE);

#ifdef CONFIG_BATERIA
    // Configura o retorno
    esp_sleep_enable_ext0_wakeup(GPIO_BUTTON, 0);
    printf("Botão: Interrupções em modo Wake\n");
#endif
}

void defineCentralPath() {
    // Cria e salva o path home
    memset(central_path, '\0', sizeof central_path);
    strcpy(central_path, "v1/devices/me/");
    grava_string_nvs("central_path", central_path);
}

void defineVariaveisEstado() {
    int32_t estado_entrada = le_int32_nvs("estado_entrada");
    if (estado_entrada == -1) {
        estado_entrada = 0;
        grava_int32_nvs("estado_entrada", estado_entrada);
    }

    int32_t estado_saida = le_int32_nvs("estado_saida");
    if (estado_saida == -1) {
        estado_saida = 0;
        grava_int32_nvs("estado_saida", estado_saida);
    }
}

void app_main() {
    // Inicializa o NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    int result = le_valor_nvs("central_path", central_path);
    if (result == -1) {
        defineCentralPath();
        defineVariaveisEstado();
    } else {
        printf("Path central: %s\n", central_path);
        flag_run = 1;
    }

    // Configura GPIO
    configuraGPIO();

    // Inicializa semáforos
    conexaoWifiSemaphore = xSemaphoreCreateBinary();
    sendDataMQTTSemaphore = xSemaphoreCreateBinary();

    // Inicializa módulo WI-FI
    wifi_start();

    filaDeInterrupcao = xQueueCreate(10, sizeof(int));
    xTaskCreate(&trataInterrupcaoBotao, "TrataBotao", 2048, NULL, 1, NULL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_BUTTON, gpio_isr_handler, (void *)GPIO_BUTTON);

    xTaskCreate(&conectadoWifi, "Conexão ao MQTT", 4096, NULL, 1, NULL);
    xTaskCreate(&enviaDadosServidor, "Envio de dados", 4096, NULL, 1, NULL);
    if (flag_run) {
        xSemaphoreGive(sendDataMQTTSemaphore);
    }
}