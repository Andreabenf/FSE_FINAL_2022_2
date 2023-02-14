#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

#include <driver/adc.h>

void configBuzzerGpio();
void ligaBuzzer();
void configSevenColorsLedGpio();
void configSoundSensorGpio();
void configInclineSensorGpio();
void configLedGpio();
void configTilt();
void configAnalogDetection();
void configButtonGpio();
void piscaLed();
int getSound();
int getAnalogicSound();
int getAnalogicMagne();
int getAnalogicTilt();
int getDigitalMagne();
void ledPWM(int32_t duty);
void configKy025();
void partyMode();
void play_music();
#endif