
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "flash.h"

#include <driver/adc.h>

#define GPIO_LED CONFIG_ESP_LED_GPIO_NUMBER
#define GPIO_BUTTON CONFIG_ESP_BUTTON_GPIO_NUMBER
#define GPIO_DIGITAL_MAGNET 15



void configBuzzerGpio(){}
void configTwoColorLedGpio(){}
void configSoundSensorGpio(){}
void configInclineSensorGpio(){}
void ledPWM(int32_t duty){
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}
void configLedGpio(){
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
        ledPWM(intensidade_led);
    }

}
void configButtonGpio(void){
     // Configuração do pino do Botão
    gpio_pad_select_gpio(GPIO_BUTTON);
    // Configura o pino do Botão como Entrada
    gpio_set_direction(GPIO_BUTTON, GPIO_MODE_INPUT);

    // Configura o resistor de Pull-up para o botão (por padrão a entrada
    // estará em Um)
    gpio_pullup_en(GPIO_BUTTON);

    // Desabilita o resistor de Pull-down por segurança.
    gpio_pulldown_dis(GPIO_BUTTON);

    // Configura pino para interrupção
    gpio_set_intr_type(GPIO_BUTTON, GPIO_INTR_NEGEDGE);
}

void piscaLed() {
    for (int blips = 3; blips >= 0; blips--) {
        gpio_set_level(GPIO_LED, 1);
        vTaskDelay(400 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_LED, 0);
        vTaskDelay(400 / portTICK_PERIOD_MS);
    }
}

void configKy025(){
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_0);

    gpio_pad_select_gpio(GPIO_DIGITAL_MAGNET);
    gpio_set_direction(GPIO_DIGITAL_MAGNET, GPIO_MODE_INPUT);
}

int getAnalogicMagne(){
    return adc1_get_raw(ADC1_CHANNEL_0);
}

int getDigitalMagne(){
    return gpio_get_level(GPIO_DIGITAL_MAGNET);
}