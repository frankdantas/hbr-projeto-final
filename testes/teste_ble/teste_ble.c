#include <stdio.h>
#include "pico/stdlib.h"
#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "server_common.h"
#include <string.h>

#define LED_PLACA 13

static btstack_packet_callback_registration_t hci_event_callback_registration;


int main()
{
    stdio_init_all();
    gpio_init(LED_PLACA);
    gpio_set_dir(LED_PLACA, GPIO_OUT);
    gpio_put(LED_PLACA, 0);
    
    printf("Esperando abrir serial monitor...");
    while (!stdio_usb_connected()) {
      printf(".");
      sleep_ms(500);
    }
    printf("\nSerial aberta!\n");


    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43_arch\n");
        return -1;
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

    uint8_t contador = 0;

    while (true) {
        //printf("Hello, world!\n");
        sleep_ms(1000);
        contador++;
        if(contador >= 15){
            contador = 0;
            if(le_notification_enabled){
                printf("Enviar notificacao\n");
                att_server_request_can_send_now_event(con_handle);
            }else{
                printf("Notificacao desabilitada\n");
            }
            
        }
        
    }
}
