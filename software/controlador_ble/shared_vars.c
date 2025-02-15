#include "shared_vars.h"

// Default values
volatile uint16_t amountLeds = 0;
volatile uint16_t prevAmountLeds = 0;
volatile bool shouldStopEffect = false;
volatile uint8_t efeitoAtivo = 0;
volatile uint32_t mainColor = 0;
volatile uint32_t secondColor = 0;
volatile uint32_t fullColor = RGB32(20, 20, 20);
volatile bool shouldUpdateFullColor = false;
volatile uint8_t colunasLed = 5;
volatile bool shouldSaveLeds = false;
volatile uint8_t ledsPorColuna = 1;
volatile uint8_t sensibilityADC = 1;
volatile uint8_t messageFromDevice = 0;