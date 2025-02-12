#ifndef SHARED_VARS_H
#define SHARED_VARS_H

#include "pico/stdlib.h"
#include "stdio.h"

extern volatile uint16_t amountLeds;
extern volatile bool shouldStopEffect;
extern volatile uint8_t efeitoAtivo;
extern volatile uint32_t mainColor;
extern volatile uint32_t secondColor;

#endif