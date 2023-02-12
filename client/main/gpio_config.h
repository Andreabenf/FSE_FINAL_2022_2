#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

#include <driver/adc.h>

void configBuzzerGpio();
void configSevenColorsLedGpio();
void configSoundSensorGpio();
void configInclineSensorGpio();
void configLedGpio();
void configTilt();
void configButtonGpio();
void piscaLed();
int getAnalogicMagne();
int getAnalogicTilt();
int getDigitalMagne();
void ledPWM(int32_t duty);
void configKy025();
#endif