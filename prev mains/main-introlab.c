// clang-format off
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#include "LPC40xx.h"
#include "gpio_isr.h"
#include "gpio_lab.h"
#include "lpc_peripherals.h"
#include "semphr.h"

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

uint8_t GPIOPORTs[] = {1, 1, 1, 2};
uint8_t GPIOLEDs[] = {18, 24, 26, 3};

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

typedef struct {
  uint8_t port;
  uint8_t pin;
} port_pin_s;

static SemaphoreHandle_t switch_press_indication0, switch_press_indication1, switch_press_indication2,
    switch_press_indication3;

void switch_task(void *param) {
  port_pin_s *sw = (port_pin_s *)(param);
  gpioX__set_as_input(sw->port, sw->pin);
  while (true) {
    if (gpioX__get_level(sw->port, sw->pin)) {
      xSemaphoreGive(switch_press_indication0);
    }
    vTaskDelay(300);
  }
}
void led_task(void *param) {
  const port_pin_s *led = (port_pin_s *)(param);
  while (true) {
    gpioX__set_as_output(led->port, led->pin);
    if (xSemaphoreTake(switch_press_indication0, 1000)) {
      gpioX__set(led->port, led->pin, !gpioX__get_level(led->port, led->pin)); // ezpz toggle
      vTaskDelay(100);
    } else {
      puts("Timeout: No switch press indication for 1000ms");
    }
  }
}
////////////////////////////// EXTRA CREDIT

void switch_task1(void *param) {
  port_pin_s *sw = (port_pin_s *)(param);
  gpioX__set_as_input(sw->port, sw->pin);
  while (true) {
    if (gpioX__get_level(sw->port, sw->pin)) {
      xSemaphoreGive(switch_press_indication1);
    }
    vTaskDelay(300);
  }
}
void ecled(void *param) {
  // const port_pin_s *led = (port_pin_s *)(param);
  while (true) {
    // gpioX__set_as_output(led->port, led->pin);
    // if (xSemaphoreTake(switch_press_indication1, 1000)) {
    for (int i = 0; i < 4; i++) {
      gpioX__set_low(GPIOPORTs[i], GPIOLEDs[i]);
      vTaskDelay(150);
      gpioX__set_high(GPIOPORTs[i], GPIOLEDs[i]);
      vTaskDelay(50);
    }
    for (int i = 3; i >= 0; i--) {
      gpioX__set_low(GPIOPORTs[i], GPIOLEDs[i]);
      vTaskDelay(150);
      gpioX__set_high(GPIOPORTs[i], GPIOLEDs[i]);
      vTaskDelay(50);
    }
    //}
  }
}

void intr_switch_task(void *param) {
  port_pin_s *sw = (port_pin_s *)(param);
  gpioX__set_as_input(sw->port, sw->pin);
  while (true) {
    if (gpioX__get_level(sw->port, sw->pin)) {
      xSemaphoreGive(switch_press_indication0);
    }
    vTaskDelay(300);
  }
}

void clear_pin_interrupts(int port, int pin) {
  switch (port) {
  case 0:
    LPC_GPIOINT->IO0IntClr |= (1 << pin);
    break;
  case 2:
    LPC_GPIOINT->IO2IntClr |= (1 << pin);
  default:
    break;
  }
}

void gpio_interrupt(void) {
  LPC_GPIOINT->IO0IntClr |= (1 << 29);
  gpioX__set_low(1, 18);
  fprintf(stderr, "Enter ISR\n");
  fprintf(stderr, "Exit Isr\n");
  gpioX__set_high(1, 18);
}

void gpio_interrupt_semphr(void) {
  //LPC_GPIOINT->IO0IntClr |= (1 << 30);
  xSemaphoreGiveFromISR(switch_press_indication1, NULL);
}
void sleep_on_semphr_task(void *param) {
  while (1)
    if (xSemaphoreTake(switch_press_indication1, portMAX_DELAY))
      for (int i = 0; i <= 3; i++) {
        gpioX__toggle(GPIOPORTs[i],GPIOLEDs[i]);
        fprintf(stderr, "Led%d toggled\n", i);
        vTaskDelay(100);
      }
}

int detect_rise(uint8_t port) {
  switch (port) {
  case 0: {
    for (int i = 0; i < 32; i++) {
      if (LPC_GPIOINT->IO0IntStatR & (1 << i))
        return i;
    }
    return -1;
  } break;
  case 2: {
    for (int i = 0; i < 32; i++) {
      if (LPC_GPIOINT->IO2IntStatR & (1 << i))
        return i;
    }
    return -1;
  } break;
  default:
    break;
  }
}
int detect_fall(uint8_t port) {
  switch (port) {
  case 0: {
    for (int i = 0; i < 32; i++) {
      if (LPC_GPIOINT->IO0IntStatF & (1 << i))
        return i;
    }
    return -1;
  } break;
  case 2: {
    for (int i = 0; i < 32; i++) {
      if (LPC_GPIOINT->IO2IntStatF & (1 << i))
        return i;
    }
    return -1;
  } break;
  default:
    break;
  }
}
// clang-format on
/////////////////////////// MAIN ///////////////////////////
int main(void) {
  switch_press_indication0 = switch_press_indication1 = switch_press_indication2 = switch_press_indication3 =
      xSemaphoreCreateBinary();

  // NVIC_EnableIRQ(GPIO_IRQn);
  // static port_pin_s sw = {0, 29}, sw1 = {0, 30}, sw2 = {1, 15}, sw3 = {1, 10};
  // static port_pin_s led0 = {1, 18}, led1 = {1, 24}, led2 = {1, 26}, led3 = {2, 3};
  // static port_pin_s LEDarray[4] = {{1, 18}, {1, 24}, {1, 26}, {2, 3}};

  gpioX__attach_interrupt(0, 30, GPIO_INTR__FALLING_EDGE, gpio_interrupt_semphr);
  // lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio_interrupt_semphr, '0');
  // lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio_interrupt, '0');
  // lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpioX__interrupt_dispatcher, 'This is the interrupt
  // dispatcher');// can also use this method instead of changing IVT
  xTaskCreate(sleep_on_semphr_task, "SemphrINTledflash", 2048 / sizeof(void *), NULL, 1, NULL);
  vTaskStartScheduler();

  return 0;
}
//////////////////////////END MAIN/////////////////////////

//

//

//

//

//

//
/*////////////////////////////////////////////////////*/
static void create_blinky_tasks(void) {
  /**
   * Use '#if (1)' if you wish to observe how two tasks can blink LEDs
   * Use '#if (0)' if you wish to use the 'periodic_scheduler.h' that will spawn 4 periodic tasks, one for each LED
   */
#if (1)
  // These variables should not go out of scope because the 'blink_task' will reference this memory
  static gpio_s led0, led1;

  led0 = board_io__get_led0();
  led1 = board_io__get_led1();

  xTaskCreate(blink_task, "led0", configMINIMAL_STACK_SIZE, (void *)&led0, PRIORITY_LOW, NULL);
  xTaskCreate(blink_task, "led1", configMINIMAL_STACK_SIZE, (void *)&led1, PRIORITY_LOW, NULL);
#else
  const bool run_1000hz = true;
  const size_t stack_size_bytes = 2048 / sizeof(void *); // RTOS stack size is in terms of 32-bits for ARM M4 32-bit CPU
  periodic_scheduler__initialize(stack_size_bytes, !run_1000hz); // Assuming we do not need the high rate 1000Hz task
  UNUSED(blink_task);
#endif
}

static void create_uart_task(void) {
  // It is advised to either run the uart_task, or the SJ2 command-line (CLI), but not both
  // Change '#if (0)' to '#if (1)' and vice versa to try it out
#if (0)
  // printf() takes more stack space, size this tasks' stack higher
  xTaskCreate(uart_task, "uart", (512U * 8) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#else
  sj2_cli__init();
  UNUSED(uart_task); // uart_task is un-used in if we are doing cli init()
#endif
}

static void blink_task(void *params) {
  const gpio_s led = *((gpio_s *)params); // Parameter was input while calling xTaskCreate()

  // Warning: This task starts with very minimal stack, so do not use printf() API here to avoid stack overflow
  while (true) {
    gpio__toggle(led);
    vTaskDelay(500);
  }
}

// This sends periodic messages over printf() which uses system_calls.c to send them to UART0
static void uart_task(void *params) {
  TickType_t previous_tick = 0;
  TickType_t ticks = 0;

  while (true) {
    // This loop will repeat at precise task delay, even if the logic below takes variable amount of ticks
    vTaskDelayUntil(&previous_tick, 2000);

    /* Calls to fprintf(stderr, ...) uses polled UART driver, so this entire output will be fully
     * sent out before this function returns. See system_calls.c for actual implementation.
     *
     * Use this style print for:
     *  - Interrupts because you cannot use printf() inside an ISR
     *    This is because regular printf() leads down to xQueueSend() that might block
     *    but you cannot block inside an ISR hence the system might crash
     *  - During debugging in case system crashes before all output of printf() is sent
     */
    ticks = xTaskGetTickCount();
    fprintf(stderr, "%u: This is a polled version of printf used for debugging ... finished in", (unsigned)ticks);
    fprintf(stderr, " %lu ticks\n", (xTaskGetTickCount() - ticks));

    /* This deposits data to an outgoing queue and doesn't block the CPU
     * Data will be sent later, but this function would return earlier
     */
    ticks = xTaskGetTickCount();
    printf("This is a more efficient printf ... finished in");
    printf(" %lu ticks\n\n", (xTaskGetTickCount() - ticks));
  }
}
