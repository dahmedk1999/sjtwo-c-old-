#include "gpio_lab.h"
#include "lpc40xx.h"
//#include <stdio.h>
// Port 0
void gpioX__set_as_input(uint8_t port, uint8_t pin_num) {
  switch (port) {
  case 0:
    LPC_GPIO0->DIR &= ~(1 << pin_num);
    break;
  case 1:
    LPC_GPIO1->DIR &= ~(1 << pin_num);
    break;
  case 2:
    LPC_GPIO2->DIR &= ~(1 << pin_num);
    break;
  case 3:
    LPC_GPIO3->DIR &= ~(1 << pin_num);
    break;
  case 4:
    LPC_GPIO4->DIR &= ~(1 << pin_num);
    break;
  default:
    break;
  }
}

void gpioX__set_as_output(uint8_t port, uint8_t pin_num) {
  switch (port) {
  case 0:
    LPC_GPIO0->DIR |= (1 << pin_num);
    break;
  case 1:
    LPC_GPIO1->DIR |= (1 << pin_num);
    break;
  case 2:
    LPC_GPIO2->DIR |= (1 << pin_num);
    break;
  case 3:
    LPC_GPIO3->DIR |= (1 << pin_num);
    break;
  case 4:
    LPC_GPIO4->DIR |= (1 << pin_num);
    break;
  default:
    break;
  }
}

void gpioX__set_high(uint8_t port, uint8_t pin_num) {
  switch (port) {
  case 0:
    LPC_GPIO0->PIN |= (1 << pin_num);
    break;
  case 1:
    LPC_GPIO1->PIN |= (1 << pin_num);
    break;
  case 2:
    LPC_GPIO2->PIN |= (1 << pin_num);
    break;
  case 3:
    LPC_GPIO3->PIN |= (1 << pin_num);
    break;
  case 4:
    LPC_GPIO4->PIN |= (1 << pin_num);
    break;
  default:
    break;
  }
}

void gpioX__set_low(uint8_t port, uint8_t pin_num) {
  switch (port) {
  case 0:
    LPC_GPIO0->PIN &= ~(1 << pin_num);
    break;
  case 1:
    LPC_GPIO1->PIN &= ~(1 << pin_num);
    break;
  case 2:
    LPC_GPIO2->PIN &= ~(1 << pin_num);
    break;
  case 3:
    LPC_GPIO3->PIN &= ~(1 << pin_num);
    break;
  case 4:
    LPC_GPIO4->PIN &= ~(1 << pin_num);
    break;
  default:
    break;
  }
}

void gpioX__set(uint8_t port, uint8_t pin_num, bool high) {

  switch (port) {
  case 0: {
    if (high)
      LPC_GPIO0->SET |= (1 << pin_num);
    else
      LPC_GPIO0->CLR |= (1 << pin_num);
  } break;
  case 1: {
    if (high)
      LPC_GPIO1->SET |= (1 << pin_num);
    else
      LPC_GPIO1->CLR |= (1 << pin_num);
  } break;
  case 2: {
    if (high)
      LPC_GPIO2->SET |= (1 << pin_num);
    else
      LPC_GPIO2->CLR |= (1 << pin_num);
  } break;
  case 3: {
    if (high)
      LPC_GPIO3->SET |= (1 << pin_num);
    else
      LPC_GPIO3->CLR |= (1 << pin_num);
  } break;
  case 4: {
    if (high)
      LPC_GPIO4->SET |= (1 << pin_num);
    else
      LPC_GPIO4->CLR |= (1 << pin_num);
  } break;
  default:
    break;
  }
}

bool gpioX__get_level(uint8_t port, uint8_t pin_num) {
  switch (port) {
  case 0: {
    // fprintf(stderr, "GPIO%d Pin %d High\n", port, pin_num);
    return (LPC_GPIO0->PIN & (1 << pin_num));
  } break;
  case 1: {
    return (LPC_GPIO1->PIN & (1 << pin_num));
  } break;
  case 2: {
    return (LPC_GPIO2->PIN & (1 << pin_num));
  } break;
  case 3: {
    return (LPC_GPIO3->PIN & (1 << pin_num));
  } break;
  case 4: {
    return (LPC_GPIO4->PIN & (1 << pin_num));
  } break;
  default:
    break;
  }
}

void gpioX__toggle(uint8_t port, uint8_t pin_num) {
  switch (port) {
  case 0: {
    LPC_GPIO0->PIN ^= (1 << pin_num);
  } break;
  case 1: {
    LPC_GPIO1->PIN ^= (1 << pin_num);
  } break;
  case 2: {
    LPC_GPIO2->PIN ^= (1 << pin_num);
  } break;
  case 3: {
    LPC_GPIO3->PIN ^= (1 << pin_num);
  } break;
  case 4: {
    LPC_GPIO4->PIN ^= (1 << pin_num);
  } break;
  default:
    break;
  }
}