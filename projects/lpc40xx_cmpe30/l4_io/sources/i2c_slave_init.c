#include "i2c_slave_init.h"
#include "i2c.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "common_macros.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
void i2c_Slave_initialize(i2c_e i2c_number, uint32_t desired_i2c_bus_speed_in_hz, uint32_t peripheral_clock_hz) {
  const function__void_f isrs[] = {i2c0_isr, i2c1_isr, i2c2_isr};
  const lpc_peripheral_e peripheral_ids[] = {LPC_PERIPHERAL__I2C0, LPC_PERIPHERAL__I2C1, LPC_PERIPHERAL__I2C2};

  // Make sure our i2c structs size matches our map of ISRs and I2C peripheral IDs
  COMPILE_TIME_ASSERT(ARRAY_SIZE(i2c_structs) == ARRAY_SIZE(isrs));
  COMPILE_TIME_ASSERT(ARRAY_SIZE(i2c_structs) == ARRAY_SIZE(peripheral_ids));

  i2c_s *i2c = &i2c_structs[i2c_number];
  LPC_I2C_TypeDef *lpc_i2c = i2c->registers;
  const lpc_peripheral_e peripheral_id = peripheral_ids[i2c_number];

  // Create binary semaphore and mutex. We deliberately use non static memory
  // allocation because we do not want to statically define memory for all I2C buses
  i2c->transfer_complete_signal = xSemaphoreCreateBinary();
  i2c->mutex = xSemaphoreCreateMutex();
  vTraceSetMutexName(i2c->mutex, "i2c_Slave_mutex");

  // Optional: Provide names of the FreeRTOS objects for the Trace Facility
  // vTraceSetMutexName(mI2CMutex, "I2C Mutex");
  // vTraceSetSemaphoreName(mTransferCompleteSignal, "I2C Finish Sem");

  lpc_peripheral__turn_on_power_to(peripheral_id);
  lpc_i2c->CONCLR = 0x6C; // Clear ALL I2C Flags

  /**
   * Per I2C high speed mode:
   * HS mode master devices generate a serial clock signal with a HIGH to LOW ratio of 1 to 2.
   * So to be able to optimize speed, we use different duty cycle for high/low
   *
   * Compute the I2C clock dividers.
   * The LOW period can be longer than the HIGH period because the rise time
   * of SDA/SCL is an RC curve, whereas the fall time is a sharper curve.
   */
  const uint32_t percent_high = 40;
  const uint32_t percent_low = (100 - percent_high);
  const uint32_t max_speed_hz = UINT32_C(1) * 1000 * 1000;
  const uint32_t ideal_speed_hz = UINT32_C(100) * 1000;
  const uint32_t freq_hz = (desired_i2c_bus_speed_in_hz > max_speed_hz) ? ideal_speed_hz : desired_i2c_bus_speed_in_hz;
  const uint32_t half_clock_divider = (peripheral_clock_hz / freq_hz) / 2;

  // Each clock phase contributes to 50%
  lpc_i2c->SCLH = ((half_clock_divider * percent_high) / 100) / 2;
  lpc_i2c->SCLL = ((half_clock_divider * percent_low) / 100) / 2;

  // Set I2C slave address to zeroes and enable I2C
  /* lpc_i2c->ADR0 =  */lpc_i2c->ADR1 = /* lpc_i2c->ADR2 = lpc_i2c->ADR3 = */ 0x94; // Even addresses only. Write = 0. read = 1

  // Enable I2C and the interrupt for it
  lpc_i2c->CONSET = 0x44; // 0x40 Enables Master Mode //0x44 enables slave receiver
  lpc_peripheral__enable_interrupt(peripheral_id, isrs[i2c_number], i2c_structs[i2c_number].rtos_isr_trace_name);
}