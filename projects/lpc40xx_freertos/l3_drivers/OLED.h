#pragma once
#include <stdint.h>
#include <stdlib.h>
// 4 wire spi control
// Pins D0, D1 connect to CLK and MOSI, respectively
// Pins /CS, D/C connect to P1.22 and P1.25, respectively

/* Function     E(RD#)  R/W#(WR#) CS# D/C# D0
Write command   LOW     LOW       L   L    ↑
Write data      LOW     LOW       L   H    ↑
*/
unsigned int bitmap[8][128]; // 8 pages(8 rows/8bytes), 128 bits(128 columns/16bytes)
void spi1__init();
uint8_t ssp1__exch_byte(uint8_t data_out);
void cs_OLED();
void ds_OLED();

void power_up();
void power_down();