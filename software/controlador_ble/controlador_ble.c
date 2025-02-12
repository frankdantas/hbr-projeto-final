#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "pico/multicore.h"
#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "server_common.h"
#include "shared_vars.h"
#include "ws2812.pio.h"

#define RGB32(r,g,b) ((r << 8) | (g << 16) | b)
#define WS2812_PIN 7

//uint32_t mainColor = 0;
//uint32_t secondColor = 0;
uint32_t currentColor = 0;
uint32_t starterColor = 0;

bool flagHelpColor = 0;
bool flagHelpColorStarter = 0;

static btstack_packet_callback_registration_t hci_event_callback_registration;


void init_cores(){
    mainColor = RGB32(10, 0, 0);
    secondColor = RGB32(0, 10, 0);

    currentColor = mainColor;
    starterColor = mainColor;
}

bool init_ble(){
    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43_arch\n");
        return false;
    }

    l2cap_init();
    sm_init();

    att_server_init(profile_data, att_read_callback, att_write_callback);    

    // inform about BTstack state
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // register for ATT event
    att_server_register_packet_handler(packet_handler);

    // turn on bluetooth!
    hci_power_control(HCI_POWER_ON);

    return true;}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    if(x < in_min) return out_min;
    if(x > in_max) return out_max;

    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

void efeito1(PIO *pio, uint sm){
    static uint8_t offset_inicio = 0;
    const uint8_t qtdLed = 6;
    if(qtdLed > amountLeds || shouldStopEffect){
        return;
    }

    flagHelpColor = flagHelpColorStarter;

    // Preenche os primeiros leds
    for(uint8_t i = 0; i < offset_inicio && !shouldStopEffect; i++){
      put_pixel(*pio, sm, flagHelpColor ? mainColor : secondColor);
      //printf("%d ", (flagHelpColor ? mainColor : secondColor));
    }
  
    // Preenche o restante dos leds
    for(uint8_t i = 0; i < amountLeds - offset_inicio && !shouldStopEffect; i++){
      put_pixel(*pio, sm, flagHelpColor ? mainColor : secondColor);
      //printf("%d ", (flagHelpColor ? mainColor : secondColor));

      if(i % qtdLed == 0){flagHelpColor = !flagHelpColor;}
    }

    
    offset_inicio++;
    if(offset_inicio % qtdLed == 0 && offset_inicio > 0){
        flagHelpColorStarter = !flagHelpColorStarter;
        offset_inicio = 0;
    }
    
}

void clear_all(PIO *pio, uint sm){
    for(uint8_t i = 0; i < amountLeds; i++){
        put_pixel(*pio, sm, 0);
    }
}



int main()
{
    stdio_init_all();
    init_cores();
    bool iniciouBLE = init_ble();

    if(!iniciouBLE){
        return 0;
    }

    PIO pio;//Objeto PIO para a matriz de led
    uint sm;//Objeto SM stateMachine para a matriz de led
    uint offset = 0;

    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, WS2812_PIN, 1, true);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);
    sleep_ms(100);
    

    clear_all(&pio, sm);
    sleep_ms(50);

    while (true) {
        switch(efeitoAtivo){
            case 1:
                efeito1(&pio, sm);
            break;
            default:
                printf("Esperando efeito...\n");
            break;
        }

        if(shouldStopEffect){
            clear_all(&pio, sm);
            sleep_ms(50);
            shouldStopEffect = false;
            continue;
        }
        sleep_ms(500);
    }

    pio_remove_program_and_unclaim_sm(&ws2812_program, pio, sm, offset);
}
