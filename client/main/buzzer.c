#include <stdio.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define BUZZER_PIN 23

void app_main() {
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = 1ULL<<BUZZER_PIN;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  int comp = 250;

  while (true) {
    gpio_set_level(BUZZER_PIN, 1);
    vTaskDelay(comp / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER_PIN, 0);
    vTaskDelay(comp / portTICK_PERIOD_MS);
  }
}
