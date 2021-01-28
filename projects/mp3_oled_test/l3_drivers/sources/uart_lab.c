#include "uart_lab.h"
#include "FreeRTOS.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "queue.h"
#include <stdio.h>
void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
  // Refer to LPC User manual and setup the register bits correctly
  // The first page of the UART chapter has good instructions
  // a) Power on Peripheral
  // b) Setup DLL, DLM, FDR, LCR registers
  if (uart == UART_2) {
    LPC_SC->PCONP |= (1 << 24);
    LPC_UART2->FCR = (1 << 0);

    const uint16_t divider16bit =
        (peripheral_clock * 1000 * 1000) / (16 * baud_rate + 0.5); // 0.5 is the error fraction

    const uint8_t dlab = (1 << 7);
    LPC_UART2->LCR |= dlab; // Open control
    LPC_UART2->LCR |= (3 << 0);

    LPC_UART2->DLM = ((divider16bit >> 8) & 0xFF);
    LPC_UART2->DLL = ((divider16bit >> 0) & 0xFF);
    // LPC_UART3->LCR &= ~dlab; // Disable control
    fprintf(stderr, "\n...UART 2 Ready\n");
  } else if (uart == UART_3) {
    // lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__UART3);
    LPC_SC->PCONP |= (1 << 25);
    LPC_UART3->FCR = (1 << 0);

    const uint16_t divider16bit = (peripheral_clock * 1000 * 1000) / (16 * baud_rate);

    const uint8_t dlab = (1 << 7);
    LPC_UART3->LCR |= dlab;     // Open control
    LPC_UART3->LCR |= (3 << 0); // Data length 8 bits

    LPC_UART3->DLM = ((divider16bit >> 8) & 0xFF);
    LPC_UART3->DLL = ((divider16bit >> 0) & 0xFF);
    // LPC_UART3->LCR &= ~dlab; // Disable control
    fprintf(stderr, "\n...UART 3 Ready\n");
  }
}
void UART3LSRcheck() {
  while (!(LPC_UART3->LSR & (1 << 5))) {
  }
}
void UART2LSRcheck() {
  while (!(LPC_UART2->LSR & (1 << 5))) {
  }
}
bool uart_lab__polled_get(uart_number_e uart, char *input_byte) {
  // a) Check LSR for Receive Data Ready
  // b) Copy data from RBR register to input_byte
  if (uart == UART_2) {
    LPC_UART2->LCR &= ~(1 << 7); // Dlab reset
    UART2LSRcheck();             // While LSR's RDR is not set, wait until it is set (Ready)
    *input_byte = LPC_UART2->RBR;
    return true;

  } else if (uart == UART_3) {
    LPC_UART3->LCR &= ~(1 << 7); // Dlab reset
    UART3LSRcheck();             // While LSR's RDR is not set, wait until it is set (Ready)
    *input_byte = LPC_UART3->RBR;
    return true;
  }
}

bool uart_lab__polled_put(uart_number_e uart, char output_byte) {
  // a) Check LSR for Transmit Hold Register Empty
  // b) Copy output_byte to THR register
  if (uart == UART_2) {

    LPC_UART2->LCR &= ~(1 << 7);
    UART2LSRcheck();              // THR is 0 | contains valid data. 1 when empty.
    LPC_UART2->THR = output_byte; // Send data when THR is empty
    UART2LSRcheck();
    return true;

  } else if (uart == UART_3) {

    LPC_UART3->LCR &= ~(1 << 7);
    UART3LSRcheck();              // THR is 0 | contains valid data. 1 when empty.
    LPC_UART3->THR = output_byte; // Send data when THR is empty
    UART3LSRcheck();
    return true;
  }
}

static QueueHandle_t your_uart_rx_queueU3, your_uart_rx_queueU2;
// Private function of our uart_lab.c
static void your_receive_interruptU3(void) {
  // TODO: Read the IIR register to figure out why you got interrupted
  // TODO: Based on IIR status, read the LSR register to confirm if there is data to be read

  // Do not care about bit 0
  if (LPC_UART3->IIR >> 1 & (0xf) == 2) // RDA Receive data available
    UART3LSRcheck();
  // TODO: Based on LSR status, read the RBR register and input the data to the RX Queue
  const char byte3 = LPC_UART3->RBR;
  xQueueSendFromISR(your_uart_rx_queueU3, &byte3, NULL);
}
static void your_receive_interruptU2(void) {
  // TODO: Read the IIR register to figure out why you got interrupted
  // TODO: Based on IIR status, read the LSR register to confirm if there is data to be read

  // Do not care about bit 0
  if (LPC_UART2->IIR >> 1 & (0xf) == 2) // RDA Receive data available
    UART2LSRcheck();
  // TODO: Based on LSR status, read the RBR register and input the data to the RX Queue
  const char byte2 = LPC_UART2->RBR;
  xQueueSendFromISR(your_uart_rx_queueU2, &byte2, NULL);
}
// Public function to enable UART interrupt
// TODO Declare this at the header file
void uart__enable_receive_interrupt(uart_number_e uart_number) {
  if (uart_number == UART_2) {
    // TODO: Use lpc_peripherals.h to attach your interrupt
    NVIC_EnableIRQ(UART2_IRQn); // ENABLE NVIC BEFORE INTERRUPT attachment
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART2, your_receive_interruptU2, NULL);

    // TODO: Enable UART receive interrupt by reading the LPC User manual
    LPC_UART2->LCR &= ~(1 << 7);
    LPC_UART2->IER |= (1 << 0); // SET Interrupts enable for RBR
    // Hint: Read about the IER register

    // TODO: Create your RX queue
    your_uart_rx_queueU2 = xQueueCreate(16, sizeof(char));
  } else if (uart_number == UART_3) {
    // TODO: Use lpc_peripherals.h to attach your interrupt
    NVIC_EnableIRQ(UART3_IRQn); // ENABLE NVIC BEFORE INTERRUPT attachment
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART3, your_receive_interruptU3, NULL);

    // TODO: Enable UART receive interrupt by reading the LPC User manual
    LPC_UART3->LCR &= ~(1 << 7);
    LPC_UART3->IER |= (1 << 0); // SET Interrupts enable for RBR
    // Hint: Read about the IER register

    // TODO: Create your RX queue
    your_uart_rx_queueU3 = xQueueCreate(16, sizeof(char));
  }
}

// Public function to get a char from the queue (this function should work without modification)
// TODO: Declare this at the header file
bool uart_lab__get_char_from_queue(uart_number_e uart, char *input_byte, uint32_t timeout) {
  if (uart == UART_3)
    return xQueueReceive(your_uart_rx_queueU3, input_byte, timeout);
  else if (uart == UART_2)
    return xQueueReceive(your_uart_rx_queueU2, input_byte, timeout);
  else
    fprintf(stderr, "INVALID UART\n");
}
