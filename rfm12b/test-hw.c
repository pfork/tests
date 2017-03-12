#include <stdint.h>
#include <string.h>
#include "stm32f.h"
#include "delay.h"
#include "../rsp.h"

const uint8_t msg[] = "\x2d\xe9Hello World";

// max(F) = 439756
// min(F) = 430149
#define RFM12_CMD_RESET         0xFE00  // performs software reset of RFM12 Module

/* uses pins:
gpio A 4|5|6|7 -> nss, sck, miso, mosi
*/

#define rfm12_SPIx ((SPI_Regs*) SPI1_BASE)

static uint16_t rf12_xfer (uint16_t data) {
  while(spi_status_txe(rfm12_SPIx) == 0);
  spi_send(rfm12_SPIx, data);

  while(spi_status_rxne(rfm12_SPIx) == 0);
  return spi_read(rfm12_SPIx);
}

void rfm_hw_init(void) {
  GPIO_Regs *greg;
  SPI_Regs *sreg;

  // enable gpioe(pe10 = rfm-irq)
  //MMIO32(RCC_AHB1ENR) |= RCC_AHB1Periph_GPIOE;
  //greg = (GPIO_Regs *) GPIOE_BASE;
  // enable spi for gpioe_10 for rfm12 irq
  //greg->AFR[1] &= ~((15 << (2 << 2)));
  //greg->MODER  &= ~((3 << (10 << 1)));
  //greg->MODER |=   (GPIO_Mode_IN << (10 << 1));
  //greg->PUPDR  &= ~((3 << (10 << 1)));
  //greg->PUPDR |= (GPIO_PuPd_UP << (10 << 1));
  //greg->OSPEEDR |= (GPIO_Speed_100MHz << (10 << 1));

  // enable gpioa(spi)
  MMIO32(RCC_AHB1ENR) |= RCC_AHB1Periph_GPIOA;
  // enable spi clock
  MMIO32(RCC_APB2ENR) |= RCC_APB2Periph_SPI1;

  greg = (GPIO_Regs *) GPIOA_BASE;
  // enable spi for gpioa_[5, 6, 7] (sck, miso, mosi)
  // gpioa_4 = CS
  // clear ctrl bits
  greg->AFR[0] &= ~((15 << (4 << 2)) |
                    (15 << (5 << 2)) |
                    (15 << (6 << 2)) |
                    (15 << (7 << 2)));
  greg->MODER  &= ~((3 << (4 << 1)) |
                    (3 << (5 << 1)) |
                    (3 << (6 << 1)) |
                    (3 << (7 << 1)));
  greg->PUPDR  &= ~((3 << (4 << 1)) |
                    (3 << (5 << 1)) |
                    (3 << (6 << 1)) |
                    (3 << (7 << 1)));
  // set bits
  greg->AFR[0]  |=  (GPIO_AF_SPI1 << (4 << 2)) |
                    (GPIO_AF_SPI1 << (5 << 2)) |
                    (GPIO_AF_SPI1 << (6 << 2)) |
                    (GPIO_AF_SPI1 << (7 << 2));
  greg->MODER   |=  (GPIO_Mode_AF << (4 << 1)) |
                    (GPIO_Mode_AF << (5 << 1)) |
                    (GPIO_Mode_AF << (6 << 1)) |
                    (GPIO_Mode_AF << (7 << 1));
  greg->OSPEEDR |=  (GPIO_Speed_100MHz << (4 << 1)) |
                    (GPIO_Speed_100MHz << (5 << 1)) |
                    (GPIO_Speed_100MHz << (6 << 1)) |
                    (GPIO_Speed_100MHz << (7 << 1));
  greg->PUPDR   |=  (GPIO_PuPd_UP << (4 << 1)) |
                    (GPIO_PuPd_DOWN << (5 << 1)) |
                    (GPIO_PuPd_DOWN << (6 << 1)) |
                    (GPIO_PuPd_DOWN << (7 << 1));

  sreg = (SPI_Regs *) SPI1_BASE;
  sreg->CR1 |= (unsigned short)(SPI_Direction_2Lines_FullDuplex | SPI_Mode_Master |
                                SPI_DataSize_16b | SPI_CPOL_Low  |
                                SPI_CPHA_1Edge | SPI_NSS_Hard |
                                SPI_BaudRatePrescaler_4 | SPI_FirstBit_MSB);
  sreg->CR2 |= (SPI_SSOE | SPI_ERRIE);

  /* Activate the SPI mode (Reset I2SMOD bit in I2SCFGR register) */
  sreg->I2SCFGR &= (unsigned short)~((unsigned short)SPI_I2SCFGR_I2SMOD);

  /* Write to SPIx CRCPOLY */
  //sreg->CRCPR = 7;

  sreg->CR1 |= SPI_CR1_SPE;
}

void rfm_init(uint16_t freq) {
  rf12_xfer(0x0000);//just in case
  rf12_xfer(0x8201);//sleep mode
  uDelay(100);
  rf12_xfer(0x8098);//EL,!ef,433band,12.5pF
  //rf12_xfer(0x8239);//!er,!ebb,ET,ES,EX,!eb,!ew,DC
  rf12_xfer(0xA000|(freq&0xfff));
  rf12_xfer(0xC647);//4.8kbps
  rf12_xfer(0x94A2);//VDI,FAST,134kHz,0dBm,-91Bm
  rf12_xfer(0xC2AC);//AL,!ml,DIG,DQD4
  rf12_xfer(0xCA81);//FIFO8,SYNC,!ff,DR
  rf12_xfer(0xCED4);//SYNC=2DD4;
  //rf12_xfer(0xC483);//@PWR,NO RSTRIC,!st,!fi,OE,EN
  rf12_xfer(0xC400);//!auto,NO RSTRIC,!st,!fi,!oe,!en
  rf12_xfer(0x9857);//!mp,9810=30kHz,MAX OUT
  rf12_xfer(0xCC77);//OB1,OB0,!lpx,!ddy,DDIT,BW0
  rf12_xfer(0xE000);//NOT USE
  rf12_xfer(0xC800);//NOT USE
  rf12_xfer(0xC040);//1.66MHz,2.2V
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

void test(void) {
  rfm_hw_init();

  uint16_t status = rf12_xfer(0);
  rsp_dump((uint8_t*) &status,2);

  rfm_init(0x640);

  status = rf12_xfer(0);
  rsp_dump((uint8_t*) &status,2);

  rsp_detach();
  while(1) {
    rf12_xfer(0x8239);//!er,!ebb,ET,ES,EX,!eb,!ew,DC
    uDelay(250);
    hello(msg);
    rf12_xfer(0x8201);//sleep mode
    rf12_xfer(0);
    mDelay(3000);
  }

  //rsp_detach();
  //rsp_finish();
}
