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
#include "LPC40xx.h"
#include "gpio.h"
#include "gpio_isr.h"
#include "gpio_lab.h"
#include "lpc_peripherals.h"
#include "queue.h"
#include "semphr.h"
#include "ssp2lab.h"
#include "uart_lab.h"
#include <string.h>
#include "OLED.h"
#include "ff.h"




//////////////////////////END MAIN/////////////////////////
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
static QueueHandle_t display_songname;
char buffer[16];

void OLED__inputreceivetask(void *p) {
  while (1) {
    scanf("%c", buffer);
    if (xQueueSend(userinput, &buffer, 10)) {
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
void discover_mp3_files_task(void *p) {
  FRESULT file_result; /* Return value */
  DIR dir_obj;         /* Directory object */
  FILINFO file_info;   /* File information */

  file_result = f_findfirst(&dir_obj, &file_info, "", "*.mp3"); /* Start to search for mp3 files */

  while (file_result == FR_OK && file_info.fname[0]) { /* Repeat while an item is found */
    printf("%s\n", file_info.fname);                   /* Print the object name */
    if (xQueueSend(display_songname, &file_info.fname, 0))
      file_result = f_findnext(&dir_obj, &file_info); /* Search for next item */
  }
  f_closedir(&dir_obj);
  vTaskSuspend(NULL);
}

void write_songlist_to_OLED(void *p) {
  char filename[129];
  while (1) {
    if (xQueueReceive(display_songname, &filename, portMAX_DELAY))
      print_OLED(filename);
  }
}

void main(void) {
  display_songname = xQueueCreate(8, 129);
  userinput = xQueueCreate(8, sizeof(buffer));

  configure_OLED();
  OLED_Start();
  // sj2_cli__init(); //CLI EATS UP USER INPUT. DO NOT USE WITH OLED INPUT
  xTaskCreate(OLED__inputreceivetask, "oledinput", 512, NULL, 2, NULL);
  xTaskCreate(OLED__inputwritetask, "oledwrite", 512, NULL, 2, NULL);
  // xTaskCreate(discover_mp3_files_task, "discover", 2048, NULL, 2, NULL);
  // xTaskCreate(write_songlist_to_OLED, "writesong", 2048, NULL, 2, NULL);

  vTaskStartScheduler();
}