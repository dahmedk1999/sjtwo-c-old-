#pragma once

#include "lpc40xx.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint8_t slave_memory[256];

// void i2c_Slave_initialize(i2c_e i2c_number, uint32_t desired_i2c_bus_speed_in_hz, uint32_t peripheral_clock_hz);
void i2c2__slave_init(uint8_t sel_slave_address);