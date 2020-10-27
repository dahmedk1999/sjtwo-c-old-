#pragma once

#include <stdbool.h>
#include <stdint.h>
typedef enum {
  I2C__0,
  I2C__1,
  I2C__2,
} i2c_e;

void i2c_Slave_initialize(i2c_e i2c_number, uint32_t desired_i2c_bus_speed_in_hz, uint32_t peripheral_clock_hz);