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


typedef struct {
  uint8_t port;
  uint8_t pin;
} port_pin_s;

static SemaphoreHandle_t switch_press_indication0, switch_press_indication1, switch_press_indication2,
    switch_press_indication3;

// void switch_task(void *param) {
//   port_pin_s *sw = (port_pin_s *)(param);
//   gpioX__set_as_input(sw->port, sw->pin);
//   while (true) {
//     if (gpioX__get_level(sw->port, sw->pin)) {
//       xSemaphoreGive(switch_press_indication0);
//     }
//     vTaskDelay(300);
//   }
// }
// void led_task(void *param) {
//   const port_pin_s *led = (port_pin_s *)(param);
//   while (true) {
//     gpioX__set_as_output(led->port, led->pin);
//     if (xSemaphoreTake(switch_press_indication0, 1000)) {
//       gpioX__set(led->port, led->pin, !gpioX__get_level(led->port, led->pin)); // ezpz toggle
//       vTaskDelay(100);
//     } else {
//       puts("Timeout: No switch press indication for 1000ms");
//     }
//   }
// }
////////////////////////////// EXTRA CREDIT

// void switch_task1(void *param) {
//   port_pin_s *sw = (port_pin_s *)(param);
//   gpioX__set_as_input(sw->port, sw->pin);
//   while (true) {
//     if (gpioX__get_level(sw->port, sw->pin)) {
//       xSemaphoreGive(switch_press_indication1);
//     }
//     vTaskDelay(300);
//   }
// }
// void ecled(void *param) {
//   // const port_pin_s *led = (port_pin_s *)(param);
//   while (true) {
//     // gpioX__set_as_output(led->port, led->pin);
//     // if (xSemaphoreTake(switch_press_indication1, 1000)) {
//     for (int i = 0; i < 4; i++) {
//       gpioX__set_low(GPIOPORTs[i], GPIOLEDs[i]);
//       vTaskDelay(150);
//       gpioX__set_high(GPIOPORTs[i], GPIOLEDs[i]);
//       vTaskDelay(50);
//     }
//     for (int i = 3; i >= 0; i--) {
//       gpioX__set_low(GPIOPORTs[i], GPIOLEDs[i]);
//       vTaskDelay(150);
//       gpioX__set_high(GPIOPORTs[i], GPIOLEDs[i]);
//       vTaskDelay(50);
//     }
//     //}
//   }
// }

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

// void gpio_interrupt(void) {
//   LPC_GPIOINT->IO0IntClr |= (1 << 29);
//   gpioX__set_low(1, 18);
//   fprintf(stderr, "Enter ISR\n");
//   fprintf(stderr, "Exit Isr\n");
//   gpioX__set_high(1, 18);
// }

void port0pin29isr(void) {
  LPC_GPIOINT->IO0IntClr |= (1 << 29);
  xSemaphoreGiveFromISR(switch_press_indication0, NULL);
}
void port0pin30isr(void) {
  LPC_GPIOINT->IO0IntClr |= (1 << 30);
  xSemaphoreGiveFromISR(switch_press_indication1, NULL);
}


void sleep_on_semphr_task0(void *param) {
  vTaskDelay(33);
  while (1)
    if (xSemaphoreTake(switch_press_indication0, portMAX_DELAY)) {
        gpioX__toggle(GPIOPORTs[0], GPIOLEDs[0]);
        gpioX__toggle(GPIOPORTs[3], GPIOLEDs[3]);
        fprintf(stderr, "Led 0 and 3 toggled\n");
        vTaskDelay(100);
      }
    }
void sleep_on_semphr_task1(void *param) {
  vTaskDelay(33);
  while (1)
    if (xSemaphoreTake(switch_press_indication1, portMAX_DELAY)) {
        gpioX__toggle(GPIOPORTs[1], GPIOLEDs[1]);
        gpioX__toggle(GPIOPORTs[2], GPIOLEDs[2]);
        fprintf(stderr, "Led 1 and 2 toggled\n");
        vTaskDelay(100);
      }
    }
int detect_rise(uint8_t port) {
  if (port == 0) {
    for (int i = 0; i <= 31; i++) {
      if (LPC_GPIOINT->IO0IntStatR & (1 << i))
        return i;
    }
    return -1;
  } else if (port == 2) {
    for (int i = 0; i <= 31; i++) {
      if (LPC_GPIOINT->IO2IntStatR & (1 << i))
        return i;
    }
    return -1;
  } else
    return -1;
}
int detect_fall(uint8_t port) {
  if (port == 0) {
    for (int i = 0; i <= 31; i++) {
      if (LPC_GPIOINT->IO0IntStatF & (1 << i))
        return i;
    }
    return -1;
  } else if (port == 2) {
    for (int i = 0; i <= 31; i++) {
      if (LPC_GPIOINT->IO2IntStatF & (1 << i))
        return i;
    }
    return -1;
  } else
    return -1;
}
// clang-format on
/////////////////////////// MAIN ///////////////////////////
int main(void) {
  switch_press_indication0 = xSemaphoreCreateBinary();
  switch_press_indication1 = xSemaphoreCreateBinary();
  switch_press_indication2 = xSemaphoreCreateBinary();
  switch_press_indication3 = xSemaphoreCreateBinary();

  // NVIC_EnableIRQ(GPIO_IRQn);
  // static port_pin_s sw = {0, 29}, sw1 = {0, 30}, sw2 = {1, 15}, sw3 = {1, 10};
  // static port_pin_s led0 = {1, 18}, led1 = {1, 24}, led2 = {1, 26}, led3 = {2, 3};
  // static port_pin_s LEDarray[4] = {{1, 18}, {1, 24}, {1, 26}, {2, 3}};

  gpioX__attach_interrupt(0, 29, GPIO_INTR__RISING_EDGE, port0pin29isr);
  gpioX__attach_interrupt(0, 30, GPIO_INTR__RISING_EDGE, port0pin30isr);

  // lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio_interrupt_semphr, '0');
  // lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio_interrupt, '0');
  // lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpioX__interrupt_dispatcher, 'This is the interrupt
  // dispatcher');// can also use this method instead of changing IVT
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpioX__interrupt_dispatcher, 'd');
  xTaskCreate(sleep_on_semphr_task0, "isr29", 2048 / sizeof(void *), NULL, 2, NULL);
  xTaskCreate(sleep_on_semphr_task1, "isr30", 2048 / sizeof(void *), NULL, 2, NULL);

  vTaskStartScheduler();

  return 0;
}
//////////////////////////END MAIN/////////////////////////