// @file gpio_isr.h
#pragma once
#include "LPC40xx.h"
#include "lpc_peripherals.h"
#include <stdint.h>
typedef enum {
  GPIO_INTR__FALLING_EDGE,
  GPIO_INTR__RISING_EDGE,
} gpio_interrupt_e;

// Function pointer type (demonstrated later in the code sample)
typedef void (*function_pointer_t)(void);

// Allow the user to attach their callbacks
void gpioX__attach_interrupt(uint32_t port, uint32_t pin, gpio_interrupt_e interrupt_type,
                             function_pointer_t callback); // 0 is falling 1 is rising

// Our main() should configure interrupts to invoke this dispatcher where we will invoke user attached callbacks
// You can hijack 'interrupt_vector_table.c' or use API at lpc_peripherals.h
void gpioX__interrupt_dispatcher(void); // {}
