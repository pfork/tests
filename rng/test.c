#include <usb.h>
#include "stm32f.h"
#include "../rsp.h"
#include <string.h>

#define SWEN2 3 // PB3
#define SWEN1 7 // PB7
#define COMP2 8 // PB8
#define COMP1 9 // PB9

//void xrng_init(void) {
//  GPIO_Regs *greg;
//  // enable gpiob, gpioa(spi)
//  MMIO32(RCC_AHB1ENR) |= RCC_AHB1Periph_GPIOB;
//
//  greg = (GPIO_Regs *) GPIOB_BASE;
//  greg->MODER &= ~((3 << (SWEN2 << 1)) |
//                   (3 << (SWEN1 << 1)) |
//                   (3 << (COMP2 << 1)) |
//                   (3 << (COMP1 << 1)));
//  greg->MODER |= ((GPIO_Mode_OUT << (SWEN2 << 1)) |
//                  (GPIO_Mode_OUT << (SWEN1 << 1)) |
//                  (GPIO_Mode_IN << (COMP2 << 1)) |
//                  (GPIO_Mode_IN << (COMP1 << 1)));
//  greg->OSPEEDR |= ((GPIO_Speed_100MHz << (SWEN2 << 1)) |
//                    (GPIO_Speed_100MHz << (SWEN1 << 1)) |
//                    (GPIO_Speed_100MHz << (COMP2 << 1)) |
//                    (GPIO_Speed_100MHz << (COMP1 << 1)));
//  greg->PUPDR &= ~((3 << (SWEN2 << 1)) |
//                   (3 << (SWEN1 << 1)) |
//                   (3 << (COMP2 << 1)) |
//                   (3 << (COMP1 << 1)));
//  greg->PUPDR |= ((GPIO_PuPd_DOWN << (SWEN2 << 1)) |
//                  (GPIO_PuPd_DOWN << (SWEN1 << 1))); // |
//                  //(GPIO_PuPd_DOWN << (COMP2 << 1)) |
//                  //(GPIO_PuPd_DOWN << (COMP1 << 1)));
//}

//void xadc_init ( void ) {
//
//  //enable adc1
//  MMIO32(RCC_APB2ENR) |= 1<<8;
//
//  // disable temperature/vref sensor
//  ADC_CCR &= ~ADC_CCR_TSVREFE;
//  // disable vbat sensor
//  ADC_CCR &= ~ADC_CCR_VBATEN;
//  // adc off
//  ADC_CR2(ADC1) &= ~ADC_CR2_ADON;
//  // scanmode off
//  ADC_CR1(ADC1) &= ~ADC_CR1_SCAN;
//  // single conversion mode
//  ADC_CR2(ADC1) &= ~ADC_CR2_CONT;
//  // disable external trigger
//  ADC_CR2(ADC1) &= ~ADC_CR2_EXTTRIG;
//  // right aligned
//  ADC_CR2(ADC1) &= ~ADC_CR2_ALIGN;
//  // set sampling time
//  ADC_SMPR1(ADC1) |= (ADC_SMPR_SMP_1DOT5CYC << ((ADC_CHANNEL16 - 10) * 3)) |
//                     (ADC_SMPR_SMP_1DOT5CYC << ((ADC_CHANNEL17 - 10) * 3)) |
//                     (ADC_SMPR_SMP_1DOT5CYC << ((ADC_CHANNEL18 - 10) * 3));
//  // enable EOC irq
//  ADC_CR1(ADC1) |= ADC_CR1_EOCIE;
//  ADC_SQR1(ADC1) = 0; // only one entry
//}

/* static unsigned short read_chan( unsigned char chan ) { */
/*   unsigned int res = 0x800; */

/*   // set chan */
/*   ADC_SQR3(ADC1) = (chan << (0 * 5)); */

/*   while(res==0x800) { */
/*     // start conversion */
/*     if(chan==ADC_CHANNEL16 || chan==ADC_CHANNEL17) ADC_CCR |= ADC_CCR_TSVREFE; */
/*     else if(chan==ADC_CHANNEL18) ADC_CCR |= ADC_CCR_VBATEN; */
/*     //else while(1) */
/*     ADC_CR2(ADC1) |= ADC_CR2_ADON; */
/*     //while((ADC_CR2(ADC1) & ADC_CR2_SWSTART) == 0) ADC_CR2(ADC1) |= ADC_CR2_SWSTART; */
/*     ADC_CR2(ADC1) |= ADC_CR2_SWSTART; */

/*     // wait till end of conversion */
/*     while ((ADC_SR(ADC1) & ADC_SR_EOC) == 0) ; */
/*     res = ADC_DR(ADC1); */
/*     // stop adc */
/*     ADC_CR2(ADC1) &= ~ADC_CR2_SWSTART; */
/*     ADC_CR2(ADC1) &= ~ADC_CR2_ADON; */
/*     if(chan==ADC_CHANNEL16 || chan==ADC_CHANNEL17) */
/*       ADC_CCR &= ~ADC_CCR_TSVREFE; */
/*     else if(chan==ADC_CHANNEL18) */
/*       ADC_CCR &= ~ADC_CCR_VBATEN; */
/*     else while(1); */
/*   } */
/*   return res; */
/* } */

void test(void) {
  uint8_t b[64];
  //xrng_init();
  int i=0;
  rsp_detach();
  //xadc_init();
  while(1) {
    //b[i%64] = (uint8_t) (0xff & read_chan(ADC_CHANNEL17));
    //if((i%64)==63) usb_write(b, sizeof(b), 0, USB_CRYPTO_EP_DATA_OUT);
    //i++;
    //continue;
    // external entropy source
    // alternate swen1/swen2
    if(i%2==0) {
      gpio_reset(GPIOB_BASE, 1 << SWEN1);
      gpio_set(GPIOB_BASE, 1 << SWEN2);
    } else {
      gpio_reset(GPIOB_BASE, 1 << SWEN2);
      gpio_set(GPIOB_BASE, 1 << SWEN1);
    }

    // read out comp1/comp2
    //b[i%64] = !!gpio_get(GPIOB_BASE, 1<< COMP1);
    //b[(i+1)%64] = !!gpio_get(GPIOB_BASE, 1<< COMP2);
    b[(i/8)%64]= (b[(i/8)%64] << 1) | ((i & 1) ? (!!gpio_get(GPIOB_BASE, 1<< COMP1)) : (!!gpio_get(GPIOB_BASE, 1<< COMP2)));
    if((i%512)==511) {
       usb_write(b, sizeof(b), 0, USB_CRYPTO_EP_DATA_OUT);
       memset(b,0, sizeof(b));
    }
    i++;
  }
  rsp_finish();
}
