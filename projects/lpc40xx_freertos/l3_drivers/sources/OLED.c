#include "OLED.h"
#include "clock.h"
#include "gpio.h"
#include "gpio_lab.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdio.h>
// 4 wire spi control
// Pins D0, D1 connect to CLK and MOSI, respectively
// Pins /CS, D/C connect to P1.22 and P1.25, respectively

/*
Function        E(RD#)  R/W#(WR#) CS# D/C# D0
Write command   LOW     LOW       L   L    ↑
Write data      LOW     LOW       L   H    ↑
*/

void spi1__init() { // MINIMUM  CLOCK CYCLE == 100ns for OLED Display, 10MHz
  // Refer to LPC User manual and setup the register bits correctly
  uint32_t user_clock_mhz = 9;
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP1);
  // b) Setup control registers CR0 and CR1
  LPC_SSP1->CR0 |= 7;        // we are sending only 8 bits at a time
  LPC_SSP1->CR1 |= (1 << 1); // enable SSP
  // c) Setup prescalar register to be <= user_clock_mhz

  uint8_t div = 2;
  const uint32_t cpu_clock_mhz = clock__get_core_clock_hz();
  while ((user_clock_mhz * 1000 * 1000) < (cpu_clock_mhz / div) && div <= 254) {
    div += 2;
  }
  fprintf(stderr, "OLED_CLOCK Divider= %d %d/%d = %d, \n", div, cpu_clock_mhz, user_clock_mhz * 1000 * 1000, div);
  LPC_SSP1->CPSR = div; // 10MHz is OLED limit
}

uint8_t ssp1__exch_byte(uint8_t data_out) {
  LPC_SSP1->DR = data_out;
  while (LPC_SSP1->SR & (1 << 4)) {
    /* fprintf(stderr, "FIFO is busy...\n") */; // when idle, it returns
  }

  return (uint8_t)(LPC_SSP1->DR & 0xFF); // right justify the data bits
}

void cs_OLED() {
  gpioX__set_as_output(1, 22);
  gpioX__set_low(1, 22);
}
void ds_OLED() {
  gpioX__set_as_output(1, 22);
  gpioX__set_high(1, 22);
}

void power_up();
void power_down();