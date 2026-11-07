#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include <math.h>

#include "tkjhat/sdk.h"

#define DEFAULT_STACK_SIZE 2048
#define CDC_ITF_TX      1


typedef enum {
    INIT,
    IDLE,
    WORKING
} SystemState;

volatile SystemState current_state = INIT;

void button_isr(uint gpio, uint32_t eventMask) {
    if (current_state == IDLE) {
        current_state = WORKING;
    } else if (current_state == WORKING) {
        current_state = IDLE;
    }
}

// RTOS Task 1: Pääohjain ja tilakone
void mainControlTask(void *pvParameters) {
    float ax, ay, az, gx, gy, gz, t;

    while(1) {
        switch(current_state) {
            case INIT:
                if (init_ICM42670() == 0) {
                    printf("IMU alustettu onnistuneesti!\n");
                    ICM42670_start_with_default_values();
                } else {
                    printf("Virhe IMU-sensorin alustuksessa.\n");
                }
                current_state = IDLE;
                break;
                
            case IDLE:
                break;
                
            case WORKING:
                if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
                    
                    // 1. Käännetään X-akselin suunta päinvastaiseksi
                    ax = -ax;

                    // 2. Lasketaan kaikkien kolmen akselin kulmat asteina
                    // X-akseli (Pitch / eteen-taakse)
                    float angle_x = atan2(ax, sqrt(ay * ay + az * az)) * 57.2958f;
                    
                    // Y-akseli (Roll / sivulle)
                    float angle_y = atan2(ay, sqrt(ax * ax + az * az)) * 57.2958f;
                    
                    // Z-akseli (Kallistus pystysuunnasta)
                    float angle_z = atan2(sqrt(ax * ax + ay * ay), az) * 57.2958f;

                    // Tulostetaan kaikki kolme suuntaa
                    printf("Kulmat: X=%.0f, Y=%.0f, Z=%.0f\n", angle_x, angle_y, angle_z);
                    
                } else {
                    printf("Datan luku epäonnistui\n");
                }
                break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(200)); 
    }
}

void ledControlTask(void *pvParameters) {
    while(1) {
        if (current_state == IDLE) {
            gpio_put(RED_LED_PIN, 0);
        } 
        else if (current_state == WORKING) {
            gpio_put(RED_LED_PIN, 1);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main() {
    stdio_init_all();

    gpio_init(SW1_PIN);
    gpio_set_dir(SW1_PIN, GPIO_IN);
    
    gpio_init(RED_LED_PIN);
    gpio_set_dir(RED_LED_PIN, GPIO_OUT);

    gpio_set_irq_enabled_with_callback(SW1_PIN, GPIO_IRQ_EDGE_FALL, true, button_isr);
    
    while (!stdio_usb_connected()){
        sleep_ms(10);
    }
    
    init_hat_sdk();
    sleep_ms(300);

    xTaskCreate(mainControlTask, "MainControl", 2048, NULL, 2, NULL);
    xTaskCreate(ledControlTask, "LedControl", 1024, NULL, 1, NULL);

    vTaskStartScheduler();
    
    return 0;
}