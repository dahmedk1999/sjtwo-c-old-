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
#include "ff.h"
#include <string.h>
#include "OLED.h"
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
static QueueHandle_t sensor_queue;

typedef struct {
  double sample[100];
  double sum;
  double avg;
} SampleData;
SampleData Data;

void write_file_using_fatfs_pi(void) {
  const char *filename = "file.txt";
  FIL file; // File handle
  UINT bytes_written = 0;
  FRESULT result = f_open(&file, filename, (FA_WRITE | FA_CREATE_ALWAYS));

  if (FR_OK == result) {
    char string[64];
    sprintf(string, "Value,%i\n", rand() % 1000);
    if (FR_OK == f_write(&file, string, strlen(string), &bytes_written)) {
    } else {
      printf("ERROR: Failed to write data to file\n");
    }
    f_close(&file);
  } else {
    printf("ERROR: Failed to open: %s\n", filename);
  }
}

void sensor_producer(void *p) {
  while (1) {
    printf("Sampling...\n");
    for (int i = 0; i < 100; i++) // This is where the sensor sampling will be inserted
      Data.sum += Data.sample[i]; // Input each sample into the sample array
    Data.avg = Data.sum / 100;    // Send average to queue
    if (xQueueSend(sensor_queue, &Data, 0))
      printf("Sample average sent");

    vTaskDelay(100);
  }
}

void sensor_consumer(void *p) {
  while (1) {
    printf("Before writing.\n");
    if (xQueueReceive(sensor_queue, &Data, portMAX_DELAY))
      printf("After receiving\n"); // SDCard writing goes here
  }
}

/////////////////////////// MAIN ///////////////////////////

void main(void) {

  // sj2_cli__init();
  // sensor_queue = xQueueCreate(5, sizeof(SampleData));
  // xTaskCreate(sensor_producer, "producer", 2048 / sizeof(void *), NULL, 3, NULL);
  // xTaskCreate(sensor_consumer, "consumer", 2048 / sizeof(void *), NULL, 1, NULL);
  // vTaskStartScheduler();

  configure_OLED();
  OLED_Start();
  print_OLED("01234567899876543210!@#$%^&*(){}][|><?/.,=_+-");
}
//////////////////////////END MAIN/////////////////////////
