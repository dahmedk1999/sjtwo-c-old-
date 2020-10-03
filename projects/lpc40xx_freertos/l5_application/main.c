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

uint8_t GPIOPORTs[] = {1, 1, 1, 2};
uint8_t GPIOLEDs[] = {18, 24, 26, 3};

typedef struct {
  uint8_t port;
  uint8_t pin;
} port_pin_s;

typedef struct {
  uint8_t R;
  uint8_t G;
  uint8_t B;
} RGB;
RGB globalRGB = {25, 25, 25};
static SemaphoreHandle_t switch_press_indication0, switch_press_indication1, switch_press_indication2,switch_press_indication3;
static QueueHandle_t adc_to_pwm_task_queue, Q1;
SemaphoreHandle_t mutex_SSP2;
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

void port0pin29isr(void) {
  LPC_GPIOINT->IO0IntClr |= (1 << 29);
  xSemaphoreGiveFromISR(switch_press_indication0, NULL);
}
void port0pin30isr_RGB(void) {
  LPC_GPIOINT->IO0IntClr |= (1 << 30);
  globalRGB.R = rand() % 100;
  globalRGB.G = rand() % 100;
  globalRGB.B = rand() % 100;
  xSemaphoreGiveFromISR(switch_press_indication1, NULL);
}
void sleep_on_semphr_task0(void *param) {
  vTaskDelay(33);
  while (1)
    if (xSemaphoreTake(switch_press_indication0, portMAX_DELAY)) {
      gpioX__toggle(GPIOPORTs[0], GPIOLEDs[0]);
      gpioX__toggle(GPIOPORTs[1], GPIOLEDs[1]);
      gpioX__toggle(GPIOPORTs[2], GPIOLEDs[2]);
      gpioX__toggle(GPIOPORTs[3], GPIOLEDs[3]);
      vTaskDelay(100);
    }
}
void pin_configure_pwm_channel_as_io_pin() {
  gpio__construct_with_function(2, 0, GPIO__FUNCTION_1);
  gpio__construct_with_function(2, 1, GPIO__FUNCTION_1);
  gpio__construct_with_function(2, 2, GPIO__FUNCTION_1);
}
void pwm_task(void *p) {
  pwm1__init_single_edge(1000);

  // Locate a GPIO pin that a PWM channel will control
  // NOTE You can use gpio__construct_with_function() API from gpio.h
  // TODO Write this function yourself
  pin_configure_pwm_channel_as_io_pin();

  // We only need to set PWM configuration once, and the HW will drive
  // the GPIO at 1000Hz, and control set its duty cycle to 50%
  pwm1__set_duty_cycle(PWM1__2_0, 50);

  // Continue to vary the duty cycle in the loop
  uint8_t percent0 = 0;
  uint8_t percent1 = 0;
  uint8_t percent2 = 0;
  while (1) {
    pwm1__set_duty_cycle(PWM1__2_0, percent0);
    pwm1__set_duty_cycle(PWM1__2_1, percent1);
    pwm1__set_duty_cycle(PWM1__2_2, percent2);
    if (++percent0 > rand() % 100)
      percent0 = rand() % 100;
    vTaskDelay(60);
    if (++percent1 > rand() % 100)
      percent1 = rand() % 100;
    vTaskDelay(60);
    if (++percent2 > rand() % 100)
      percent2 = rand() % 100;
    vTaskDelay(60);
  }
}
void sleep_on_semphr_task1(void *param) {
  pwm1__init_single_edge(1000);
  pin_configure_pwm_channel_as_io_pin();
  pwm1__set_duty_cycle(PWM1__2_0, 0);

  if (xSemaphoreTake(switch_press_indication1, portMAX_DELAY)) {
    while (1) {
      fprintf(stderr, "\nRGB set to: %d, %d, %d", globalRGB.R, globalRGB.G, globalRGB.B);
      pwm1__set_duty_cycle(PWM1__2_0, globalRGB.R);
      pwm1__set_duty_cycle(PWM1__2_1, globalRGB.G);
      pwm1__set_duty_cycle(PWM1__2_2, globalRGB.B);
      vTaskDelay(200);
    }
  }
}
pin_configure_adc_channel_as_io_pin(adc_channel_e channel) {
  switch (channel) {
  case ADC__CHANNEL_2: {
    LPC_IOCON->P0_25 &= ~(1 << 7);
    gpio__construct_with_function(0, 25, GPIO__FUNCTION_1);
  } break;
  case ADC__CHANNEL_4: {
    LPC_IOCON->P1_30 &= ~(1 << 7);
    gpio__construct_with_function(1, 30, GPIO__FUNCTION_3);

  } break;
  case ADC__CHANNEL_5: {
    LPC_IOCON->P1_31 &= ~(1 << 7);
    gpio__construct_with_function(1, 31, GPIO__FUNCTION_3);
  } break;
  default:
    break;
  }
}
void adc_task(void *p) {
  adc__initialize();

  // Configure a pin, such as P1.31 with FUNC 011 to route this pin as ADC channel 5
  // You can use gpio__construct_with_function() API from gpio.h
  pin_configure_adc_channel_as_io_pin(ADC__CHANNEL_2);
  // TODO This is the function you need to add to adc.h
  // You can configure burst mode for just the channel you are using
  adc__enable_burst_mode(ADC__CHANNEL_2);
  while (1) {
    // Get the ADC reading using a new routine you created to read an ADC burst reading
    // TODO: You need to write the implementation of this function
    const uint16_t adc_value = adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_2);
    printf("ADC VALUE: %d\n", adc_value);
    vTaskDelay(100);
  }
}
void adc_taskP2(void *p) {
  adc__initialize();
  pin_configure_adc_channel_as_io_pin(ADC__CHANNEL_2);
  adc__enable_burst_mode(ADC__CHANNEL_2);
  while (1) {
    // Get the ADC reading using a new routine you created to read an ADC burst reading
    // TODO: You need to write the implementation of this function
    const int adc_value = (int)adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_2);
    xQueueSend(adc_to_pwm_task_queue, &adc_value, 0);
    printf("\nADC VALUE SENT: %d", adc_value);
    vTaskDelay(100);
  }
}
void pwm_taskP2(void *p) {
  pwm1__init_single_edge(1000);
  pin_configure_pwm_channel_as_io_pin();
  double adc_voltage = 0;
  int percent0 = 0;

  while (1) {
    if (xQueueReceive(adc_to_pwm_task_queue, &percent0, 100)) {
      adc_voltage = (double)percent0 / 4095 * 3.3;
      fprintf(stderr, "\nMatch Register 0: %ld\nMatch Register 1: %ld\n", LPC_PWM1->MR0, LPC_PWM1->MR1);
      if (percent0 < 55)
        percent0 = 0;
      else
        percent0 = ((double)percent0 / 4095 * 100); // led ends up flashing even at 1%
      printf("\nADC converted to %d%% = %.3fV\n", percent0, adc_voltage);

      pwm1__set_duty_cycle(PWM1__2_0, percent0 * 0.1);
      pwm1__set_duty_cycle(PWM1__2_1, percent0 * 0.2);
      pwm1__set_duty_cycle(PWM1__2_2, percent0);
    }
  }
}
void lab3(){
    srand((unsigned int)time(NULL));
  switch_press_indication0 = xSemaphoreCreateBinary();
  switch_press_indication1 = xSemaphoreCreateBinary();
  // switch_press_indication2 = xSemaphoreCreateBinary();
  // switch_press_indication3 = xSemaphoreCreateBinary();

  adc_to_pwm_task_queue = xQueueCreate(1, sizeof(int *));
  // static port_pin_s sw = {0, 29}, sw1 = {0, 30}, sw2 = {1, 15}, sw3 = {1, 10};
  // static port_pin_s LEDarray[4] = {{1, 18}, {1, 24}, {1, 26}, {2, 3}};

  /* Led toggle switch */
  gpioX__attach_interrupt(0, 29, GPIO_INTR__RISING_EDGE, port0pin29isr);
  /* Random RGB on switch */
  gpioX__attach_interrupt(0, 30, GPIO_INTR__RISING_EDGE, port0pin30isr_RGB);

  /* Intr Dispatcher  */
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpioX__interrupt_dispatcher, 'd');

  /* Led toggle switch */
  xTaskCreate(sleep_on_semphr_task0, "intrswitch0", 2048 / sizeof(void *), NULL, 1, NULL);

  /* Random RGB on switch */
  xTaskCreate(sleep_on_semphr_task1, "intrswitch1", 2048 / sizeof(void *), NULL, 2, NULL);

  /* RGB LEDs */
  // xTaskCreate(pwm_task, "pwmtest", 2048 / sizeof(void *), NULL, 1, NULL);

  /* Part 1, ADC OUTPUT */
  // xTaskCreate(adc_task, "ADC_output", 2048 / sizeof(void *), NULL, 2, NULL);

  /* Part 2 Queue RGB */
  // xTaskCreate(adc_taskP2, "ADC Producer", 2048 / sizeof(void *), NULL, 2, NULL);
  // xTaskCreate(pwm_taskP2, "Pwm Consumer", 2048 / sizeof(void *), NULL, 2, NULL);
  vTaskStartScheduler();

}

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

// clang-format on
// UART
void uart_read_task(void *p) {

  while (1) {
    char *data;
    uart_lab__polled_get(UART_3, &data);
    fprintf(stderr, "Received %c\n", data);
    vTaskDelay(500);
  }
}

void uart_write_task(void *p) {
  while (1) {
    // char data = 'A';
    uart_lab__polled_put(UART_3, 'B');
    // fprintf(stderr, "Sent %c\n", data);
    vTaskDelay(500);
  }
}

/////////////////////////// MAIN ///////////////////////////

void main(void) {
  // TODO: Use uart_lab__init() function and initialize UART2 or UART3 (your choice)
  // TODO: Pin Configure IO pins to perform UART2/UART3 function
  uart_lab__init(UART_3, 96, 115200);
  gpio__construct_with_function(4, 28, GPIO__FUNCTION_2);
  gpio__construct_with_function(4, 29, GPIO__FUNCTION_2);

  xTaskCreate(uart_write_task, "Ux3Write", 2048 / sizeof(void *), NULL, 2, NULL);
  xTaskCreate(uart_read_task, "Ux3Read", 2048 / sizeof(void *), NULL, 2, NULL);

  vTaskStartScheduler();
}
//////////////////////////END MAIN/////////////////////////
