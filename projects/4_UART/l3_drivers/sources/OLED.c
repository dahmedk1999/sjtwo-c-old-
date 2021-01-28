// clang-format off
#include "OLED.h"
#include "clock.h"
#include "gpio.h"
#include "gpio_lab.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdio.h>
#include <string.h>

bool ON__FLAG=false;
uint8_t cursor=0;
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
  ssp1__exch_byte(0x20);ssp1__exch_byte(0x00);//Sets to horizontal mode
  ssp1__exch_byte(0x21);ssp1__exch_byte(0x00);ssp1__exch_byte(0x7F);//column address start0-127end
  ssp1__exch_byte(0x22);ssp1__exch_byte(0x00);ssp1__exch_byte(0x07);//page address start0-7end 
}
void vertical_addressing()
{
  command_mode();
  ssp1__exch_byte(0x20);ssp1__exch_byte(0x01);//set to vert
  ssp1__exch_byte(0x21);ssp1__exch_byte(0x00);ssp1__exch_byte(0x7F);//Set column mode
  ssp1__exch_byte(0x22);ssp1__exch_byte(0x00);ssp1__exch_byte(0x07);//Set page address
}

void horizontal_scrolling(SCROLLDIR DIRECTION,PAGES pageStart,PAGES pageEnd) {
  cs_OLED();
    command_mode();
    ssp1__exch_byte(DIRECTION);
    ssp1__exch_byte(0x00); // dummy byte
    ssp1__exch_byte(pageStart); // start Page 0
    ssp1__exch_byte(0x07); // 5 frames
    ssp1__exch_byte(pageEnd); // end Page 7
    ssp1__exch_byte(0x00); // dummy byte 00
    ssp1__exch_byte(0xFF); // dummy byte FF
    ssp1__exch_byte(0x2F); // activate scrolling
  ds_OLED();
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
  if(strlen(toprint)==0 || toprint=='\0'){}
  else
  {
  cs_OLED();
  data_mode();
  if(cursor>=128)cursor=0;
    for (int i = 0; i < strlen(toprint); i++) 
    {
      cursor++;//every character increments cursor. 16 chars per page
      if(cursor>=128)cursor=0;
      fprintf(stderr,"\nCursor at start of for loop is: %d",cursor);
      
      
      if(toprint[i]=='\n')//if new line, increment cursor count until it is characters/page
      {
        cursor=(cursor+((16-(cursor%16))));
        if(cursor>=128)cursor=0;
        fprintf(stderr,"\nCursor at \\n is: character %d, so char_return is getting %d\n",cursor,(uint8_t)(cursor/16));
        char_return((uint8_t)(cursor/16));
        continue;
      }
      function_pointer_OLED handler_OLED = callback_OLED[(int)(toprint[i])];
      handler_OLED();
    }
  ds_OLED();
  }
}

/* ----------------Lookup Table---------------- */
/* Bang's Contribution */
//
void char_A() {
  ssp1__exch_byte(0x7E);
  ssp1__exch_byte(0x09);
  ssp1__exch_byte(0x09);
  ssp1__exch_byte(0x09);
  ssp1__exch_byte(0x7E);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_B() {
  ssp1__exch_byte(0x7F);
  ssp1__exch_byte(0x49);
  ssp1__exch_byte(0x49);
  ssp1__exch_byte(0x49);
  ssp1__exch_byte(0x36);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_C() {
  ssp1__exch_byte(0x3E);
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x22);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_D() {
  ssp1__exch_byte(0x7F);
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x3E);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_E() {
  ssp1__exch_byte(0x7F);
  ssp1__exch_byte(0x49);
  ssp1__exch_byte(0x49);
  ssp1__exch_byte(0x49);
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_F() {
  ssp1__exch_byte(0x7F);
  ssp1__exch_byte(0x09);
  ssp1__exch_byte(0x09);
  ssp1__exch_byte(0x09);
  ssp1__exch_byte(0x01);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_G() {
  ssp1__exch_byte(0x3E);
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x49);
  ssp1__exch_byte(0x49);
  ssp1__exch_byte(0x3A);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_H() {
  ssp1__exch_byte(0x7F);
  ssp1__exch_byte(0x08);
  ssp1__exch_byte(0x08);
  ssp1__exch_byte(0x08);
  ssp1__exch_byte(0x7F);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_I() {
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x7F);
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_J() {
  ssp1__exch_byte(0x20);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x3F);
  ssp1__exch_byte(0x01);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_K() {
  ssp1__exch_byte(0x7f);
  ssp1__exch_byte(0x08);
  ssp1__exch_byte(0x14);
  ssp1__exch_byte(0x22);
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_L() {
  ssp1__exch_byte(0x7f);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_M() {
  ssp1__exch_byte(0x7F);
  ssp1__exch_byte(0x02);
  ssp1__exch_byte(0x0C);
  ssp1__exch_byte(0x02);
  ssp1__exch_byte(0x7F);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_N() {
  ssp1__exch_byte(0x7f);
  ssp1__exch_byte(0x02);
  ssp1__exch_byte(0x04);
  ssp1__exch_byte(0x08);
  ssp1__exch_byte(0x7f);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_O() {
  ssp1__exch_byte(0x3e);
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x3e);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_P() {
  ssp1__exch_byte(0x7F);
  ssp1__exch_byte(0x09);
  ssp1__exch_byte(0x09);
  ssp1__exch_byte(0x09);
  ssp1__exch_byte(0x06);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_Q() {
  ssp1__exch_byte(0x3E);
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x51);
  ssp1__exch_byte(0x21);
  ssp1__exch_byte(0x5E);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_R() {
  ssp1__exch_byte(0x7F);
  ssp1__exch_byte(0x09);
  ssp1__exch_byte(0x19);
  ssp1__exch_byte(0x29);
  ssp1__exch_byte(0x46);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_S() {
  ssp1__exch_byte(0x26);
  ssp1__exch_byte(0x49);
  ssp1__exch_byte(0x49);
  ssp1__exch_byte(0x49);
  ssp1__exch_byte(0x32);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_T() {
  ssp1__exch_byte(0x01);
  ssp1__exch_byte(0x01);
  ssp1__exch_byte(0x7F);
  ssp1__exch_byte(0x01);
  ssp1__exch_byte(0x01);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_U() {
  ssp1__exch_byte(0x3F);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x3F);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_V() {
  ssp1__exch_byte(0x1F);
  ssp1__exch_byte(0x20);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x20);
  ssp1__exch_byte(0x1F);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_W() {
  ssp1__exch_byte(0x3F);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x38);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x3F);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_X() {
  ssp1__exch_byte(0x63);
  ssp1__exch_byte(0x14);
  ssp1__exch_byte(0x08);
  ssp1__exch_byte(0x14);
  ssp1__exch_byte(0x63);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_Y() {
  ssp1__exch_byte(0x07);
  ssp1__exch_byte(0x08);
  ssp1__exch_byte(0x70);
  ssp1__exch_byte(0x08);
  ssp1__exch_byte(0x07);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_Z() {
  ssp1__exch_byte(0x61);
  ssp1__exch_byte(0x51);
  ssp1__exch_byte(0x49);
  ssp1__exch_byte(0x45);
  ssp1__exch_byte(0x43);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_a() {
  ssp1__exch_byte(0x20);
  ssp1__exch_byte(0x54);
  ssp1__exch_byte(0x54);
  ssp1__exch_byte(0x54);
  ssp1__exch_byte(0x78);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_b() {
  ssp1__exch_byte(0x7F);
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x38);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_c() {
  ssp1__exch_byte(0x38);
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_d() {
  ssp1__exch_byte(0x38);
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x7F);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_e() {
  ssp1__exch_byte(0x38);
  ssp1__exch_byte(0x54);
  ssp1__exch_byte(0x54);
  ssp1__exch_byte(0x54);
  ssp1__exch_byte(0x18);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_f() {
  ssp1__exch_byte(0x08);
  ssp1__exch_byte(0x7E);
  ssp1__exch_byte(0x09);
  ssp1__exch_byte(0x01);
  ssp1__exch_byte(0x02);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_g() {
  ssp1__exch_byte(0x18);
  ssp1__exch_byte(0xA4);
  ssp1__exch_byte(0xA4);
  ssp1__exch_byte(0xA4);
  ssp1__exch_byte(0x7C);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_h() {
  ssp1__exch_byte(0x7F);
  ssp1__exch_byte(0x08);
  ssp1__exch_byte(0x04);
  ssp1__exch_byte(0x04);
  ssp1__exch_byte(0x78);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_i() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x7D);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_j() {
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x80);
  ssp1__exch_byte(0x80);
  ssp1__exch_byte(0x84);
  ssp1__exch_byte(0x7D);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_k() {
  ssp1__exch_byte(0x7F);
  ssp1__exch_byte(0x10);
  ssp1__exch_byte(0x28);
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_l() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x41);
  ssp1__exch_byte(0x7F);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_m() {
  ssp1__exch_byte(0x7C);
  ssp1__exch_byte(0x04);
  ssp1__exch_byte(0x18);
  ssp1__exch_byte(0x04);
  ssp1__exch_byte(0x78);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_n() {
  ssp1__exch_byte(0x7C);
  ssp1__exch_byte(0x08);
  ssp1__exch_byte(0x04);
  ssp1__exch_byte(0x04);
  ssp1__exch_byte(0x78);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_o() {
  ssp1__exch_byte(0x38);
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x38);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_p() {
  ssp1__exch_byte(0xFC);
  ssp1__exch_byte(0x24);
  ssp1__exch_byte(0x24);
  ssp1__exch_byte(0x24);
  ssp1__exch_byte(0x18);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_q() {
  ssp1__exch_byte(0x18);
  ssp1__exch_byte(0x24);
  ssp1__exch_byte(0x24);
  ssp1__exch_byte(0x28);
  ssp1__exch_byte(0xFC);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_r() {
  ssp1__exch_byte(0x7C);
  ssp1__exch_byte(0x08);
  ssp1__exch_byte(0x04);
  ssp1__exch_byte(0x04);
  ssp1__exch_byte(0x08);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_s() {
  ssp1__exch_byte(0x48);
  ssp1__exch_byte(0x54);
  ssp1__exch_byte(0x54);
  ssp1__exch_byte(0x54);
  ssp1__exch_byte(0x20);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_t() {
  ssp1__exch_byte(0x04);
  ssp1__exch_byte(0x3E);
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x20);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_u() {
  ssp1__exch_byte(0x3C);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x20);
  ssp1__exch_byte(0x7C);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_v() {
  ssp1__exch_byte(0x1C);
  ssp1__exch_byte(0x20);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x20);
  ssp1__exch_byte(0x1C);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}

void char_w() {
  ssp1__exch_byte(0x3c);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x30);
  ssp1__exch_byte(0x40);
  ssp1__exch_byte(0x3C);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_x() {
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x28);
  ssp1__exch_byte(0x10);
  ssp1__exch_byte(0x28);
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_y() {
  ssp1__exch_byte(0x9C);
  ssp1__exch_byte(0xA0);
  ssp1__exch_byte(0xA0);
  ssp1__exch_byte(0xA0);
  ssp1__exch_byte(0x7C);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_z() {
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x64);
  ssp1__exch_byte(0x54);
  ssp1__exch_byte(0x4C);
  ssp1__exch_byte(0x44);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
/* Danish's */
//
void char_0() {
  ssp1__exch_byte(0b00111110);
  ssp1__exch_byte(0b01010001);
  ssp1__exch_byte(0b01001001);
  ssp1__exch_byte(0b01000101);
  ssp1__exch_byte(0b00111110);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_1() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0b01000010);
  ssp1__exch_byte(0b01111111);
  ssp1__exch_byte(0b01000000);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_2() {
  ssp1__exch_byte(0b01000010);
  ssp1__exch_byte(0b01100001);
  ssp1__exch_byte(0b01010001);
  ssp1__exch_byte(0b01001001);
  ssp1__exch_byte(0b01000110);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_3() {
  ssp1__exch_byte(0b00100010);
  ssp1__exch_byte(0b01000001);
  ssp1__exch_byte(0b01001001);
  ssp1__exch_byte(0b01001001);
  ssp1__exch_byte(0b00110110);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_4() {
  ssp1__exch_byte(0b00011000);
  ssp1__exch_byte(0b00010100);
  ssp1__exch_byte(0b00010010);
  ssp1__exch_byte(0b01111111);
  ssp1__exch_byte(0b00010000);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_5() {
  ssp1__exch_byte(0b00100111);
  ssp1__exch_byte(0b01000101);
  ssp1__exch_byte(0b01000101);
  ssp1__exch_byte(0b01000101);
  ssp1__exch_byte(0b00111001);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_6() {
  ssp1__exch_byte(0b00111100);
  ssp1__exch_byte(0b01001010);
  ssp1__exch_byte(0b01001001);
  ssp1__exch_byte(0b01001001);
  ssp1__exch_byte(0b00110000);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_7() {
  ssp1__exch_byte(0b00000001);
  ssp1__exch_byte(0b01110001);
  ssp1__exch_byte(0b00001001);
  ssp1__exch_byte(0b00000101);
  ssp1__exch_byte(0b00000011);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_8() {
  ssp1__exch_byte(0b00110110);
  ssp1__exch_byte(0b01001001);
  ssp1__exch_byte(0b01001001);
  ssp1__exch_byte(0b01001001);
  ssp1__exch_byte(0b00110110);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_9() {
  ssp1__exch_byte(0b00000110);
  ssp1__exch_byte(0b01001001);
  ssp1__exch_byte(0b01001001);
  ssp1__exch_byte(0b00101001);
  ssp1__exch_byte(0b00011110);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_dquote() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0b00000111);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0b00000111);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_squote() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0b00000101);
  ssp1__exch_byte(0b00000011);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_comma() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0b10100000);
  ssp1__exch_byte(0b01100000);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_qmark() {
  ssp1__exch_byte(0b00000010);
  ssp1__exch_byte(0b00000001);
  ssp1__exch_byte(0b01010001);
  ssp1__exch_byte(0b00001001);
  ssp1__exch_byte(0b00000110);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_excl() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0b01011111);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_at() {
  ssp1__exch_byte(0b00110010);
  ssp1__exch_byte(0b01001001);
  ssp1__exch_byte(0b01111001);
  ssp1__exch_byte(0b01000001);
  ssp1__exch_byte(0b00111110);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_undersc() {
  ssp1__exch_byte(0b10000000);
  ssp1__exch_byte(0b10000000);
  ssp1__exch_byte(0b10000000);
  ssp1__exch_byte(0b10000000);
  ssp1__exch_byte(0b10000000);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_star() {
  ssp1__exch_byte(0b00010100);
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0b00111110);
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0b00010100);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_hash() {
  ssp1__exch_byte(0b00010100);
  ssp1__exch_byte(0b01111111);
  ssp1__exch_byte(0b00010100);
  ssp1__exch_byte(0b01111111);
  ssp1__exch_byte(0b00010100);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_percent() {
  ssp1__exch_byte(0b00100011);
  ssp1__exch_byte(0b00010011);
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0b01100100);
  ssp1__exch_byte(0b01100010);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_amper() {
  ssp1__exch_byte(0b00110110);
  ssp1__exch_byte(0b01001001);
  ssp1__exch_byte(0b01010101);
  ssp1__exch_byte(0b00100010);
  ssp1__exch_byte(0b01010000);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_parenthL() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0b00011100);
  ssp1__exch_byte(0b00100010);
  ssp1__exch_byte(0b01000001);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_parenthR() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0b01000001);
  ssp1__exch_byte(0b00100010);
  ssp1__exch_byte(0b00011100);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_plus() {
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0b00111110);
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_minus() {
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_div() {
  ssp1__exch_byte(0b00100000);
  ssp1__exch_byte(0b00010000);
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0b00000100);
  ssp1__exch_byte(0b00000010);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_colon() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0b00110110);
  ssp1__exch_byte(0b00110110);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_scolon() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0b01010110);
  ssp1__exch_byte(0b00110110);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_less() {
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0b00010100);
  ssp1__exch_byte(0b00100010);
  ssp1__exch_byte(0b01000001);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_greater() {
  ssp1__exch_byte(0b10000010);
  ssp1__exch_byte(0b01000100);
  ssp1__exch_byte(0b00101000);
  ssp1__exch_byte(0b00010000);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_equal() {
  ssp1__exch_byte(0b00010100);
  ssp1__exch_byte(0b00010100);
  ssp1__exch_byte(0b00010100);
  ssp1__exch_byte(0b00010100);
  ssp1__exch_byte(0b00010100);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_bracketL() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0b01111111);
  ssp1__exch_byte(0b01000001);
  ssp1__exch_byte(0b01000001);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_backslash() {
  ssp1__exch_byte(0b00000010);
  ssp1__exch_byte(0b00000100);
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0b00010000);
  ssp1__exch_byte(0b00100000);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_bracketR() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0b01000001);
  ssp1__exch_byte(0b01000001);
  ssp1__exch_byte(0b01111111);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_caret() {
  ssp1__exch_byte(0b00000100);
  ssp1__exch_byte(0b00000010);
  ssp1__exch_byte(0b00000001);
  ssp1__exch_byte(0b00000010);
  ssp1__exch_byte(0b00000100);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_bquote() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0b00000001);
  ssp1__exch_byte(0b00000010);
  ssp1__exch_byte(0b00000100);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_braceL() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0b00110110);
  ssp1__exch_byte(0b01000001);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_braceR() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0b01000001);
  ssp1__exch_byte(0b00110110);
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_bar() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0b01111111);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_tilde() {
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0b00000100);
  ssp1__exch_byte(0b00000100);
  ssp1__exch_byte(0b00001000);
  ssp1__exch_byte(0b00000100);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_space() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_period() {
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0b01100000);
  ssp1__exch_byte(0b01100000);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_dollar() {
  ssp1__exch_byte(0b00100100);
  ssp1__exch_byte(0b00101010);
  ssp1__exch_byte(0b01101011);
  ssp1__exch_byte(0b00101010);
  ssp1__exch_byte(0b00010010);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
  ssp1__exch_byte(0x00);
}
void char_return(uint8_t addr) {
  command_mode();
  ssp1__exch_byte(0xB0 | addr);
  fprintf(stderr, "set column\n");
  ssp1__exch_byte(0x10);
  ssp1__exch_byte(0x00);
  data_mode();
}
/* ---------------End Lookup Table--------------*/
// clang-format on
void setup_lookuptable_OLED() {
  callback_OLED[(int)'A'] = char_A;
  callback_OLED[(int)'B'] = char_B;
  callback_OLED[(int)'C'] = char_C;
  callback_OLED[(int)'D'] = char_D;
  callback_OLED[(int)'E'] = char_E;
  callback_OLED[(int)'F'] = char_F;
  callback_OLED[(int)'G'] = char_G;
  callback_OLED[(int)'H'] = char_H;
  callback_OLED[(int)'I'] = char_I;
  callback_OLED[(int)'J'] = char_J;
  callback_OLED[(int)'K'] = char_K;
  callback_OLED[(int)'L'] = char_L;
  callback_OLED[(int)'M'] = char_M;
  callback_OLED[(int)'N'] = char_N;
  callback_OLED[(int)'O'] = char_O;
  callback_OLED[(int)'P'] = char_P;
  callback_OLED[(int)'P'] = char_P;
  callback_OLED[(int)'Q'] = char_Q;
  callback_OLED[(int)'R'] = char_R;
  callback_OLED[(int)'S'] = char_S;
  callback_OLED[(int)'T'] = char_T;
  callback_OLED[(int)'U'] = char_U;
  callback_OLED[(int)'V'] = char_V;
  callback_OLED[(int)'W'] = char_W;
  callback_OLED[(int)'X'] = char_X;
  callback_OLED[(int)'Y'] = char_Y;
  callback_OLED[(int)'Z'] = char_Z;
  callback_OLED[(int)'a'] = char_a;
  callback_OLED[(int)'b'] = char_b;
  callback_OLED[(int)'c'] = char_c;
  callback_OLED[(int)'d'] = char_d;
  callback_OLED[(int)'e'] = char_e;
  callback_OLED[(int)'f'] = char_f;
  callback_OLED[(int)'g'] = char_g;
  callback_OLED[(int)'h'] = char_h;
  callback_OLED[(int)'i'] = char_i;
  callback_OLED[(int)'j'] = char_j;
  callback_OLED[(int)'k'] = char_k;
  callback_OLED[(int)'l'] = char_l;
  callback_OLED[(int)'m'] = char_m;
  callback_OLED[(int)'n'] = char_n;
  callback_OLED[(int)'o'] = char_o;
  callback_OLED[(int)'p'] = char_p;
  callback_OLED[(int)'q'] = char_q;
  callback_OLED[(int)'r'] = char_r;
  callback_OLED[(int)'s'] = char_s;
  callback_OLED[(int)'t'] = char_t;
  callback_OLED[(int)'u'] = char_u;
  callback_OLED[(int)'v'] = char_v;
  callback_OLED[(int)'w'] = char_w;
  callback_OLED[(int)'x'] = char_x;
  callback_OLED[(int)'y'] = char_y;
  callback_OLED[(int)'z'] = char_z;

  callback_OLED[(int)'0'] = char_0;
  callback_OLED[(int)'1'] = char_1;
  callback_OLED[(int)'2'] = char_2;
  callback_OLED[(int)'3'] = char_3;
  callback_OLED[(int)'4'] = char_4;
  callback_OLED[(int)'5'] = char_5;
  callback_OLED[(int)'6'] = char_6;
  callback_OLED[(int)'7'] = char_7;
  callback_OLED[(int)'8'] = char_8;
  callback_OLED[(int)'9'] = char_9;

  callback_OLED[(int)'"'] = char_dquote;
  callback_OLED[(int)'\''] = char_squote;
  callback_OLED[(int)','] = char_comma;
  callback_OLED[(int)'?'] = char_qmark;
  callback_OLED[(int)'!'] = char_excl;
  callback_OLED[(int)'@'] = char_at;
  callback_OLED[(int)'_'] = char_undersc;
  callback_OLED[(int)'*'] = char_star;
  callback_OLED[(int)'#'] = char_hash;
  callback_OLED[(int)'%'] = char_percent;

  callback_OLED[(int)'&'] = char_amper;
  callback_OLED[(int)'('] = char_parenthL;
  callback_OLED[(int)')'] = char_parenthR;
  callback_OLED[(int)'+'] = char_plus;
  callback_OLED[(int)'-'] = char_minus;
  callback_OLED[(int)'/'] = char_div;
  callback_OLED[(int)':'] = char_colon;
  callback_OLED[(int)';'] = char_scolon;
  callback_OLED[(int)'<'] = char_less;
  callback_OLED[(int)'>'] = char_greater;

  callback_OLED[(int)'='] = char_equal;
  callback_OLED[(int)'['] = char_bracketL;
  callback_OLED[(int)'\\'] = char_backslash;
  callback_OLED[(int)']'] = char_bracketR;
  callback_OLED[(int)'^'] = char_caret;
  callback_OLED[(int)'`'] = char_bquote;
  callback_OLED[(int)'{'] = char_braceL;
  callback_OLED[(int)'}'] = char_braceR;
  callback_OLED[(int)'|'] = char_bar;
  callback_OLED[(int)'~'] = char_tilde;

  callback_OLED[(int)' '] = char_space;
  callback_OLED[(int)'.'] = char_period;
  callback_OLED[(int)'$'] = char_dollar;
  callback_OLED[(int)'\n'] = char_return;
}

void OLED_Start() {
  configure_OLED();
  spi1__init();

  clear_OLED();
  cs_OLED();

  power_up();
  setup_lookuptable_OLED();
  update_OLED();

  ds_OLED();
  ON__FLAG = true;
}
