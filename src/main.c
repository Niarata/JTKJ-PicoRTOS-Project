#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

#define DEFAULT_STACK_SIZE 2048
#define CDC_ITF_TX      1


void my_toggle_led(uint gpio, uint32_t eventMask) {
    // Vaihdetaan LEDin tila negaatiolla
    uint8_t pinValue = gpio_get(RED_LED_PIN);
    pinValue = !pinValue;
    gpio_put(RED_LED_PIN, pinValue);
}

int main() {
    stdio_init_all();

    // Pinnien alustukset
    gpio_init(SW1_PIN);
    gpio_set_dir(SW1_PIN, GPIO_IN);
    
    gpio_init(RED_LED_PIN);
    gpio_set_dir(RED_LED_PIN, GPIO_OUT);

    // Asetetaan napin painallukselle keskeytys (interrupt)
    gpio_set_irq_enabled_with_callback(SW1_PIN, GPIO_IRQ_EDGE_FALL, true, my_toggle_led);
    
    init_hat_sdk();
    sleep_ms(300); // Odotetaan, että laitteisto on alustettu

    // Käynnistetään FreeRTOS-käyttöjärjestelmän vuorottelija.
    // Koska poistimme omat tehtävät (taskit), ohjelma pyörittää nyt vain 
    // käyttöjärjestelmän omaa "tyhjäkäyntiä" (idle task) ja reagoi 
    // ainoastaan napin painalluksiin.
    vTaskStartScheduler();
    
    return 0;
}