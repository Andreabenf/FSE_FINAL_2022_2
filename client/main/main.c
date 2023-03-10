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
#include "gpio_config.h"

xSemaphoreHandle conexaoWifiSemaphore;
xSemaphoreHandle sendDataMQTTSemaphore;

#define GPIO_DHT CONFIG_ESP_DHT_GPIO_NUMBER
#define GPIO_BUTTON CONFIG_ESP_BUTTON_GPIO_NUMBER

char central_path[150], comodo_path[150];
char telemetry_path[150], attr_path[150], state_path[150];
int flag_run = 0;
xQueueHandle filaDeInterrupcao;

static void IRAM_ATTR gpio_isr_handler(void *args)
{
    int pino = (int)args;
    xQueueSendFromISR(filaDeInterrupcao, &pino, NULL);
}

void trataInterrupcaoBotao(void *params)
{
    int pino;
    int contador = 0;

    while (true)
    {
        if (xQueueReceive(filaDeInterrupcao, &pino, portMAX_DELAY))
        {
            int estado = gpio_get_level(pino);
            if (estado == 0)
            {
                gpio_isr_handler_remove(pino);
                int contadorPressionado = 0;
                printf("Apertou o botão\n");
                while (gpio_get_level(pino) == estado)
                {
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                    contadorPressionado++;
                    printf("Manteve o botão pressionado: %d\n",
                           contadorPressionado);
                    if (contadorPressionado == 50)
                    {
                        ledPWM(20);
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

void conectadoWifi(void *params)
{
    while (true)
    {
        if (xSemaphoreTake(conexaoWifiSemaphore, portMAX_DELAY))
        {
            // Processamento Internet
            mqtt_start();
        }
    }
}

void definePaths()
{
    memset(comodo_path, '\0', sizeof comodo_path);
    int result = le_valor_nvs("comodo_path", comodo_path);
    if (result > 0)
    {
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

void startSleep()
{
    // Coloca a ESP no Deep Sleep
    esp_deep_sleep_start();
}

void trataBotaoPressionadoLowPower()
{
    // Trata segurar botão para reiniciar
    int estado = gpio_get_level(GPIO_BUTTON);
    if (estado == 0)
    {
        gpio_isr_handler_remove(GPIO_BUTTON);
        int contadorPressionado = 0;
        printf("Apertou o botão\n");
        while (gpio_get_level(GPIO_BUTTON) == estado)
        {
            vTaskDelay(50 / portTICK_PERIOD_MS);
            contadorPressionado++;
            printf("Manteve o botão pressionado: %d\n", contadorPressionado);
            if (contadorPressionado == 50)
            {
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

void enviaDadosServidor(void *params)
{
    if (xSemaphoreTake(sendDataMQTTSemaphore, portMAX_DELAY))
    {
        // Define os paths
        definePaths();

#ifdef CONFIG_BATERIA
        // Trata Botão pressionadoCONFIG_BATERIA
        trataBotaoPressionadoLowPower();

            #ifdef CONFIG_PLACA_1
            float humValue, tempValue;
            int magDig, magAn;
            magDig = getDigitalMagne();
            magAn = getAnalogicMagne();
            dht_read_float_data(DHT_TYPE_DHT11, GPIO_DHT, &humValue,
                                &tempValue);

            cJSON *humidity = cJSON_CreateNumber(humValue);
            cJSON *temperature = cJSON_CreateNumber(tempValue);
            cJSON *magnet = cJSON_CreateNumber(magDig);

            cJSON *resHumidity = cJSON_CreateObject();
            cJSON *resTemperature = cJSON_CreateObject();
            cJSON *resMag = cJSON_CreateObject();
            cJSON_AddItemReferenceToObject(resHumidity, "umidade", humidity);
            cJSON_AddItemReferenceToObject(resTemperature, "temperatura",
                                           temperature);
            cJSON_AddItemReferenceToObject(resMag, "magnetismo", magnet);
            mqtt_envia_mensagem(attr_path, cJSON_Print(resHumidity));
            vTaskDelay(50 / portTICK_PERIOD_MS);
            mqtt_envia_mensagem(telemetry_path, cJSON_Print(resTemperature));
            vTaskDelay(50 / portTICK_PERIOD_MS);
            mqtt_envia_mensagem(attr_path, cJSON_Print(resMag));
            vTaskDelay(50 / portTICK_PERIOD_MS);
            mqtt_envia_mensagem(attr_path, cJSON_Print(resHumidity));
            vTaskDelay(50 / portTICK_PERIOD_MS);
            printf("magDig: %d, magAn: %d\n\n\n", magDig, magAn);
            cJSON_Delete(resHumidity);
            cJSON_Delete(resTemperature);
            cJSON_Delete(resMag);
            #elif CONFIG_PLACA_3
            int tilt, sound;

            sound = getSound();
            tilt = getAnalogicTilt();
            // soundAnalog = getAnalogicSound();
            if (tilt == 4095)
            {
                cJSON *tiltData = cJSON_CreateString("Horizontal");
                cJSON *resTilt = cJSON_CreateObject();
                cJSON_AddItemReferenceToObject(resTilt, "tilt", tiltData);
                mqtt_envia_mensagem(telemetry_path, cJSON_Print(resTilt));
                vTaskDelay(50 / portTICK_PERIOD_MS);
                cJSON_Delete(resTilt);
            }
            else
            {
                cJSON *tiltData = cJSON_CreateString("Vertical");
                cJSON *resTilt = cJSON_CreateObject();
                cJSON_AddItemReferenceToObject(resTilt, "tilt", tiltData);
                mqtt_envia_mensagem(telemetry_path, cJSON_Print(resTilt));
                vTaskDelay(50 / portTICK_PERIOD_MS);
                cJSON_Delete(resTilt);
            }

            cJSON *soundData = cJSON_CreateNumber(sound);

            cJSON *resSound = cJSON_CreateObject();

            cJSON_AddItemReferenceToObject(resSound, "sound", soundData);
            vTaskDelay(50 / portTICK_PERIOD_MS);
            mqtt_envia_mensagem(telemetry_path, cJSON_Print(resSound));

            printf("Tilt: %d\n\n\n", tilt);
            printf("Sound: %d\n\n\n", sound);
            // printf("SoundAnalog: %d\n\n\n", soundAnalog);

        
            cJSON_Delete(resSound);
        #endif
        // Trata botão pressionado ao acordar
        if (flag_run)
        {
            printf("O botão foi acionado. Botão: %d\n", GPIO_BUTTON);
            int32_t estado_entrada = le_int32_nvs("estado_entrada");
            estado_entrada = estado_entrada ? 0 : 1;
            grava_int32_nvs("estado_entrada", estado_entrada);

            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
        else
        {
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        printf("Entrando em modo sleep...\n");
        startSleep();
#else
        enviaEstadosCentral();
#endif

        // Loop da task
        while (true)
        {
#ifdef CONFIG_ENERGIA

#ifdef CONFIG_PLACA_1
            float humValue, tempValue;
            int magDig, magAn;
            magDig = getDigitalMagne();
            magAn = getAnalogicMagne();
            dht_read_float_data(DHT_TYPE_DHT11, GPIO_DHT, &humValue,
                                &tempValue);

            cJSON *humidity = cJSON_CreateNumber(humValue);
            cJSON *temperature = cJSON_CreateNumber(tempValue);
            cJSON *magnet = cJSON_CreateNumber(magDig);

            cJSON *resHumidity = cJSON_CreateObject();
            cJSON *resTemperature = cJSON_CreateObject();
            cJSON *resMag = cJSON_CreateObject();
            cJSON_AddItemReferenceToObject(resHumidity, "umidade", humidity);
            cJSON_AddItemReferenceToObject(resTemperature, "temperatura",
                                           temperature);
            cJSON_AddItemReferenceToObject(resMag, "magnetismo", magnet);

            mqtt_envia_mensagem(attr_path, cJSON_Print(resHumidity));
            vTaskDelay(50 / portTICK_PERIOD_MS);
            mqtt_envia_mensagem(telemetry_path, cJSON_Print(resTemperature));
            vTaskDelay(50 / portTICK_PERIOD_MS);
            mqtt_envia_mensagem(attr_path, cJSON_Print(resMag));
            vTaskDelay(50 / portTICK_PERIOD_MS);
            mqtt_envia_mensagem(attr_path, cJSON_Print(resHumidity));
            vTaskDelay(50 / portTICK_PERIOD_MS);
            printf("magDig: %d, magAn: %d\n\n\n", magDig, magAn);
            cJSON_Delete(resHumidity);
            cJSON_Delete(resTemperature);
            cJSON_Delete(resMag);
#elif CONFIG_PLACA_3
            int tilt, sound;

            sound = getSound();
            tilt = getAnalogicTilt();
            // soundAnalog = getAnalogicSound();
            if (tilt == 4095)
            {
                cJSON *tiltData = cJSON_CreateString("Horizontal");
                cJSON *resTilt = cJSON_CreateObject();
                cJSON_AddItemReferenceToObject(resTilt, "tilt", tiltData);
                mqtt_envia_mensagem(telemetry_path, cJSON_Print(resTilt));
                vTaskDelay(50 / portTICK_PERIOD_MS);
                cJSON_Delete(resTilt);
            }
            else
            {
                cJSON *tiltData = cJSON_CreateString("Vertical");
                cJSON *resTilt = cJSON_CreateObject();
                cJSON_AddItemReferenceToObject(resTilt, "tilt", tiltData);
                mqtt_envia_mensagem(telemetry_path, cJSON_Print(resTilt));
                vTaskDelay(50 / portTICK_PERIOD_MS);
                cJSON_Delete(resTilt);
            }

            cJSON *soundData = cJSON_CreateNumber(sound);

            cJSON *resSound = cJSON_CreateObject();

            cJSON_AddItemReferenceToObject(resSound, "sound", soundData);

         
            vTaskDelay(50 / portTICK_PERIOD_MS);
            mqtt_envia_mensagem(telemetry_path, cJSON_Print(resSound));

            printf("Tilt: %d\n\n\n", tilt);
            printf("Sound: %d\n\n\n", sound);
            // printf("SoundAnalog: %d\n\n\n", soundAnalog);

        
            cJSON_Delete(resSound);
#endif
#endif
        }
    }
}

void configuraGPIO()
{
#ifdef CONFIG_PLACA_1
    configLedGpio();
    configButtonGpio();
#elif CONFIG_PLACA_2
    configBuzzerGpio();
    configSevenColorsLedGpio();
#elif CONFIG_PLACA_3
    configTilt();
    configAnalogDetection();
#endif

#ifdef CONFIG_BATERIA
    // Configura o retorno
    esp_sleep_enable_ext0_wakeup(GPIO_BUTTON, 0);
    printf("Botão: Interrupções em modo Wake\n");
#endif
}

void defineCentralPath()
{
    // Cria e salva o path home
    memset(central_path, '\0', sizeof central_path);
    strcpy(central_path, "v1/devices/me/");
    grava_string_nvs("central_path", central_path);
}

void defineVariaveisEstado()
{
    int32_t estado_entrada = le_int32_nvs("estado_entrada");
    if (estado_entrada == -1)
    {
        estado_entrada = 0;
        grava_int32_nvs("estado_entrada", estado_entrada);
    }

    int32_t estado_saida = le_int32_nvs("estado_saida");
    if (estado_saida == -1)
    {
        estado_saida = 0;
        grava_int32_nvs("estado_saida", estado_saida);
    }
}

void app_main()
{
    // Inicializa o NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    int result = le_valor_nvs("central_path", central_path);
    if (result == -1)
    {
        defineCentralPath();
        defineVariaveisEstado();
    }
    else
    {
        printf("Path central: %s\n", central_path);
        flag_run = 1;
    }

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
#ifdef CONFIG_PLACA_1
    xTaskCreate(&enviaDadosServidor, "Envio de dados", 4096, NULL, 1, NULL);
#elif CONFIG_PLACA_3
    xTaskCreate(&enviaDadosServidor, "Envio de dados", 4096, NULL, 1, NULL);
#endif
    if (flag_run)
    {
        xSemaphoreGive(sendDataMQTTSemaphore);
    }
}