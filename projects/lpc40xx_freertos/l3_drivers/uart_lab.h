#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
  UART_2,
  UART_3,
} uart_number_e;

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate);
bool uart_lab__polled_get(uart_number_e uart, char *input_byte);
bool uart_lab__polled_put(uart_number_e uart, char output_byte);