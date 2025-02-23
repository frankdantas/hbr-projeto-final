#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "server_common.h"
#include "shared_vars.h"
#include "ws2812.pio.h"

#include "pico_hal.h"
#include "stdinit.h"

//#define WAIT_FOR_SERIAL 1

/// @brief Estrutura de configuração para salvar na flash
typedef struct __attribute__((packed)){
    uint8_t amountLeds;
    uint8_t colunasLed;
    uint8_t efeitoAtivo;
    uint32_t mainColor;
    uint32_t secondColor;
    uint32_t fullColor;
}config_t;



uint32_t currentColor = 0;
//uint32_t starterColor = 0;

bool flagHelpColor = 0;
bool flagHelpColorStarter = 0;

static btstack_packet_callback_registration_t hci_event_callback_registration;
config_t config = {25, 5, 0, RGB32(10, 0, 0), RGB32(0, 10, 0), RGB32(20, 20, 20)};



void init_cores(){
    mainColor = RGB32(10, 0, 0);
    secondColor = RGB32(0, 10, 0);

    currentColor = mainColor;
    //starterColor = mainColor;
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

    // registra o callback
    att_server_register_packet_handler(packet_handler);

    // liga o bluetooth
    hci_power_control(HCI_POWER_ON);

    return true;
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    if(x <= in_min) return out_min;
    if(x >= in_max) return out_max;

    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

void clear_all(PIO *pio, uint sm, uint8_t hasDelay){
    printf("Cleaning leds\n");
    uint8_t maxLedCount = amountLeds > prevAmountLeds ? amountLeds : prevAmountLeds;
    for(uint8_t i = 0; i < maxLedCount; i++){
        put_pixel(*pio, sm, 0);
    }
    if(hasDelay){
        sleep_ms(5);
    }
    
}

void efeito1(PIO *pio, uint sm){
    printf("Executou efeito 1\n");
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

void efeito2(PIO *pio, uint sm){
    printf("Executou efeito 2\n");
    for (size_t i = 0; i < amountLeds && !shouldStopEffect; i++)
    {
        for (size_t j = 0; j <= i && !shouldStopEffect; j++)
        {
            put_pixel(*pio, sm, currentColor);
        }
        sleep_ms(50);
    }

    currentColor = currentColor == mainColor ? secondColor : mainColor;
}

void efeito3(PIO *pio, uint sm){
    printf("Executou efeito 3 com cor 0x%08x em %d leds\n", fullColor, amountLeds);
    uint8_t log = 0;
    for (size_t i = 0; i < amountLeds && !shouldStopEffect; i++)
    {
        put_pixel(*pio, sm, fullColor);
        ++log;
    }
    printf("Executou efeito 3 com %d leds\n", log);
}

void efeito4(PIO *pio, uint sm, uint16_t qtdLeds){
    printf("Executou efeito 4\n");
    for(int i = 0; i < amountLeds && !shouldStopEffect; i++){
        if(i <= qtdLeds){
            put_pixel(*pio, sm, mainColor);
        }else{
            put_pixel(*pio, sm, RGB32(0, 0, 0));
        }
    }
}

void efeito5(PIO *pio, uint sm, uint8_t qtdLeds){
    printf("Executou efeito 5\n");
    const uint8_t maxCores = 7;
    static const uint32_t arco_iris[] = {
      RGB32(255, 0, 0),
      RGB32(255, 127, 0),
      RGB32(255, 255, 0),
      RGB32(0, 255, 0),
      RGB32(0, 255, 255),
      RGB32(0, 0, 255),
      RGB32(255, 0, 255)  
    };

    if(qtdLeds < 1){
        clear_all(pio, sm, false);
        return;
    }

    uint8_t indexPar = 0;
    uint8_t indexImpar = 9;
    uint8_t indexCor = 0;
    //uint8_t ledsPorColuna = amountLeds / colunasLed;
    uint32_t *coresColuna = malloc(ledsPorColuna * sizeof(uint32_t));

    for (size_t i = 0; i < colunasLed && !shouldStopEffect; i++)
    {
        bool colunaPar = i % 2 == 0;
        uint32_t cor = arco_iris[indexCor];

        //uint8_t rnd = random() % ledsPorColuna;
        uint8_t rnd = random() % qtdLeds;
        
        for (size_t j = 0; j < ledsPorColuna; j++){
            if(j <= rnd){
                coresColuna[j] = cor;
            }else{
                coresColuna[j] = 0x0;
            }
        }
        if(colunaPar){
            for (size_t k = 0; k < ledsPorColuna; k++)
            {
                put_pixel(*pio, sm, coresColuna[k]);
            }
        }else{
            for (int k = ledsPorColuna - 1; k >= 0; k--)
            {
                put_pixel(*pio, sm, coresColuna[k]);
            }
        }

        /*if(colunaPar){
            for (size_t j = 0; j < ledsPorColuna; j++){
                if(j <= rnd){
                    put_pixel(*pio, sm, cor);
                }else{
                    put_pixel(*pio, sm, RGB32(0, 0, 0));
                }
            }
        }else{
            for (size_t j = 0; j < ledsPorColuna; j++){
                if(j < (ledsPorColuna - rnd)){
                    put_pixel(*pio, sm, RGB32(0,0,0));
                }else{
                    put_pixel(*pio, sm, cor);
                }
            }
        }*/




        if(colunaPar){
            indexPar += 10;
        }else{
            indexImpar += 10;
        }

        ++indexCor;
        if(indexCor > maxCores-1){indexCor = 0;}
    }

    free(coresColuna);
}



void setup_adc(){
    adc_gpio_init(MIC_PIN);
    adc_init();
    adc_select_input(MIC_CHANNEL);
}

void setup_led(){
    gpio_init(PIN_LED_RED);
    gpio_set_dir(PIN_LED_RED, GPIO_OUT);
    gpio_put(PIN_LED_RED, 0);

    gpio_init(PIN_LED_GREEN);
    gpio_set_dir(PIN_LED_GREEN, GPIO_OUT);
    gpio_put(PIN_LED_GREEN, 0);
}

uint8_t read_adc_mapped(uint8_t sensibilidade, uint8_t maxValue, int offset){
    int ad_raw = adc_read(); //map(adc_read(), 0, 4095, 0, 25);//Le o ad mas sempre na metado do valor total
    ad_raw -= 2048;//Coloco tudo na linha d0 0
    ad_raw = ((ad_raw < 0) ? -ad_raw : ad_raw) + offset;
    //ad_raw = (ad_raw < 0) ? 0 : ad_raw;

    ad_raw = ad_raw * sensibilidade;
        

    int quatidade = map(ad_raw, 0, 2048, 0, maxValue + 1);

    return quatidade;
}

void saveFS(const char *fileName){
    if (pico_mount(false) < 0) {
        printf("Mount failed\n");
        return;
    }else{
        printf("FS mounted successfully\n");
    }

    printf("Saving values\n");
    printf("Led count: %d\n", config.amountLeds);
    printf("Main color: 0x%08x\n", config.mainColor);
    printf("Second color: 0x%08x\n", config.secondColor);
    printf("Full color: 0x%08x\n", config.fullColor);
    printf("Colunas: %d\n", config.colunasLed);
    printf("Efeito: %d\n", config.efeitoAtivo);

    int file = pico_open(fileName, LFS_O_RDWR | LFS_O_CREAT);
    pico_rewind(file);
    pico_write(file, &config, sizeof(config));
	// save the file size
	int pos = pico_lseek(file, 0, LFS_SEEK_CUR);
    // Close the file, making sure all buffered data is flushed
    pico_close(file);
	// Unmount the file system, freeing memory
    pico_unmount();

    //printf("Saved leds count with value: %d\n", ledsCount);
    // log config
    

}


void readFS(const char *fileName){

    if (pico_mount(false) < 0) {
        printf("Mount failed\n");
        return;
    }else{
        printf("FS mounted successfully\n");
    }


    int file = pico_open(fileName, LFS_O_RDWR | LFS_O_CREAT);
    uint8_t leds_count = 0;
	// Read previous boot count. If file was just created read will return 0 bytes
    pico_read(file, &config, sizeof(config));
    // Close the file, making sure all buffered data is flushed
    pico_close(file);
	// Unmount the file system, freeing memory
    pico_unmount();

    printf("Read values\n");
    printf("Led count: %d\n", config.amountLeds);
    printf("Main color: 0x%08x\n", config.mainColor);
    printf("Second color: 0x%08x\n", config.secondColor);
    printf("Full color: 0x%08x\n", config.fullColor);
    printf("Colunas: %d\n", config.colunasLed);
    printf("Efeito: %d\n", config.efeitoAtivo);

    amountLeds = config.amountLeds;
    mainColor = config.mainColor;
    secondColor = config.secondColor;
    fullColor = config.fullColor;
    colunasLed = config.colunasLed;
    efeitoAtivo = config.efeitoAtivo;
}


int main()
{
    stdio_init_all();

    setup_adc();
    setup_led();

    init_cores();
    bool iniciouBLE = init_ble();

    #ifdef WAIT_FOR_SERIAL
    /*printf("Esperando abrir serial monitor...");
    while (!stdio_usb_connected()) {
      printf(".");
      sleep_ms(500);
    }
    printf("\nSerial aberta!\n");*/

    #endif

    const char *fileName = "/config.cfg";

    

    if(!iniciouBLE){
        while(true){
            gpio_put(PIN_LED_RED, 1);
            sleep_ms(250);
            gpio_put(PIN_LED_RED, 0);
            sleep_ms(250);
        }
        return 0;
    }

    readFS(fileName);

    PIO pio;//Objeto PIO para a matriz de led
    uint sm;//Objeto SM stateMachine para a matriz de led
    uint offset = 0;

    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, WS2812_PIN, 1, true);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);
    sleep_ms(100);
    

    clear_all(&pio, sm, true);
    //sleep_ms(50);

    static absolute_time_t last_run_efeito1 = 0;
    static absolute_time_t last_run_efeito2 = 0;

    uint16_t adc_value = 0;

    // Led vermelho ligado, modo espera
    gpio_put(PIN_LED_RED, (efeitoAtivo == 0));
    gpio_put(PIN_LED_GREEN, (efeitoAtivo != 0));

    ledsPorColuna = amountLeds / colunasLed;
    shouldUpdateFullColor = true;

    //sleep_ms(500);
    
    while (true) {
        switch(efeitoAtivo){
            case 1:
                if(absolute_time_diff_us(last_run_efeito1, get_absolute_time()) > 250000){//250ms
                    last_run_efeito1 = get_absolute_time();
                    efeito1(&pio, sm);
                }
            break;
            case 2:
                if(absolute_time_diff_us(last_run_efeito2, get_absolute_time()) > 100000){//100ms
                    last_run_efeito2 = get_absolute_time();
                    efeito2(&pio, sm);
                }
            break;
            case 3:
                if(shouldUpdateFullColor){
                    shouldUpdateFullColor = false;
                    efeito3(&pio, sm);
                }
            break;
            case 4:
                adc_value = read_adc_mapped(sensibilityADC, amountLeds + 1, -200);
                efeito4(&pio, sm, adc_value);
            break;
            case 5:
                //uint8_t ledsPorColuna = amountLeds / colunasLed;
                adc_value = read_adc_mapped(sensibilityADC, ledsPorColuna + 1, -100);
                efeito5(&pio, sm, adc_value);
            break;
            default:
                //printf("Esperando efeito...\n");
            break;
        }

        if(shouldStopEffect){
            sleep_ms(10);
            clear_all(&pio, sm, true);
            shouldStopEffect = false;
            if(efeitoAtivo == 3) shouldUpdateFullColor = true;
            continue;
        }

        if(shouldSaveLeds){
            shouldSaveLeds = false;
            config.amountLeds = amountLeds;
            config.mainColor = mainColor;
            config.secondColor = secondColor;
            config.fullColor = fullColor;
            config.colunasLed = colunasLed;
            config.efeitoAtivo = efeitoAtivo;
            saveFS(fileName);
        }
        sleep_ms(10);
    }

    pio_remove_program_and_unclaim_sm(&ws2812_program, pio, sm, offset);
}