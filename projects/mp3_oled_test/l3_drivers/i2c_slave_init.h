#pragma once

#include "lpc40xx.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint8_t slave_memory[256];

void i2c2__slave_init(uint8_t sel_slave_address);

bool i2c_slave_callback__read_memory(uint8_t memory_index, uint8_t *memory);

bool i2c_slave_callback__write_memory(uint8_t memory_index, uint8_t memory_value);