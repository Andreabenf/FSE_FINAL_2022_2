#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

#include <driver/adc.h>

void configBuzzerGpio();
void configSevenColorsLedGpio();
void configSoundSensorGpio();
void configInclineSensorGpio();
void configLedGpio();
void configTilt();
void configDigitalDetection();
void configAnalogDetection();
void configButtonGpio();
void piscaLed();
int getDigitalSound();
int getAnalogicSound();
int getAnalogicMagne();
int getAnalogicTilt();
int getDigitalMagne();
void ledPWM(int32_t duty);
void configKy025();
#endif