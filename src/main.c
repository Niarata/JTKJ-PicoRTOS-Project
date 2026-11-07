
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

#define DEFAULT_STACK_SIZE 2048
#define CDC_ITF_TX      1


enum state { DATA_READY,WAITING=1};
enum state programState = WAITING;

// Tehtävä 3: Valoisuuden globaali muuttuja
// Exercise 3: Global variable for ambient light
uint32_t ambientLight;

static void sensor_task(void *arg){
    (void)arg;
   
    for(;;){
         
        if (programState == DATA_READY) {
        ambientLight = veml6030_read_light();
        programState = WAITING;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void print_task(void *arg){
    (void)arg;
    
    while(1){
        
        if (programState == WAITING) {
        printf("Ambient light: %d\n", ambientLight);
        programState = DATA_READY;
        }
        printf("printTask\n");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void my_toggle_led(uint gpio, uint32_t eventMask) {
    // We change the state of the LED using negation
    uint8_t pinValue = gpio_get(RED_LED_PIN);
    pinValue = !pinValue;
    gpio_put(RED_LED_PIN, pinValue);
}

int main() {

    stdio_init_all();

    // Initializing pins
    gpio_init(SW1_PIN);
    gpio_set_dir(SW1_PIN, GPIO_IN);
    
    gpio_init(RED_LED_PIN);
    gpio_set_dir(RED_LED_PIN, GPIO_OUT);

   // We set the buttonFxn as an interrupt handler
   // for the button pin
    gpio_set_irq_enabled_with_callback(SW1_PIN, GPIO_IRQ_EDGE_FALL, true, my_toggle_led)

    stdio_init_all();

    // Uncomment this lines if you want to wait till the serial monitor is connected
    /*while (!stdio_usb_connected()){
        sleep_ms(10);
    }*/ 
    
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.

    TaskHandle_t hSensorTask, hPrintTask, hUSB = NULL;

    // Create the tasks with xTaskCreate
    BaseType_t result = xTaskCreate(sensor_task, // (en) Task function
                "sensor",                        // (en) Name of the task 
                DEFAULT_STACK_SIZE,              // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                            // (en) Arguments of the task 
                2,                               // (en) Priority of this task
                &hSensorTask);                   // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("Sensor task creation failed\n");
        return 0;
    }
    result = xTaskCreate(print_task,  // (en) Task function
                "print",              // (en) Name of the task 
                DEFAULT_STACK_SIZE,   // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                 // (en) Arguments of the task 
                2,                    // (en) Priority of this task
                &hPrintTask);         // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("Print Task creation failed\n");
        return 0;
    }

    // Start the scheduler (never returns)
    vTaskStartScheduler();
    
    // Never reach this line.
    return 0;
}

