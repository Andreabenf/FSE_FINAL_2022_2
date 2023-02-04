#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

#include <driver/adc.h>


void configLedGpio();
void configButtonGpio();
void piscaLed();
int getAnalogicMagne();
int getDigitalMagne();
void ledPWM(int32_t duty);
void configKy025();
#endif