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


void adesto_cs(void) {
  gpioX__set_as_output(1, 10);
  gpioX__set_low(1, 10);
  gpio__construct_as_output(4, 28); // external gpio pin to use as trigger
  gpioX__set_low(4, 28);
} // P1_10 is connected to Flash memory CS
void adesto_ds(void) {
  gpioX__set_as_output(1, 10);
  gpioX__set_high(1, 10);
  gpio__construct_as_output(4, 28); // external gpio pin to use as trigger
  gpioX__set_high(4, 28);
} // deselect
typedef struct {
  uint8_t manufacturer_id;
  uint8_t device_id_1;
  uint8_t device_id_2;
  uint8_t extended_device_id;
} adesto_flash_id_s;
adesto_flash_id_s adesto_read_signature(void) {

  adesto_flash_id_s data = {0};
  adesto_cs();
  (void)ssp2__exch_byte(0x9F);                     // Opcode 9Fh, HIGH IMPEDANCE
  data.manufacturer_id = ssp2__exch_byte(0x9F);    // Manufacturer ID returned
  data.device_id_1 = ssp2__exch_byte(0x9F);        // next byte is id1
  data.device_id_2 = ssp2__exch_byte(0x9F);        // next byte is id2
  data.extended_device_id = ssp2__exch_byte(0x9F); // last byte is extended_device id, which should be 00h
  adesto_ds();
  return data;
}
void todo_configure_your_ssp2_pin_functions() {
  gpio__construct_with_function(1, 4, GPIO__FUNCTION_4); // Set as SSP2MISO
  gpio__construct_with_function(1, 0, GPIO__FUNCTION_4); // SSP2_SCK
  gpio__construct_with_function(1, 1, GPIO__FUNCTION_4); // Set as SSP2MOSI
}
void spi_task(void *p) {
  const uint32_t spi_clock_mhz = 24;
  ssp2__init(spi_clock_mhz);
  todo_configure_your_ssp2_pin_functions();
  while (1) {
    adesto_flash_id_s id = adesto_read_signature();
    // TODO: printf the members of the 'adesto_flash_id_s' struct
    printf("Manufacturer ID   : %0x\n", id.manufacturer_id);
    vTaskDelay(500);
    printf("Device ID1        : %0x\n", id.device_id_1);
    vTaskDelay(500);
    printf("Device ID2        : %0x\n", id.device_id_2);
    vTaskDelay(500);
    printf("Extended Device ID: %0x\n", id.extended_device_id);
    vTaskDelay(500);
  }
}
adesto_flash_id_s ssp2_adesto_read_signature(void) {

  adesto_flash_id_s data = {0};
  // gpio__construct_as_output(4, 28); // external gpio pin to use as trigger
  // gpioX__set_as_output(4, 28);      //
  // gpioX__set_low(4, 28);            //
  adesto_cs();
  (void)ssp2__exch_byte(0x9F);                     // Opcode 9Fh, HIGH IMPEDANCE
  data.manufacturer_id = ssp2__exch_byte(0x9F);    // Manufacturer ID returned
  data.device_id_1 = ssp2__exch_byte(0x00);        // next byte is id1
  data.device_id_2 = ssp2__exch_byte(0x00);        // next byte is id2
  data.extended_device_id = ssp2__exch_byte(0x00); // last byte is extended_device id, which should be 00h
  adesto_ds();
  // gpioX__set_high(4, 28);
  return data;
}
void spi_id_verification_task(void *p) {
  const uint32_t spi_clock_mhz = 24;
  ssp2__init(spi_clock_mhz);
  todo_configure_your_ssp2_pin_functions();
  while (1) {
    if (xSemaphoreTake(mutex_SSP2, 1000)) {
      const adesto_flash_id_s id = ssp2_adesto_read_signature();

      // When we read a manufacturer ID we do not expect, we will kill this task
      if (id.manufacturer_id != 0x1F) {
        fprintf(stderr, "Manufacturer ID read failure\n");
        vTaskSuspend(NULL); // Kill this task
      }
      printf("Manufacturer ID   : %0x\n", id.manufacturer_id);

      printf("Device ID1        : %0x\n", id.device_id_1);

      printf("Device ID2        : %0x\n", id.device_id_2);

      printf("Extended Device ID: %0x\n", id.extended_device_id);
      xSemaphoreGive(mutex_SSP2);
      vTaskDelay(100);
    }
  }
}
typedef struct page {
  uint8_t byte[256];
  uint32_t address;
} page;
void adesto_flash_send_address(uint32_t address) {
  (void)ssp2__exchange_byte((address >> 16) & 0xFF);
  (void)ssp2__exchange_byte((address >> 8) & 0xFF);
  (void)ssp2__exchange_byte((address >> 0) & 0xFF);
}
void write_enable() {
  adesto_cs();
  ssp2__exch_byte(0x06); // WEL to 1
  adesto_ds();
}
void write_disable() {
  adesto_cs();
  ssp2__exch_byte(0x04); // WEL to 0
  adesto_ds();
}
void unblock_mem(void) {
  adesto_cs();
  ssp2__exch_byte(0x01);
  ssp2__exch_byte(0x00);
  adesto_ds();
}
void page_erase(uint8_t address) {
  write_enable();
  adesto_cs();
  ssp2__exch_byte(0x81);
  ssp2__exch_byte(0xFF);
  ssp2__exch_byte(address);
  ssp2__exch_byte(0xFF);
  adesto_ds();
}
void page_write(uint32_t address, uint8_t *data) {

  write_enable();
  adesto_cs();           // Chip enable
  ssp2__exch_byte(0x02); // page opcode
  adesto_flash_send_address(address);
  for (int i = 0; i < 25; i++) {
    ssp2__exch_byte(data++);
  }
  
  adesto_ds();
  write_disable();
}
void page_read(uint32_t address, uint8_t *data) {
  adesto_cs();           // Chip enable
  ssp2__exch_byte(0x03); // Read array opcode 03
  adesto_flash_send_address(address);
  for (int i = 0; i < 25; i++) {
    data[i] = ssp2__exch_byte(0x00); // test data
  }
  adesto_ds();
}
page pageofdata;
void pagewriteandread(void *p) {
  unblock_mem();
  while (1) {
    uint8_t readdata[256];
    page *PAGE = (page *)(p);
    // page_erase(0x00);
    page_write(PAGE->address, PAGE->byte);
    for (int i = 0; i < 25; i++) {
      fprintf(stderr, "Write Address : %0x, Data: %0x\n", PAGE->address + i, PAGE->byte[i]);
    }
    fprintf(stderr, "\n##### Writing finished #####\n\n");
    vTaskDelay(1000);
    page_read(PAGE->address, readdata);
    for (int i = 0; i < 25; i++) {
      fprintf(stderr, "Read Address  : %0x, Data: %0x\n", PAGE->address + i, readdata[i]);
    }
    fprintf(stderr, "\n##### Reading finished #####\n\n");
    vTaskDelay(1000);
  }
}
void ssp2lab(){
  
  mutex_SSP2 = xSemaphoreCreateMutex();
  /* Part 1 */
  // xTaskCreate(spi_task, "SPID", 2048 / sizeof(void *), NULL, 2, NULL);

  /* Part 2b */
  // xTaskCreate(spi_id_verification_task, "SPIDV1", 2048 / sizeof(void *), NULL, 1, NULL);
  // xTaskCreate(spi_id_verification_task, "SPIDV2", 2048 / sizeof(void *), NULL, 1, NULL);

  /* Part 2a Extra credit */
  const uint32_t spi_clock_mhz = 12;
  ssp2__init(spi_clock_mhz);
  todo_configure_your_ssp2_pin_functions();
  pageofdata.address = 0x0000;
  for (int i = 0; i < 25; i++)
    pageofdata.byte[i] = i;

  xTaskCreate(pagewriteandread, "WR_SPIPAGE", 2048, &pageofdata, 2, NULL);

  vTaskStartScheduler();
  
}
