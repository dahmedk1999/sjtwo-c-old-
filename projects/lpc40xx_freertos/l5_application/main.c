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
void uart_read_task(void *p) {

  while (1) {
    char *data;
    uart_lab__polled_get(UART_3, &data);
    fprintf(stderr, "%c", data);
    vTaskDelay(50);
  }
}

void uart_write_task(void *p) {
  while (1) {
    uart_lab__polled_put(UART_3, 'W');
    uart_lab__polled_put(UART_3, 'O');
    uart_lab__polled_put(UART_3, 'R');
    uart_lab__polled_put(UART_3, 'K');
    uart_lab__polled_put(UART_3, 'p');
    uart_lab__polled_put(UART_3, 'l');
    uart_lab__polled_put(UART_3, 's');
    uart_lab__polled_put(UART_3, '\n');
    // fprintf(stderr, "Sent %c\n", data);
    vTaskDelay(500);
  }
}
// PART 2
void uart_read_isr(void *p) {
  while (true) {
    char *data;
    if (uart_lab__get_char_from_queue(&data, 50))
      fprintf(stderr, "%c", data);
    else
      fprintf(stderr, "\nQueue is empty\n");
  } // no need for delays since queues auto sleep/block
}
//Part 3 EX

void board_1_sender_task(void *p) {
  char number_as_string[16] = { 0 };
  
   while (true) {
     const int number = rand();
     sprintf(number_as_string, "%i", number);
     
     // Send one char at a time to the other board including terminating NULL char
     for (int i = 0; i <= strlen(number_as_string); i++) {
       uart_lab__polled_put(number_as_string[i]);
       printf("Sent: %c\n", number_as_string[i]);
     }
 
     printf("Sent: %i over UART to the other board\n", number);
     vTaskDelay(3000);
   }
}

void board_2_receiver_task(void *p) {
  char number_as_string[16] = { 0 };
  int counter = 0;

  while (true) {
    char byte = 0;
    uart_lab__get_char_from_queue(&byte, portMAX_DELAY);
    printf("Received: %c\n", byte);
    
    // This is the last char, so print the number
    if ('\0' == byte) {
      number_as_string[counter] = '\0';
      counter = 0;
      printf("Received this number from the other board: %s\n", number_as_string);
    }
    // We have not yet received the NULL '\0' char, so buffer the data
    else {
      // TODO: Store data to number_as_string[] array one char at a time
      // Hint: Use counter as an index, and increment it as long as we do not reach max value of 16
    }
  }
}
/////////////////////////// MAIN ///////////////////////////

void main(void) {
  // TODO: Use uart_lab__init() function and initialize UART2 or UART3 (your choice)
  // TODO: Pin Configure IO pins to perform UART2/UART3 function
  uart_lab__init(UART_3, 96, 9600);
  gpio__construct_with_function(4, 28, GPIO__FUNCTION_2);
  gpio__construct_with_function(4, 29, GPIO__FUNCTION_2);
  // Part 1
  // xTaskCreate(uart_write_task, "Ux3Write", 2048 / sizeof(void *), NULL, 3, NULL);
  // xTaskCreate(uart_read_task, "Ux3Read", 2048 / sizeof(void *), NULL, 3, NULL);

  // Part 2
  uart__enable_receive_interrupt(UART_3);
  xTaskCreate(uart_write_task, "Ux3Write", 2048 / sizeof(void *), NULL, 3, NULL);
  xTaskCreate(uart_read_isr, "Ux3ISRRead", 2048 / sizeof(void *), NULL, 3, NULL);

  vTaskStartScheduler();
}
//////////////////////////END MAIN/////////////////////////
