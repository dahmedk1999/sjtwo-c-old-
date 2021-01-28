// file gpio_lab.h
#pragma once

#include <stdbool.h>
#include <stdint.h>

// include this file at gpio_lab.c file
// #include "lpc40xx.h"

// NOTE: The IOCON is not part of this driver

/// Should alter the hardware registers to set the pin as input
void gpioX__set_as_input(uint8_t port, uint8_t pin_num);

/// Should alter the hardware registers to set the pin as output
void gpioX__set_as_output(uint8_t port, uint8_t pin_num);

/// Should alter the hardware registers to set the pin as high
void gpioX__set_high(uint8_t port, uint8_t pin_num);

/// Should alter the hardware registers to set the pin as low
void gpioX__set_low(uint8_t port, uint8_t pin_num);

/**
 * Should alter the hardware registers to set the pin as low
 *
 * @param {bool} high - true => set pin high, false => set pin low
 */
void gpioX__set(uint8_t port, uint8_t pin_num, bool high); // Use with get level to toggle

/**
 * Should return the state of the pin (input or output, doesn't matter)
 *
 * @return {bool} level of pin high => true, low => false
 */
bool gpioX__get_level(uint8_t port, uint8_t pin_num);

void gpioX__toggle(uint8_t port, uint8_t pin_num);