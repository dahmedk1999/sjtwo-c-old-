// clang-format on
#include "FreeRTOS.h"
#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "math.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>

#include "C:\sjtwo-c\projects\lpc40xx_freertos\l3_drivers\adc.h"
#include "LPC40xx.h"
#include "gpio.h"
#include "gpio_isr.h"
#include "gpio_lab.h"
#include "lpc_peripherals.h"
#include "pwm1.h"
#include "queue.h"
#include "semphr.h"
uint8_t GPIOPORTs[] = {1, 1, 1, 2};
uint8_t GPIOLEDs[] = {18, 24, 26, 3};

typedef struct {
  uint8_t port;
  uint8_t pin;
} port_pin_s;

typedef struct {
  uint8_t R;
  uint8_t G;
  uint8_t B;
} RGB;
RGB globalRGB = {25, 25, 25};

static SemaphoreHandle_t switch_press_indication0, switch_press_indication1, switch_press_indication2,
    switch_press_indication3;
static QueueHandle_t adc_to_pwm_task_queue, Q1;

void port0pin29isr(void) {
  LPC_GPIOINT->IO0IntClr |= (1 << 29);
  xSemaphoreGiveFromISR(switch_press_indication0, NULL);
}

void port0pin30isr_RGB(void) {
  LPC_GPIOINT->IO0IntClr |= (1 << 30);
  globalRGB.R = rand() % 100;
  globalRGB.G = rand() % 100;
  globalRGB.B = rand() % 100;
  xSemaphoreGiveFromISR(switch_press_indication1, NULL);
}

void sleep_on_semphr_task0(void *param) {
  vTaskDelay(33);
  while (1)
    if (xSemaphoreTake(switch_press_indication0, portMAX_DELAY)) {
      gpioX__toggle(GPIOPORTs[0], GPIOLEDs[0]);
      gpioX__toggle(GPIOPORTs[1], GPIOLEDs[1]);
      gpioX__toggle(GPIOPORTs[2], GPIOLEDs[2]);
      gpioX__toggle(GPIOPORTs[3], GPIOLEDs[3]);
      vTaskDelay(100);
    }
}

void pin_configure_pwm_channel_as_io_pin() {
  gpio__construct_with_function(2, 0, GPIO__FUNCTION_1);
  gpio__construct_with_function(2, 1, GPIO__FUNCTION_1);
  gpio__construct_with_function(2, 2, GPIO__FUNCTION_1);
}

void pwm_task(void *p) {
  pwm1__init_single_edge(1000);

  // Locate a GPIO pin that a PWM channel will control
  // NOTE You can use gpio__construct_with_function() API from gpio.h
  // TODO Write this function yourself
  pin_configure_pwm_channel_as_io_pin();

  // We only need to set PWM configuration once, and the HW will drive
  // the GPIO at 1000Hz, and control set its duty cycle to 50%
  pwm1__set_duty_cycle(PWM1__2_0, 50);

  // Continue to vary the duty cycle in the loop
  uint8_t percent0 = 0;
  uint8_t percent1 = 0;
  uint8_t percent2 = 0;
  while (1) {
    pwm1__set_duty_cycle(PWM1__2_0, percent0);
    pwm1__set_duty_cycle(PWM1__2_1, percent1);
    pwm1__set_duty_cycle(PWM1__2_2, percent2);
    if (++percent0 > rand() % 100)
      percent0 = rand() % 100;
    vTaskDelay(60);
    if (++percent1 > rand() % 100)
      percent1 = rand() % 100;
    vTaskDelay(60);
    if (++percent2 > rand() % 100)
      percent2 = rand() % 100;
    vTaskDelay(60);
  }
}
void sleep_on_semphr_task1(void *param) {
  pwm1__init_single_edge(1000);
  pin_configure_pwm_channel_as_io_pin();
  pwm1__set_duty_cycle(PWM1__2_0, 0);

  if (xSemaphoreTake(switch_press_indication1, portMAX_DELAY)) {
    while (1) {
      fprintf(stderr, "\nRGB set to: %d, %d, %d", globalRGB.R, globalRGB.G, globalRGB.B);
      pwm1__set_duty_cycle(PWM1__2_0, globalRGB.R);
      pwm1__set_duty_cycle(PWM1__2_1, globalRGB.G);
      pwm1__set_duty_cycle(PWM1__2_2, globalRGB.B);
      vTaskDelay(200);
    }
  }
}
pin_configure_adc_channel_as_io_pin(adc_channel_e channel) {
  switch (channel) {
  case ADC__CHANNEL_2: {
    LPC_IOCON->P0_25 &= ~(1 << 7);
    gpio__construct_with_function(0, 25, GPIO__FUNCTION_1);
  } break;
  case ADC__CHANNEL_4: {
    LPC_IOCON->P1_30 &= ~(1 << 7);
    gpio__construct_with_function(1, 30, GPIO__FUNCTION_3);

  } break;
  case ADC__CHANNEL_5: {
    LPC_IOCON->P1_31 &= ~(1 << 7);
    gpio__construct_with_function(1, 31, GPIO__FUNCTION_3);
  } break;
  default:
    break;
  }
}
// Part 1
void adc_task(void *p) {
  adc__initialize();

  // Configure a pin, such as P1.31 with FUNC 011 to route this pin as ADC channel 5
  // You can use gpio__construct_with_function() API from gpio.h
  pin_configure_adc_channel_as_io_pin(ADC__CHANNEL_2);
  // TODO This is the function you need to add to adc.h
  // You can configure burst mode for just the channel you are using
  adc__enable_burst_mode(ADC__CHANNEL_2);
  while (1) {
    // Get the ADC reading using a new routine you created to read an ADC burst reading
    // TODO: You need to write the implementation of this function
    const uint16_t adc_value = adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_2);
    printf("ADC VALUE: %d\n", adc_value);
    vTaskDelay(100);
  }
}
// Part 2
void adc_taskP2(void *p) {
  adc__initialize();
  pin_configure_adc_channel_as_io_pin(ADC__CHANNEL_2);
  adc__enable_burst_mode(ADC__CHANNEL_2);
  while (1) {
    // Get the ADC reading using a new routine you created to read an ADC burst reading
    // TODO: You need to write the implementation of this function
    const int adc_value = (int)adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_2);
    xQueueSend(adc_to_pwm_task_queue, &adc_value, 0);
    printf("\nADC VALUE SENT: %d", adc_value);
    vTaskDelay(100);
  }
}

void pwm_taskP2(void *p) {
  pwm1__init_single_edge(1000);
  pin_configure_pwm_channel_as_io_pin();
  double adc_voltage = 0;
  int percent0 = 0;

  while (1) {
    if (xQueueReceive(adc_to_pwm_task_queue, &percent0, 100)) {
      adc_voltage = (double)percent0 / 4095 * 3.3;
      fprintf(stderr, "\nMatch Register 0: %ld\nMatch Register 1: %ld\n", LPC_PWM1->MR0, LPC_PWM1->MR1);
      if (percent0 < 55)
        percent0 = 0;
      else
        percent0 = ((double)percent0 / 4095 * 100); // led ends up flashing even at 1%
      printf("\nADC converted to %d%% = %.3fV\n", percent0, adc_voltage);

      pwm1__set_duty_cycle(PWM1__2_0, percent0 * 0.1);
      pwm1__set_duty_cycle(PWM1__2_1, percent0 * 0.2);
      pwm1__set_duty_cycle(PWM1__2_2, percent0);
    }
  }
}




/////////////////////////// MAIN ///////////////////////////
int main(void) {
  srand((unsigned int)time(NULL));
  switch_press_indication0 = xSemaphoreCreateBinary();
  switch_press_indication1 = xSemaphoreCreateBinary();
  adc_to_pwm_task_queue = xQueueCreate(1, sizeof(int *));
  /* Intr Dispatcher */
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpioX__interrupt_dispatcher, 'd');

  /* Part 1, ADC OUTPUT */
  // xTaskCreate(adc_task, "ADC_output", 2048 / sizeof(void *), NULL, 2, NULL);


  /*************** onboard Led toggle switch task *****************/
  gpioX__attach_interrupt(0, 29, GPIO_INTR__RISING_EDGE, port0pin29isr);
  xTaskCreate(sleep_on_semphr_task0, "intrswitch0", 2048 / sizeof(void *), NULL, 1, NULL);
 /********************************************************/
  
  /************* Random RGB on switch *********************/
  gpioX__attach_interrupt(0, 30, GPIO_INTR__RISING_EDGE, port0pin30isr_RGB);
  xTaskCreate(sleep_on_semphr_task1, "intrswitch1", 2048 / sizeof(void *), NULL, 2, NULL);
  /********************************************************/
  

  /***************** RGB LEDs ********************/
  // xTaskCreate(pwm_task, "pwmtest", 2048 / sizeof(void *), NULL, 1, NULL);
  /**********************************************/
  
  /* Part 2 Queue RGB with ADC */
  // xTaskCreate(adc_taskP2, "ADC Producer", 2048 / sizeof(void *), NULL, 2, NULL);
  // xTaskCreate(pwm_taskP2, "Pwm Consumer", 2048 / sizeof(void *), NULL, 2, NULL);

  vTaskStartScheduler();

  return 0;
}
//////////////////////////END MAIN/////////////////////////