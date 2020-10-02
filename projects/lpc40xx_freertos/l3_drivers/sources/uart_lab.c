#include "uart_lab.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdio.h>
void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
  // Refer to LPC User manual and setup the register bits correctly
  // The first page of the UART chapter has good instructions
  // a) Power on Peripheral
  // b) Setup DLL, DLM, FDR, LCR registers
  if (uart == 2) {
    lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__UART2);
    const uint16_t divider16bit = 96 * 1000 * 1000 / (16 * baud_rate);
    const uint8_t dlab = (1 << 7);
    LPC_UART2->LCR |= dlab; // Open control
    LPC_UART2->DLM = (divider16bit >> 8 & 0xFF);
    LPC_UART2->DLL = (divider16bit & 0xFF);
    LPC_UART2->LCR &= ~dlab; // Disable control
  } else if (uart == 3) {
    lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__UART3);
    const uint16_t divider16bit = 96 * 1000 * 1000 / (16 * baud_rate);
    const uint8_t dlab = (1 << 7);
    LPC_UART3->LCR |= dlab; // Open control
    LPC_UART3->DLM = (divider16bit >> 8 & 0xFF);
    LPC_UART3->DLL = (divider16bit & 0xFF);
    LPC_UART3->LCR &= ~dlab; // Disable control
  }
}

bool uart_lab__polled_get(uart_number_e uart, char *input_byte) {
  // a) Check LSR for Receive Data Ready
  // b) Copy data from RBR register to input_byte
  if (uart == 2) {
    LPC_UART2->LCR &= ~(1 << 7); // Dlab reset
    while (!LPC_UART2->LSR & 1 << 0)
      ; // While LSR's RDR is not set, wait until it is set (Ready)
    *input_byte = LPC_UART2->RBR;
    return true;
  } else if (uart == 3) {
    LPC_UART3->LCR &= ~(1 << 7); // Dlab reset
    while (!LPC_UART2->LSR & 1 << 0)
      ; // While LSR's RDR is not set, wait until it is set (Ready)
    *input_byte = LPC_UART2->RBR;
    return true;
  }
}

bool uart_lab__polled_put(uart_number_e uart, char output_byte) {
  // a) Check LSR for Transmit Hold Register Empty
  // b) Copy output_byte to THR register
  if (uart == 2) {
    LPC_UART2->LCR &= ~(1 << 7); // Dlab reset
    while (!(LPC_UART2->LSR & 1 << 5))
      ;                           // THR is 0 | contains valid data. 1 when empty.
    LPC_UART2->THR = output_byte; // Send data when THR is empty
    return true;
  } else if (uart == 3) {
    LPC_UART3->LCR &= ~(1 << 7); // Dlab reset
    while (!(LPC_UART3->LSR & 1 << 5))
      ;                           // THR is 0 | contains valid data. 1 when empty.
    LPC_UART3->THR = output_byte; // Send data when THR is empty
    return true;
  }
}