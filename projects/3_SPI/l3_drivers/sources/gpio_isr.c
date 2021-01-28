// @file gpio_isr.c
// clang-format off
#include "gpio_isr.h"
int detect_rise(uint8_t port) {
  if (port == 0) {
    for (int i = 0; i <= 31; i++) {
      if (LPC_GPIOINT->IO0IntStatR & (1 << i))
        return i;
    }
    return -1;
  } else if (port == 2) {
    for (int i = 0; i <= 31; i++) {
      if (LPC_GPIOINT->IO2IntStatR & (1 << i))
        return i;
    }
    return -1;
  } else
    return -1;
}
int detect_fall(uint8_t port) {
  if (port == 0) {
    for (int i = 0; i <= 31; i++) {
      if (LPC_GPIOINT->IO0IntStatF & (1 << i))
        return i;
    }
    return -1;
  } else if (port == 2) {
    for (int i = 0; i <= 31; i++) {
      if (LPC_GPIOINT->IO2IntStatF & (1 << i))
        return i;
    }
    return -1;
  } else
    return -1;
}
void clear_pin_interrupts(int port, int pin) {
  switch (port) {
  case 0:
    LPC_GPIOINT->IO0IntClr |= (1 << pin);
    break;
  case 2:
    LPC_GPIOINT->IO2IntClr |= (1 << pin);
  default:
    break;
  }
}

// Note: You may want another separate array for falling vs. rising edge callbacks
static function_pointer_t gpio0_callbackF[32] /*=  {
  callback, callback, callback, callback, callback, callback, callback, callback,             //00  01  02  03  04  05  06  07

    callback, callback, callback, callback, callback, callback, callback, callback,           //08   09  10  11  12  13  14  15

    callback, callback, callback, callback, callback, callback, callback, callback,           //16  17  18  19  20  21  22  23

    callback, callback, callback, callback, callback, callback, callback, callback            //24  25  26  27  28  29  30  31
}*/;
static function_pointer_t gpio0_callbackR[32];

static function_pointer_t gpio2_callbackF[32];
static function_pointer_t gpio2_callbackR[32];
//clang-format on
void gpioX__attach_interrupt(uint32_t port, uint32_t pin, gpio_interrupt_e interrupt_type,
                             function_pointer_t callback) {
  // 1) Store the callback based on the pin at gpio0_callbackF/R
  // 2) Configure GPIO 0/2 pin for rising or falling edge
NVIC_EnableIRQ(GPIO_IRQn);

switch (port)
{
case 0:{
  if(interrupt_type)//1 is rising
  {
    LPC_GPIOINT->IO0IntEnR |= (1<<pin);gpio0_callbackR[pin]=callback;
  }else
  {
    LPC_GPIOINT->IO0IntEnF |= (1<<pin);gpio0_callbackF[pin]=callback;
  }
}
  break;
case 2:{
  if(interrupt_type)
  {
    LPC_GPIOINT->IO2IntEnR |= (1<<pin);gpio2_callbackR[pin]=callback;
  }else
  {
    LPC_GPIOINT->IO2IntEnF |= (1<<pin);gpio2_callbackF[pin]=callback;
  }}
  break;
default:
  break;
}

}
void gpioX__interrupt_dispatcher(void)
{
  const int p0_Rising_intr_pin = detect_rise(0);
  const int p0_Falling_intr_pin = detect_fall(0);
  const int p2_Rising_intr_pin = detect_rise(2);
  const int p2_Falling_intr_pin = detect_fall(2);
  //Port 0 Interrupt handling
  if((p0_Rising_intr_pin == p0_Falling_intr_pin) && (p0_Falling_intr_pin != -1 && p0_Rising_intr_pin !=-1))//if the same pin has both falling and rising edge interrupts, delay clearing of them
  {
    function_pointer_t attached_user_handler0F = gpio0_callbackF[p0_Falling_intr_pin];
    function_pointer_t attached_user_handler0R = gpio0_callbackR[p0_Rising_intr_pin];
    attached_user_handler0F();
    attached_user_handler0R();
    clear_pin_interrupts(0,p0_Rising_intr_pin);
  }
  else
  {
    if(p0_Falling_intr_pin !=-1)
    {
      function_pointer_t attached_user_handler0F = gpio0_callbackF[p0_Falling_intr_pin];
      attached_user_handler0F();
      clear_pin_interrupts(0,p0_Falling_intr_pin);
    }
    if(p0_Rising_intr_pin != -1)
    {
      function_pointer_t attached_user_handler0R = gpio0_callbackR[p0_Rising_intr_pin];
      attached_user_handler0R();
      clear_pin_interrupts(0,p0_Rising_intr_pin);
    }
  }

  //Port 2 interrupt handling
  if((p2_Falling_intr_pin==p2_Rising_intr_pin ) && (p2_Falling_intr_pin != -1 && p2_Rising_intr_pin !=-1))
  {
    function_pointer_t attached_user_handler2F = gpio2_callbackF[p2_Falling_intr_pin];
    function_pointer_t attached_user_handler2R = gpio2_callbackR[p2_Rising_intr_pin];
    attached_user_handler2F();
    attached_user_handler2R();
    clear_pin_interrupts(2,p0_Rising_intr_pin);
  }
  else{
  if(p2_Falling_intr_pin !=-1)
  {
    function_pointer_t attached_user_handler2F = gpio2_callbackF[p2_Falling_intr_pin];
    attached_user_handler2F();
    clear_pin_interrupts(2,p0_Falling_intr_pin);
  }
  if(p2_Rising_intr_pin != -1)
  {
    function_pointer_t attached_user_handler2R = gpio2_callbackR[p2_Rising_intr_pin];
    attached_user_handler2R();
    clear_pin_interrupts(2,p0_Rising_intr_pin);
  }
  }
}
