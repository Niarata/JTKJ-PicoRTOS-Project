#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

#define DEFAULT_STACK_SIZE 2048
#define CDC_ITF_TX      1


// Määritellään tilat
typedef enum {
    INIT,
    IDLE,
    WORKING
} SystemState;

volatile SystemState current_state = INIT;

// Napin keskeytyskäsittelijä
void button_isr(uint gpio, uint32_t eventMask) {
    if (current_state == IDLE) {
        current_state = WORKING;
    } else if (current_state == WORKING) {
        current_state = IDLE;
    }
}

// RTOS Task 1: Pääohjain ja tilakone
void mainControlTask(void *pvParameters) {
    // Muuttujat IMU-datan tallentamiseen
    float ax, ay, az, gx, gy, gz, t;

    while(1) {
        switch(current_state) {
            case INIT:
                // 1. Alustetaan IMU-sensori laitteen käynnistyessä
                if (init_ICM42670() == 0) {
                    printf("IMU alustettu onnistuneesti!\n");
                    ICM42670_start_with_default_values();
                } else {
                    printf("Virhe IMU-sensorin alustuksessa.\n");
                }
                
                // Siirrytään odotustilaan
                current_state = IDLE;
                break;
                
            case IDLE:
                // Odotustilassa ei tehdä mitään raskasta.
                break;
                
            case WORKING:
                // 2. Luetaan IMU-dataa vain silloin, kun laite on "WORKING"-tilassa
                if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
                    printf("Kiihtyvyys: X=%.2f, Y=%.2f, Z=%.2f | Lämpö: %.1f C\n", ax, ay, az, t);
                } else {
                    printf("Datan luku epäonnistui\n");
                }
                break;
        }
        
        // Viive (200 ms). Tämä tarkoittaa, että WORKING-tilassa dataa tulostetaan 5 kertaa sekunnissa.
        vTaskDelay(pdMS_TO_TICKS(200)); 
    }
}

// RTOS Task 2: LEDin ohjaus tilan mukaan
void ledControlTask(void *pvParameters) {
    while(1) {
        if (current_state == IDLE) {
            gpio_put(RED_LED_PIN, 0); // LED OFF
        } 
        else if (current_state == WORKING) {
            gpio_put(RED_LED_PIN, 1); // LED ON
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main() {
    stdio_init_all();

    // Pinnien alustukset (Nappi ja LED)
    gpio_init(SW1_PIN);
    gpio_set_dir(SW1_PIN, GPIO_IN);
    
    gpio_init(RED_LED_PIN);
    gpio_set_dir(RED_LED_PIN, GPIO_OUT);

    // Napin keskeytyksen asetus
    gpio_set_irq_enabled_with_callback(SW1_PIN, GPIO_IRQ_EDGE_FALL, true, button_isr);
    
    // Odotetaan, että sarjaportti (Serial Monitor) yhdistyy tietokoneeseen, 
    // jotta näemme varmasti "IMU alustettu" -tulosteen heti alussa.
    while (!stdio_usb_connected()){
        sleep_ms(10);
    }
    
    init_hat_sdk();
    sleep_ms(300);

    // RTOS-tehtävien luonti
    xTaskCreate(mainControlTask, "MainControl", 2048, NULL, 2, NULL);
    xTaskCreate(ledControlTask, "LedControl", 1024, NULL, 1, NULL);

    vTaskStartScheduler();
    
    return 0;
}