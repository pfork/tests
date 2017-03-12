#include "stm32f.h"

#define DCPIN 4
#define RSTPIN 5
#define CSPIN 6
#ifdef HWrev1
#define SDCLKPIN 15
#define SDINPIN 13
#else
#define SDCLKPIN 13
#define SDINPIN 15
#endif // HWrev1

#define DC(x)					x ? (gpio_set(GPIOB_BASE, 1 << DCPIN)) : (gpio_reset(GPIOB_BASE, 1 << DCPIN));
#define CS(x)					x ? (gpio_set(GPIOB_BASE, 1 << CSPIN)) : (gpio_reset(GPIOB_BASE, 1 << CSPIN));
#define SCLK(x)				x ? (gpio_set(GPIOB_BASE, 1 << SDCLKPIN)) : (gpio_reset(GPIOB_BASE, 1 << SDCLKPIN));
#define SDIN(x)				x ? (gpio_set(GPIOB_BASE, 1 << SDINPIN)) : (gpio_reset(GPIOB_BASE, 1 << SDINPIN));
#define RST(x)					x ? (gpio_set(GPIOB_BASE, 1 << RSTPIN)) : (gpio_reset(GPIOB_BASE, 1 << RSTPIN));

#include "delay.h"

void test(void) {
   int i=0;
   while(1) {
      DC(i&1);
      CS(i&2);
      SCLK(i&4);
      RST(i&8);
      uDelay(1);
      i++;
   }
}
