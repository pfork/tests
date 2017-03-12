#include <stdint.h>
#include <string.h>
#include "stm32f.h"
#include "delay.h"
#include "../rsp.h"

const uint8_t msg[] = "\x2d\xe9Hello World";

/* uses pins:
gpio A 4|5|6|7 -> nss, sck, miso, mosi
*/
// max(F) = 439756
// min(F) = 430149

#define CSPIN 4
#define SCKPIN 5
#define SDOPIN 6
#define SDIPIN 7

#define CS(x)					x ? (gpio_set(GPIOA_BASE, 1 << CSPIN)) : (gpio_reset(GPIOA_BASE, 1 << CSPIN));
#define SCK(x)					x ? (gpio_set(GPIOA_BASE, 1 << SCKPIN)) : (gpio_reset(GPIOA_BASE, 1 << SCKPIN));
#define SDI(x)					x ? (gpio_set(GPIOA_BASE, 1 << SDIPIN)) : (gpio_reset(GPIOA_BASE, 1 << SDIPIN));
#define SDO()					gpio_get(GPIOA_BASE, 1 << SDOPIN)

static uint16_t rf12_xfer (uint16_t cmd) {
  unsigned char i;
  unsigned int temp = 0;
  SCK(0);
  CS(0);
  for(i=0;i<16;i++){
    SCK(0);
    if(cmd&0x8000){
      SDI(1);
    }else{
      SDI(0);
    }
    temp<<=1;
    if(SDO()){
      temp|=0x0001;
    }
    SCK(1);
    uDelay(1);
    cmd<<=1;
  };
  SCK(0);
  CS(1);
  return(temp);
}

void hello(const uint8_t* msg) {
  uint16_t i,j;
  for(i=0;i<8;i++) {
    rf12_xfer(0xb8aa);
    mDelay(1);
  }
  for(j=0;msg[j]!=0;j++) {
    rf12_xfer(0xb800 | msg[j]);
    mDelay(1);
  }
  for(i=0;i<(128-21);i++) {
    rf12_xfer(0xb800 | i);
    mDelay(1);
  }
  for(i=0;i!=32;i++) {
    rf12_xfer(0xb800);
    mDelay(1);
  }
  for(i=0;i!=32;i++) {
    rf12_xfer(0xb8ff);
    mDelay(1);
  }
  for(i=0;i!=64;i++) {
    rf12_xfer(0xb855);
    mDelay(1);
  }
}

void rfm_hw_init(void) {
  GPIO_Regs *greg;

  // enable gpioa(spi)
  MMIO32(RCC_AHB1ENR) |= RCC_AHB1Periph_GPIOA;

  greg = (GPIO_Regs *) GPIOA_BASE;
  // enable spi for gpioa_[4, 5, 6, 7] (ss, sck, miso, mosi)
  greg->AFR[0] &= ~((15 << (4 << 2)) |(15 << (5 << 2)) | (15 << (6 << 2)) | (15 << (7 << 2)));
  greg->MODER  &= ~((3 << (4 << 1)) |(3 << (5 << 1)) | (3 << (6 << 1)) | (3 << (7 << 1)));
  greg->PUPDR  &= ~((3 << (4 << 1)) |(3 << (5 << 1)) | (3 << (6 << 1)) | (3 << (7 << 1)));
  greg->MODER |=   (GPIO_Mode_OUT << (4 << 1)) | (GPIO_Mode_OUT << (5 << 1)) | (GPIO_Mode_IN << (6 << 1)) | (GPIO_Mode_OUT << (7 << 1));
  greg->OSPEEDR |= (GPIO_Speed_100MHz << (4 << 1)) | (GPIO_Speed_100MHz << (5 << 1)) | (GPIO_Speed_100MHz << (6 << 1)) | (GPIO_Speed_100MHz << (7 << 1));
  greg->PUPDR |= (GPIO_PuPd_UP << (4 << 1)) | (GPIO_PuPd_DOWN << (5 << 1)) | (GPIO_PuPd_DOWN << (7 << 1));
  CS(1);
}

void rfm_init(void) {
  rf12_xfer(0x0000);//read status register - just in case
  rf12_xfer(0x8201);//!er,!ebb,ET,ES,EX,!eb,!ew,DC
  rf12_xfer(0x80D8);//EL,EF,433band,12.5pF
  //rf12_xfer(0x8239);//!er,!ebb,ET,ES,EX,!eb,!ew,DC
  rf12_xfer(0xA640);//A140=430.8MHz, A640=434.24MHz
  rf12_xfer(0xC647);//4.8kbps
  rf12_xfer(0x9420);//VDI,FAST,400kHz,0dBm,-103dBm
  rf12_xfer(0xC2AC);//AL,!ml,DIG,DQD4
  rf12_xfer(0xCA81);//FIFO8,SYNC,!ff,DR
  rf12_xfer(0xCED4);//SYNC=2DD4;
  rf12_xfer(0xC483);//@PWR,NO RSTRIC,!st,!fi,OE,EN
  rf12_xfer(0x9850);//!mp,9810=30kHz,MAX OUT
  rf12_xfer(0xCC17);//!OB1,!OB0,!lpx,!ddy,DDIT,BW0
  rf12_xfer(0xE000);//NOT USE
  rf12_xfer(0xC800);//NOT USE
  rf12_xfer(0xC040);//1.66MHz,2.2V
}

void send_test(void) {
  int i;
  while(1) {
    for(i=96;i<=3903;i++) {
      rf12_xfer(0x8239);//!er,!ebb,ET,ES,EX,!eb,!ew,DC
      rf12_xfer(0xA000|i);
      rf12_xfer(0x0000); //read status register
      uDelay(250);
      hello(msg);
      rf12_xfer(0x8201);//sleep mode
      rf12_xfer(0);
      mDelay(1000);
    }
  }
}

void test(void) {
  rfm_hw_init();
  rfm_init();

  rsp_detach();
  send_test();

  rsp_finish();
}
