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
#include <string.h>
#include "OLED.h"
#include "acceleration.h"

#include "event_groups.h"
#include "ff.h"

#include "i2c.h"
#include "i2c_slave_init.h"

/*
// LPC_GPIO0 - LPC_GPIO4    Ports
// CLR -> LOW 1
// SET -> HIGH 1
// &= ~(1 << 31); To Clear  (0)
// |= (1 << 31);  To Set    (1)
// REG & (1 << 9) To Check  (?)
// PIN -> 1 High 0 Low
// LEDs			 BUTTONS

GPIO1 -18 LED 0  GPIO0 -29 Button 0
GPIO1 -24 LED 1  GPIO0 -30 Button 1
GPIO1 -26 LED 2  GPIO1 -15 Button 2
GPIO2 -3  LED 3  GPIO1 -10 Button 3

void foo(void) {}
typedef void (*foo)(void);
const port_pin_s *led = (port_pin_s *)(params);
*/

// clang-format on

// static QueueHandle_t userinput;
// char buffer[4];
// void OLED__inputreceivetask(void *p) {
//   while (1) {

//     scanf("%c", buffer);
//     if (xQueueSend(userinput, &buffer, 100)) {
//     }
//     vTaskDelay(100);
//   }
// }

// void OLED__inputwritetask(void *p) {
//   while (1) {
//     if (xQueueReceive(userinput, &buffer, portMAX_DELAY))
//       print_OLED(buffer);
//   }
// }

/////////////////////////// MAIN ///////////////////////////
void slave_i2c_starter() {
  LPC_IOCON->P0_0 |= (1 << 10); // set to open drain mode
  LPC_IOCON->P0_1 |= (1 << 10);
  gpio__construct_with_function(GPIO__PORT_0, 0, GPIO__FUNCTION_3); // I2C1_SDA  //Slave
  gpio__construct_with_function(GPIO__PORT_0, 1, GPIO__FUNCTION_3); // I2C1_SCL  //
  int pick_slave_adr = 0x42;
  i2c__initialize(I2C__1, 400000, 96000000); // initialize slave as master first
  i2c2__slave_init(pick_slave_adr);          // turn I2c1 into slave, ADR1 set, CONSET 0x44

  /* From peripherals_init.c */
  for (unsigned slave_address = 2; slave_address <= 254; slave_address += 2) {
    if (i2c__detect(I2C__2, slave_address)) {
      printf("I2C slave detected at address: 0x%02X\n", slave_address);
    }
  }
}
void main(void) {
  printf("#########################################################################################\nSeparate I2C "
         "debug\n#########################################################################################");
  sj2_cli__init();
  slave_i2c_starter();

  vTaskStartScheduler();
}
//////////////////////////END MAIN/////////////////////////
