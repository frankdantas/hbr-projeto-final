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


#define CMD_NUMERO_LEDS "NL="
#define CMD_EFEITO "EF="
#define CMD_MAIN_COLOR "MC="
#define CMD_SECOND_COLOR "SC="
#define CMD_FULL_COLOR "FC="
#define CMD_SENSIBILITY_ADC "SA="
#define CMD_QUANTIDADE_COLUNAS "QC="
#define CMD_SAVE "SAVE"


#define APP_AD_FLAGS 0x06
static uint8_t adv_data[] = {
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, APP_AD_FLAGS,
    0x08, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'C', 't', 'r', 'l', 'B', 'L', 'E',//Nome do dispositivo
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0xF0, 0xFF,//Serviço custom
    //0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x1a, 0x18,
};
static const uint8_t adv_data_len = sizeof(adv_data);

//int le_notification_enabled = 0;
//hci_con_handle_t con_handle;
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

uint32_t convertColor(const char* corHex, uint32_t defaultColor){

    if(corHex != NULL){
        printf("Cor Hex: %s, tamanho: %d\n", corHex, strlen(corHex));
        if(strlen(corHex) != 6){
            printf("Formato de cor inválido!\n");
            return defaultColor;
        }
        uint32_t rgb;
        if (sscanf(corHex, "%06X", &rgb) != 1) {
            printf("Formato de cor inválido!\n");
            return defaultColor;
        }

        // Separa as cores RGB
        uint8_t r = (rgb >> 16) & 0xFF;
        uint8_t g = (rgb >> 8) & 0xFF;
        uint8_t b = rgb & 0xFF;


        return RGB32(r, g, b);
    }

    return 0;
}

void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(channel);
    //printf("Executou funcao packet_handler, type: %02x, packet: %04x\n", packet_type, hci_event_packet_get_type(packet));


    if (packet_type != HCI_EVENT_PACKET) return;

    //verify_event(hci_event_packet_get_type(packet));

    if (hci_event_packet_get_type(packet) == BTSTACK_EVENT_STATE) {
        printf("Executou funcao BTSTACK_EVENT_STATE, state\n");
        gap_advertisements_set_data(adv_data_len, adv_data);
        gap_advertisements_enable(1);
    }
    /*if (hci_event_packet_get_type(packet) == ATT_EVENT_CAN_SEND_NOW) {
        // Enviar notificação quando for permitido
        char message[10];
        sprintf(message, "E=%02x", messageFromDevice);

        printf("Escreveu: %s\n", message);
        att_server_notify(con_handle, ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_STRING_01_VALUE_HANDLE, &message, sizeof(message));
    }*/
    if (hci_event_packet_get_type(packet) == HCI_EVENT_DISCONNECTION_COMPLETE) {
        //le_notification_enabled = 0;
        printf("Desconectou\n");
        //gpio_put(PIN_LED_RED, 1);
    }
    if (hci_event_packet_get_type(packet) == ATT_EVENT_CONNECTED) {
        //le_notification_enabled = 0;
        printf("Conectou\n");
        //gpio_put(PIN_LED_RED, 0);
    }

    
}

uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
     UNUSED(connection_handle);

     printf("All parameters solicited: %d, %04x, %d, %d, %s\n", connection_handle, att_handle, offset, buffer_size, (char *)buffer);

     if(att_handle == ATT_CHARACTERISTIC_1191e8b3_5c6b_417c_b7b5_813ec84a757e_01_VALUE_HANDLE){
        printf("Solicitou valor de RX\n");
     }/*else if(att_handle == ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_STRING_01_VALUE_HANDLE){
        printf("Solicitou valor de TX\n");
     }*/

    return 0;
}


int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    UNUSED(transaction_mode);

    printf("All parameters wrote: %d, %04x, %04x, %d, %d, %s\n", connection_handle, att_handle, transaction_mode, offset, buffer_size, (char *)buffer);

    if(att_handle == ATT_CHARACTERISTIC_1191e8b3_5c6b_417c_b7b5_813ec84a757e_01_VALUE_HANDLE){
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
        if(strncmp(comunicacao_data, CMD_EFEITO, strlen(CMD_EFEITO)) == 0){// Verifica se os 3 primeiros caracteres sao 'ef='
            char *ptr = strchr(comunicacao_data, '='); // Encontra '='
            if(ptr != NULL){
                int valor = atoi(ptr + 1); // Converte para int
                valor = CLAMP(valor, 0, 5);
                uint8_t prevVal = efeitoAtivo;
                efeitoAtivo = valor;
                shouldStopEffect = (prevVal != efeitoAtivo);
                //shouldSaveLeds = (prevVal != efeitoAtivo);
                printf("Ativou efeito %d\n", efeitoAtivo);
                gpio_put(PIN_LED_RED, (efeitoAtivo == 0));
                gpio_put(PIN_LED_GREEN, (efeitoAtivo != 0));
            }
        }else if(strncmp(comunicacao_data, CMD_NUMERO_LEDS, strlen(CMD_NUMERO_LEDS)) == 0){// Define a quantidade de leds, verifica se os 3 primeiros caracteres sao 'nl='
            char *ptr = strchr(comunicacao_data, '='); // Encontra '='
            if(ptr != NULL){
                int valor = atoi(ptr + 1); // Converte para int
                valor = CLAMP(valor, 0, 255);

                uint8_t prevVal = amountLeds;
                prevAmountLeds = amountLeds;
                amountLeds = valor;
                shouldStopEffect = (prevVal != efeitoAtivo);
                ledsPorColuna = amountLeds / colunasLed;
                //shouldSaveLeds = prevVal != amountLeds;
                printf("Ativou %d leds\n", amountLeds);

                /*if(amountLeds == 0){
                    amountLeds = valor;
                    shouldStopEffect = true;
                    shouldSaveLeds = true;
                    printf("Ativou %d leds\n", amountLeds);
                }else{
                    printf("Nao pode alterar a quantidade de leds depois de iniciado\n");
                }*/
                
            }
        }else if(strncmp(comunicacao_data, CMD_MAIN_COLOR, strlen(CMD_MAIN_COLOR)) == 0){// Define a mainColor
            char *ptr = strchr(comunicacao_data, '='); // Encontra '=' e pega tudo a partir dele, inclusive o =
            if(ptr != NULL){
                const char *corHex = ptr + 1;//Pega tudo depois do =
                uint32_t prevVal = mainColor;
                int valor = convertColor(corHex, prevVal);
                
                mainColor = valor;
                //shouldSaveLeds = prevVal != mainColor;
                printf("Alterou cor principal %d\n", mainColor);

            }
        }else if(strncmp(comunicacao_data, CMD_SECOND_COLOR, strlen(CMD_SECOND_COLOR)) == 0){// Define a secondColor, verifica se os 3 primeiros caracteres sao 'c2='
            char *ptr = strchr(comunicacao_data, '='); // Encontra '='
            if(ptr != NULL){
                const char *corHex = ptr + 1;//Pega tudo depois do =
                uint32_t prevVal = secondColor;
                int valor = convertColor(corHex, prevVal);
                
                secondColor = valor;
                //shouldSaveLeds = prevVal != secondColor;
                printf("Alterou cor secundaria %d\n", secondColor);

            }
        }else if(strncmp(comunicacao_data, CMD_FULL_COLOR, strlen(CMD_FULL_COLOR)) == 0){// Define a fullColor, verifica se os 3 primeiros caracteres sao 'c3='
            char *ptr = strchr(comunicacao_data, '='); // Encontra '='
            if(ptr != NULL){
                const char *corHex = ptr + 1;//Pega tudo depois do =
                uint32_t prevVal = fullColor;
                int valor = convertColor(corHex, prevVal);
                
                fullColor = valor;
                //shouldSaveLeds = prevVal != fullColor;
                shouldUpdateFullColor = prevVal != fullColor;
                printf("Alterou cor total %d\n", fullColor);

            }
        }else if(strncmp(comunicacao_data, CMD_QUANTIDADE_COLUNAS, strlen(CMD_QUANTIDADE_COLUNAS)) == 0){// Define a quantidade de colunas de leds
            char *ptr = strchr(comunicacao_data, '='); // Encontra '='
            if(ptr != NULL){
                int valor = atoi(ptr + 1); // Converte para int
                valor = CLAMP(valor, 1, amountLeds);
                uint8_t prevVal = colunasLed;
                colunasLed = valor;
                ledsPorColuna = amountLeds / colunasLed;
                //shouldSaveLeds = prevVal != colunasLed;
                printf("Alterou colunas de led para %d\n", colunasLed);
            }
        }else if(strncmp(comunicacao_data, CMD_SENSIBILITY_ADC, strlen(CMD_SENSIBILITY_ADC)) == 0){// Define um multiplicador para o ADC
            char *ptr = strchr(comunicacao_data, '='); // Encontra '='
            if(ptr != NULL){
                int valor = atoi(ptr + 1); // Converte para int
                valor = CLAMP(valor, 1, 5);
                uint8_t prevVal = sensibilityADC;
                sensibilityADC = valor;
                //shouldSaveLeds = prevVal != sensibilityADC;
                printf("Alterou sensibilidade ADC para %d\n", sensibilityADC);
            }
        }else if(strncmp(comunicacao_data, CMD_SAVE, strlen(CMD_SAVE)) == 0){// Salva as mudanças na flash
            shouldSaveLeds = true;
            printf("Salvar na flash as mudancas\n");
        }else{
            printf("Outro comando: %s\n", comunicacao_data);
        }

    }/*else if(att_handle == ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_STRING_01_CLIENT_CONFIGURATION_HANDLE){
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
    }*/else{
        printf("Outras alteração\n");
    }

    return 0;
}