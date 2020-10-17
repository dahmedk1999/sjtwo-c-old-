// clang-format off
#include "OLED.h"
#include "clock.h"
#include "gpio.h"
#include "gpio_lab.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdio.h>
#include <string.h>
// 4 wire spi control
// Pins D0, D1 connect to CLK(P0.7) and MOSI(P0.9), respectively
// Pins /CS, D/C connect to P1.22 and P1.25, respectively

/*
Function        E(RD#)  R/W#(WR#) CS# D/C# D0
Write command   LOW     LOW       L   L    ↑
Write data      LOW     LOW       L   H    ↑
*/
// clang-format off
void spi1__init() { // MINIMUM  CLOCK CYCLE == 100ns for OLED Display, 10MHz
  // Refer to LPC User manual and setup the register bits correctly
  uint32_t user_clock_mhz = 9;
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP1);
  // b) Setup control registers CR0 and CR1
  LPC_SSP1->CR0 |= 7;        // we are sending only 8 bits at a time
  LPC_SSP1->CR1 |= (1 << 1); // enable SSP
  // c) Setup prescalar register to be <= user_clock_mhz

  uint8_t div = 2;
  const uint32_t cpu_clock_mhz = clock__get_core_clock_hz();
  while ((user_clock_mhz * 1000 * 1000) < (cpu_clock_mhz / div) && div <= 254) {
    div += 2;
  }
  fprintf(stderr, "OLED_CLOCK Divider= %d %d/%d = %d, \n", div, cpu_clock_mhz, user_clock_mhz * 1000 * 1000, div);
  LPC_SSP1->CPSR = div; // 10MHz is OLED limit
}

uint8_t ssp1__exch_byte(uint8_t data_out) {
  LPC_SSP1->DR = data_out;
  while (LPC_SSP1->SR & (1 << 4)) {
    /* fprintf(stderr, "FIFO is busy...\n") */; // when idle, it returns
  }

  return (uint8_t)(LPC_SSP1->DR & 0xFF); // right justify the data bits
}

void cs_OLED() {
  gpioX__set_as_output(1, 22);
  gpioX__set_low(1, 22);
}
void ds_OLED() {
  gpioX__set_as_output(1, 22);
  gpioX__set_high(1, 22);
}
void configure_OLED()
{
  gpio__construct_with_function(0, 7, GPIO__FUNCTION_2);
  gpioX__set_as_output(0, 7);
  gpio__construct_with_function(0, 9, GPIO__FUNCTION_2);
  gpioX__set_as_output(0, 9);
  gpio__construct_with_function(1, 25, GPIO__FUNCITON_0_IO_PIN);
  gpioX__set_as_output(1, 25);
}
void command_mode()
{
gpioX__set_low(1,25);
}
void data_mode()
{
gpioX__set_high(1,25);
}

void horizontal_addressing()
{
  command_mode();
  ssp1__exch_byte(0x28);ssp1__exch_byte(0x00);//Sets to horizontal mode
  ssp1__exch_byte(0x21);ssp1__exch_byte(0x00);ssp1__exch_byte(0x7F);//column address start0-127end
  ssp1__exch_byte(0x22);ssp1__exch_byte(0x00);ssp1__exch_byte(0x07);//page address start0-7end 
}

// clang-format off

/* Using the datasheet, follow the initialization steps at page 19 */
void power_up()
{
command_mode();//Set to command mode by setting D/C pin low
ssp1__exch_byte(0xAE);//turn off panel
ssp1__exch_byte(0xD5);ssp1__exch_byte(0x80);//Display clock, Freq
ssp1__exch_byte(0xA8);ssp1__exch_byte(0x3F);//Multiplex Ratio
ssp1__exch_byte(0xD3);ssp1__exch_byte(0x00);//Display Offset
ssp1__exch_byte(0x40);//Display Start line
ssp1__exch_byte(0x8D);ssp1__exch_byte(0x14);//Set charge pump, internal DC/DC circuit
ssp1__exch_byte(0xA1);//segment remap
ssp1__exch_byte(0xC8);//Set COM output Scan direction
ssp1__exch_byte(0xDA);ssp1__exch_byte(0x12);//set COM output scan dir
ssp1__exch_byte(0x81);ssp1__exch_byte(0xCF);//Set contrast reg
ssp1__exch_byte(0xD9);ssp1__exch_byte(0xF1);//Precharge period
ssp1__exch_byte(0xDB);ssp1__exch_byte(0x40);//VCOMH Deselect level

horizontal_addressing();

ssp1__exch_byte(0xA4);//enable panel
ssp1__exch_byte(0xA6);//Normal colors, not inverted
ssp1__exch_byte(0xAF);//turn on panel
}

void power_down(){
command_mode();//Set to command mode by setting D/C pin low
ssp1__exch_byte(0xAE);//turn off panel
}

void clear_OLED() {
for(int i=0;i<8;i++)
  {
    for(int j=0;j<128;j++)
    {
      bitmap[i][j]=0x00;
    }
  }
// memset(bitmap, 0x00, sizeof(bitmap));
}
void fill_OLED() {
for(int i=0;i<8;i++)
  {
    for(int j=0;j<128;j++)
    {
      bitmap[i][j]=0xFF;
    }
  }
// memset(bitmap, 0xFF, sizeof(bitmap));
}
void noise_OLED() {
for(int i=0;i<8;i++)
  {
    for(int j=0;j<128;j++)
    {
      bitmap[i][j]=rand()%0x100;//00 to FF
    }
  }
}

void update_OLED() {
  horizontal_addressing();
  for (int i= 0; i < 8; i++) 
  {
    for (int j = 0;j < 128; j++) 
    {
      data_mode();
      ssp1__exch_byte(bitmap[i][j]);
    }
  }
}

void print_OLED(char *toprint){
  cs_OLED();
  data_mode();

    for (int i = 0; i < strlen(toprint); i++) 
    {
      function_pointer_OLED handler_OLED = callback_OLED[(int)(toprint[i])];
      handler_OLED();
    }
  ds_OLED();
}

/* Lookup Table */
void char_0() {
  ssp1__exch_byte(0b01111100);
  ssp1__exch_byte(0b10001010);
  ssp1__exch_byte(0b10010010);
  ssp1__exch_byte(0b10100010);
  ssp1__exch_byte(0b01111100);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
/* End Lookup Table */

void setup_lookuptable_OLED(){
  callback_OLED[(int)'0']=char_0;
}

void OLED_Start()
{
configure_OLED();
spi1__init();

clear_OLED();
cs_OLED();

power_up();
setup_lookuptable_OLED();
update_OLED();

ds_OLED();
}