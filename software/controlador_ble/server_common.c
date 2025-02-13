/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "btstack.h"

#include "server_common.h"
#include "comunicacao.h"


#define APP_AD_FLAGS 0x06
static uint8_t adv_data[] = {
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, APP_AD_FLAGS,
    0x08, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'C', 't', 'r', 'l', 'B', 'L', 'E',
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x1a, 0x18,
};
static const uint8_t adv_data_len = sizeof(adv_data);

int le_notification_enabled = 0;
hci_con_handle_t con_handle;
char comunicacao_data[31] = {0};

void verify_event(uint8_t event) {
    uint8_t valores[] = {
        HCI_EVENT_NOP,
        HCI_EVENT_INQUIRY_COMPLETE,
        HCI_EVENT_INQUIRY_RESULT,
        HCI_EVENT_CONNECTION_REQUEST,
        HCI_EVENT_DISCONNECTION_COMPLETE,
        HCI_EVENT_CONNECTION_COMPLETE,
        HCI_EVENT_AUTHENTICATION_COMPLETE,
        HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE
    };

    for (size_t i = 0; i < 8; i++)
    {
        if(event == valores[i]){
            printf("Executou funcao verify_event, event: %02x no index: %d\n", event, i);
        }
        return;
    }

    printf("Nao encontrou evento: %02x\n", event);
    
}

void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(channel);
    printf("Executou funcao packet_handler, type: %02x, packet: %04x\n", packet_type, hci_event_packet_get_type(packet));


    if (packet_type != HCI_EVENT_PACKET) return;

    //verify_event(hci_event_packet_get_type(packet));

    if (hci_event_packet_get_type(packet) == BTSTACK_EVENT_STATE) {
        printf("Executou funcao BTSTACK_EVENT_STATE, state\n");
        gap_advertisements_set_data(adv_data_len, adv_data);
        gap_advertisements_enable(1);
    }
    if (hci_event_packet_get_type(packet) == ATT_EVENT_CAN_SEND_NOW) {
        // Enviar notificação quando for permitido
        uint8_t notification_data = rand() % 100;
        printf("Escreveu: %02x\n", notification_data);
        att_server_notify(con_handle, ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_STRING_01_VALUE_HANDLE, &notification_data, sizeof(notification_data));
    }
    if (hci_event_packet_get_type(packet) == HCI_EVENT_DISCONNECTION_COMPLETE) {
        le_notification_enabled = 0;
        printf("Desconectou\n");
        gpio_put(PIN_LED_RED, 1);
    }
    if (hci_event_packet_get_type(packet) == ATT_EVENT_CONNECTED) {
        le_notification_enabled = 0;
        printf("Conectou\n");
        gpio_put(PIN_LED_RED, 0);
    }

    
}

uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
     UNUSED(connection_handle);

     printf("All parameters solicited: %d, %04x, %d, %d, %s\n", connection_handle, att_handle, offset, buffer_size, (char *)buffer);

     if(att_handle == ATT_CHARACTERISTIC_87654321_4321_6789_4321_abcdef012345_01_VALUE_HANDLE){
        printf("Solicitou valor de RX\n");
     }else if(att_handle == ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_STRING_01_VALUE_HANDLE){
        printf("Solicitou valor de TX\n");
     }

    return 0;
}

// TODO: Fazer validação dos dados inseridos, como quando a quantidade de leds for 0 ou colunas maior que a quantidade de leds
int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    UNUSED(transaction_mode);

    printf("All parameters wrote: %d, %04x, %04x, %d, %d, %s\n", connection_handle, att_handle, transaction_mode, offset, buffer_size, (char *)buffer);

    if(att_handle == ATT_CHARACTERISTIC_87654321_4321_6789_4321_abcdef012345_01_VALUE_HANDLE){
        printf("Alteração no valor de RX:");
        for (size_t i = 0; i < buffer_size; i++)
        {
            printf("%02x  ", buffer[i]);
        }
        printf("\n");
        strncpy(comunicacao_data, (char *)buffer, buffer_size);
        comunicacao_data[buffer_size] = '\0';
        printf("Recebi novo valor: %s\n", comunicacao_data);

        // Escolhe o efeito
        if(strncmp(comunicacao_data, "ef=", 3) == 0){// Verifica se os 3 primeiros caracteres sao 'ef='
            char *ptr = strchr(comunicacao_data, '='); // Encontra '='
            if(ptr != NULL){
                int valor = atoi(ptr + 1); // Converte para int
                efeitoAtivo = valor;
                shouldStopEffect = true;
                
                printf("Ativou efeito %d\n", efeitoAtivo);
            }
        }else if(strncmp(comunicacao_data, "nl=", 3) == 0){// Define a quantidade de leds, verifica se os 3 primeiros caracteres sao 'nl='
            char *ptr = strchr(comunicacao_data, '='); // Encontra '='
            if(ptr != NULL){
                int valor = atoi(ptr + 1); // Converte para int
                if(amountLeds == 0){
                    amountLeds = valor;
                    shouldStopEffect = true;
                    printf("Ativou %d leds\n", amountLeds);
                }else{
                    printf("Nao pode alterar a quantidade de leds depois de iniciado\n");
                }
                
            }
        }else if(strncmp(comunicacao_data, "c1=", 3) == 0){// Define a mainColor
            char *ptr = strchr(comunicacao_data, '='); // Encontra '='
            if(ptr != NULL){
                int valor = atoi(ptr + 1); // Converte para int
                mainColor = valor;
                printf("Alterou cor principal %d\n", mainColor);
            }
        }else if(strncmp(comunicacao_data, "c2=", 3) == 0){// Define a secondColor, verifica se os 3 primeiros caracteres sao 'c2='
            char *ptr = strchr(comunicacao_data, '='); // Encontra '='
            if(ptr != NULL){
                int valor = atoi(ptr + 1); // Converte para int
                secondColor = valor;
                printf("Alterou cor secundaria %d\n", secondColor);
            }
        }else if(strncmp(comunicacao_data, "c3=", 3) == 0){// Define a fullColor, verifica se os 3 primeiros caracteres sao 'c3='
            char *ptr = strchr(comunicacao_data, '='); // Encontra '='
            if(ptr != NULL){
                int valor = atoi(ptr + 1); // Converte para int
                fullColor = valor;
                printf("Alterou cor fullColor %d\n", fullColor);
                shouldUpdateFullColor = true;
            }
        }else if(strncmp(comunicacao_data, "qc=", 3) == 0){// Define a quantidade de colunas de leds
            char *ptr = strchr(comunicacao_data, '='); // Encontra '='
            if(ptr != NULL){
                int valor = atoi(ptr + 1); // Converte para int
                colunasLed = valor;
                printf("Alterou colunas de led para %d\n", colunasLed);
            }
        }else{
            printf("Outro comando: %s\n", comunicacao_data);
        }

    }else if(att_handle == ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_STRING_01_CLIENT_CONFIGURATION_HANDLE){
        printf("Alteração na configuração de TX:");
        for (size_t i = 0; i < buffer_size; i++)
        {
            printf("%02x  ", buffer[i]);
        }
        printf("\n");

        uint16_t config = little_endian_read_16(buffer, 0);
        if(config == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION){
            le_notification_enabled = 1;
            con_handle = connection_handle;
            printf("Notificação ativada no TX no con_handle: %d\n", con_handle);
        }else{
            le_notification_enabled = 0;
            printf("Notificação desativada no TX\n");
        }

    }else if(att_handle == ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_STRING_01_VALUE_HANDLE){
        printf("Alteração no valor de TX:");
        for (size_t i = 0; i < buffer_size; i++)
        {
            printf("%02x  ", buffer[i]);
        }
        printf("\n");
        strncpy(comunicacao_data, (char *)buffer, buffer_size);
        comunicacao_data[buffer_size] = '\0';
        printf("Recebi novo valor: %s\n", comunicacao_data);
    }else{
        printf("Outras alteração\n");
    }

    return 0;
}

void poll_temp(void) {
    printf("Chamou poll_temp\n");
 }