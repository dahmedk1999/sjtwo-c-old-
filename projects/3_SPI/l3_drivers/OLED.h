#pragma once
#include <stdint.h>
#include <stdlib.h>

typedef enum { SCROLL_LEFT = 0x26, SCROLL_RIGHT = 0x27 } SCROLLDIR;
typedef enum { page0, page1, page2, page3, page4, page5, page6, page7 } PAGES;
// 4 wire spi control
// Pins D0, D1 connect to CLK and MOSI, respectively
// Pins /CS, D/C connect to P1.22 and P1.25, respectively

/* Function     E(RD#)  R/W#(WR#) CS# D/C# D0
Write command   LOW     LOW       L   L    ↑
Write data      LOW     LOW       L   H    ↑
*/
unsigned int bitmap[8][128]; // 8 pages(8 rows/8bytes), 128 bits(128 columns/16bytes)
typedef void (*function_pointer_OLED)(void);
function_pointer_OLED callback_OLED[256]; // Call the character callbacks by using ascii values for indexing
void spi1__init();
uint8_t ssp1__exch_byte(uint8_t data_out);
void cs_OLED();
void ds_OLED();

void command_mode();
void data_mode();

void horizontal_addressing();
void vertical_addressing();
void horizontal_scrolling(SCROLLDIR DIRECTION, PAGES pageStart, PAGES pageEnd);

void power_up();
void power_down();
void update_OLED();
/* Clear screen */
void clear_OLED();
/* Fill screen */
void fill_OLED();
/*Experimental, Should randomize pixels*/
void noise_OLED();
/* Updates the OLED with what is on the bitmap */
void update_OLED();

/* User functions */
/* Use Function pointer Lookup table to display each character on OLED  */
void configure_OLED();
void OLED_Start();
void print_OLED(char *toprint);