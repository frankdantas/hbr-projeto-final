#ifndef SHARED_VARS_H
#define SHARED_VARS_H

#include "pico/stdlib.h"
#include "stdio.h"

#define RGB32(r,g,b) ((r << 8) | (g << 16) | b)
#define CLAMP(x, min, max) (((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x)))

#define WS2812_PIN 7
#define MIC_CHANNEL 2
#define MIC_PIN (26 + MIC_CHANNEL)
#define PIN_LED_RED 13
#define PIN_LED_GREEN 11

extern volatile uint16_t amountLeds;//Quantidade de leds para controlar
extern volatile uint16_t prevAmountLeds;//Quantidade de leds anteriores à modificação para controlar
extern volatile bool shouldStopEffect;//Indica se o efeito deve ser parado
extern volatile uint8_t efeitoAtivo;//Indica qual efeito deve ser executado
extern volatile uint32_t mainColor;//Cor principal do efeito
extern volatile uint32_t secondColor;//Cor secundária do efeito
extern volatile uint32_t fullColor;//Cor completa do efeito
extern volatile bool shouldUpdateFullColor;//Indica se a cor completa deve ser atualizada
extern volatile uint8_t colunasLed;//Quantidade de colunas de leds
extern volatile bool shouldSaveLeds;//Indica se as comfigs de led devem ser salvas
extern volatile uint8_t ledsPorColuna;//Quantidade de leds por coluna
extern volatile uint8_t sensibilityADC;//Sensibilidade do ADC
//extern volatile uint8_t messageFromDevice;//Mensagem recebida do dispositivo

#endif