#include "i2c_slave_init.h"

void i2c2__slave_init(uint8_t sel_slave_address) {
  LPC_I2C1->ADR1 |= (sel_slave_address >> 0);
  LPC_I2C1->CONSET = 0x44;
}

bool i2c_slave_callback__read_memory(uint8_t memory_index, uint8_t *memory) {
  if (memory_index < 256) {
    *memory = slave_memory[memory_index];
    return 1;
  } else {
    perror("mem_index_out_of_range");
    return 0;
  }
}

bool i2c_slave_callback__write_memory(uint8_t memory_index, uint8_t memory_value) {
  if (memory_index < 256) {
    slave_memory[memory_index] = memory_value;
    return 1;
  } else {
    perror("mem_index_out_of_range");
    return 0;
  }
}
