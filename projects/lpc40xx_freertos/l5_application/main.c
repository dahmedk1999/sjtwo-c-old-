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
// UART


void board_1_sender_task(void *p) { // UART3
  char number_as_string[16] = {0};

  while (true) {
    const int number = rand() % 10000;
    sprintf(number_as_string, "%i", number);

    // Send one char at a time to the other board including terminating NULL char
    for (int i = 0; i <= strlen(number_as_string); i++) {
      gpioX__set_high(0, 26);
      uart_lab__polled_put(UART_3, number_as_string[i]);
      gpioX__set_low(0, 26);
      printf("\nput: %c\n", number_as_string[i]);
    }

    printf("\nput full: %i\n", number);
    vTaskDelay(3000);
  }
}

void board_2_receiver_task(void *p) { // UART2
  char number_as_string[16] = {0};
  int counter = 0;

  while (true) {
    char byte = 0;
    gpioX__set_high(0, 26);
    uart_lab__get_char_from_queue(UART_2, &byte, portMAX_DELAY);
    gpioX__set_low(0, 26);
    printf("\nget: %c\n", byte);

    // This is the last char, so print the number
    if ('\0' == byte) {
      number_as_string[counter] = '\0';
      counter = 0;
      printf("\nget full: %s\n", number_as_string);
    }
    // We have not yet received the NULL '\0' char, so buffer the data
    else {
      // TODO: Store data to number_as_string[] array one char at a time
      // Hint: Use counter as an index, and increment it as long as we do not reach max value of 16
      number_as_string[counter] = byte;
      if (counter < 16)
        counter++;
      printf("\nStored %c", byte);
    }
  }
}
/////////////////////////// MAIN ///////////////////////////

void main(void) {
  
  xTaskCreate(board_1_sender_task, "Sender", 2048 / sizeof(void *), NULL, 2, NULL);
  xTaskCreate(board_2_receiver_task, "Receiver", 2048 / sizeof(void *), NULL, 3, NULL);

  vTaskStartScheduler();
}
//////////////////////////END MAIN/////////////////////////
