#include "ssp2lab.h"
#include "clock.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdio.h>
void ssp2__init(uint32_t user_clock_mhz) {
  // Refer to LPC User manual and setup the register bits correctly

  // a) Power on Peripheral
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP2);
  // b) Setup control registers CR0 and CR1
  LPC_SSP2->CR0 |= 7;        // we are sending only 8 bits at a time
  LPC_SSP2->CR1 |= (1 << 1); // enable SSP
  // c) Setup prescalar register to be <= user_clock_mhz
  // Reused from ADC clock divider

  uint8_t div = 2;
  const uint32_t cpu_clock_mhz = clock__get_core_clock_hz();
  while (user_clock_mhz * 1000 < (cpu_clock_mhz / div) && div <= 254) {
    div += 2;
  }
  // if the cpu clock speed is less than user clock AND 24mhz, set the divider
  LPC_SSP2->CPSR = div; // 24MHz is SPI clock limit
}

uint8_t ssp2__exch_byte(uint8_t data_out) {
  // Configure the Data register(DR) to send and receive data by checking the SPI peripheral status register
  /*
  B  S                                                                                                    R
  0 TFE Transmit FIFO Empty. This bit is 1 is the Transmit FIFO is empty, 0 if not.                       1
  1 TNF Transmit FIFO Not Full. This bit is 0 if the Tx FIFO is full, 1 if not.                           1
  2 RNE Receive FIFO Not Empty. This bit is 0 if the Receive FIFO is empty, 1 if not.                     0
  3 RFF Receive FIFO Full. This bit is 1 if the Receive FIFO is full, 0 if not.                           0
  4 BSY Busy. This bit is 0 if the SSPn controller is idle, or 1 if it is currently sending/receiving a
  frame and/or the Tx FIFO is not empty.                                                                  0
  */
  LPC_SSP2->DR = data_out;
  /*
    if (LPC_SSP2->SR & (1 << 0))
      fprintf(stderr, "Transmission FIFO is EMPTY!\n");
    else
      fprintf(stderr, "Transmission FIFO is not empty.\n");
    if (LPC_SSP2->SR & (1 << 1))
      fprintf(stderr, "Transmission FIFO is not full.\n");
    else
      fprintf(stderr, "Transmission FIFO is FULL!\n");
    if (LPC_SSP2->SR & (1 << 2))
      fprintf(stderr, "Receiving FIFO is not empty.\n");
    else
      fprintf(stderr, "Receiving FIFO is EMPTY!\n");
    if (LPC_SSP2->SR & (1 << 3))
      fprintf(stderr, "Receiving FIFO is FULL!\n");
    else
      fprintf(stderr, "Receiving FIFO is not full.\n");
   */
  while (LPC_SSP2->SR & 1 << 4) {
    /* fprintf(stderr, "FIFO is busy...\n") */; // when idle, it returns
  }

  return (uint8_t)(LPC_SSP2->DR & 0xFF); // right justify the data bits
}