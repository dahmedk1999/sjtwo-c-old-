// clang-format off
#include "FreeRTOS.h"
#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "math.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include "time.h"
#include "C:\sjtwo-c\projects\lpc40xx_freertos\l3_drivers\adc.h"
#include "LPC40xx.h"
#include "gpio.h"
#include "gpio_isr.h"
#include "gpio_lab.h"
#include "lpc_peripherals.h"
#include "pwm1.h"
#include "queue.h"
#include "semphr.h"
#include "ssp2lab.h"
#include "uart_lab.h"

// LPC_GPIO0 - LPC_GPIO4    Ports
// CLR -> LOW 1
// SET -> HIGH 1
// &= ~(1 << 31); To Clear  (0)
// |= (1 << 31);  To Set    (1)
// REG & (1 << 9) To Check  (?)
// PIN -> 1 High 0 Low
// LEDs			 BUTTONS
/*
GPIO1 -18 LED 0  GPIO0 -29 Button 0
GPIO1 -24 LED 1  GPIO0 -30 Button 1
GPIO1 -26 LED 2  GPIO1 -15 Button 2
GPIO2 -3  LED 3  GPIO1 -10 Button 3

void foo(void) {}
typedef void (*foo)(void);
const port_pin_s *led = (port_pin_s *)(params);
*/

// clang-format on
static QueueHandle_t switch_queue;

typedef enum { switch__off, switch__on } switch_e;
switch_e get_switch_input_from_switch0(switch_e set) { return set; }
// TODO: Create this task at PRIORITY_LOW
void producer(void *p) {
  while (1) {

    // TODO: Get some input value from your board
    const switch_e switch_value = get_switch_input_from_switch0(switch__on);

    // TODO: Print a message before xQueueSend()
    printf("Before queue sends\n");
    xQueueSend(switch_queue, &switch_value, 0);
    // TODO: Print a message after xQueueSend()
    printf("After queue sends\n");
    vTaskDelay(7000);
  }
}

// TODO: Create this task at PRIORITY_HIGH
void consumer(void *p) {
  switch_e switch_value;
  while (1) {
    // TODO: Print a message before xQueueReceive()
    printf("Before receive.\n");
    xQueueReceive(switch_queue, &switch_value, portMAX_DELAY);
    // TODO: Print a message after xQueueReceive()
    printf("After receiving\n");
  }
}

/////////////////////////// MAIN ///////////////////////////

void main(void) {
  // create_uart_task();
  sj2_cli__init();
  switch_queue = xQueueCreate(1, sizeof(switch_e));
  xTaskCreate(producer, "producer", 2048 / sizeof(void *), NULL, 3, NULL);
  xTaskCreate(consumer, "consumer", 2048 / sizeof(void *), NULL, 1, NULL);
  vTaskStartScheduler();
}
//////////////////////////END MAIN/////////////////////////
