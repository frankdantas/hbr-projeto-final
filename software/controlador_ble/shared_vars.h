#ifndef SHARED_VARS_H
#define SHARED_VARS_H

#include "pico/stdlib.h"
#include "stdio.h"

#define RGB32(r,g,b) ((r << 8) | (g << 16) | b)

#define WS2812_PIN 7
#define MIC_CHANNEL 2
#define MIC_PIN (26 + MIC_CHANNEL)
#define PIN_LED_RED 13

extern volatile uint16_t amountLeds;
extern volatile bool shouldStopEffect;
extern volatile uint8_t efeitoAtivo;
extern volatile uint32_t mainColor;
extern volatile uint32_t secondColor;
extern volatile uint32_t fullColor;
extern volatile bool shouldUpdateFullColor;
extern volatile uint8_t colunasLed;

#endif