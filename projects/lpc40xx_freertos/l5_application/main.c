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
//#include "i2c_slave_init.h"

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

static QueueHandle_t userinput;
char buffer[4];
void OLED__inputreceivetask(void *p) {
  while (1) {

    scanf("%c", buffer);
    if (xQueueSend(userinput, &buffer, 100)) {
    }
    vTaskDelay(100);
  }
}

void OLED__inputwritetask(void *p) {
  while (1) {
    if (xQueueReceive(userinput, &buffer, portMAX_DELAY))
      print_OLED(buffer);
  }
}
/////////////////////////// MAIN ///////////////////////////
void main(void) {
  printf("\n\n\n\nSeparate I2C debug\n\n\n\n#############################################################################################################"
         "##################");
  // i2c__initialize(I2C__2, UINT32_C(400) * 1000, clock__get_peripheral_clock_hz());
  // i2c_Slave_initialize(I2C__0, UINT32_C(400) * 1000, clock__get_peripheral_clock_hz());
  gpio__construct_with_function(GPIO__PORT_0, 10, GPIO__FUNCTION_2); // I2C2_SDA  //Master
  gpio__construct_with_function(GPIO__PORT_0, 11, GPIO__FUNCTION_2); // I2C2_SCL  //
  gpio__construct_with_function(GPIO__PORT_0, 0, GPIO__FUNCTION_3);  // I2C1_SDA  //Slave
  gpio__construct_with_function(GPIO__PORT_0, 1, GPIO__FUNCTION_3);  // I2C1_SCL  //

  // const uint32_t i2c_speed_hz = UINT32_C(400) * 1000;
  // i2c_slave_initialize(I2C__1, i2c_speed_hz, 96000000);

  sj2_cli__init();
  // i2c__detect(I2C__1, 0x14);
  vTaskStartScheduler();
}
//////////////////////////END MAIN/////////////////////////
