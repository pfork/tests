/* uses pins:
gpio B 13|14|15 -> irq, ce, csn
gpio A 5|6|7 -> sck, miso, mosi
*/
#include <stdint.h>
#include "stm32f.h"
#include "nrf24l01.h"
#include "delay.h"
#include "../rsp.h"

#define CHANNEL 81
#define MAX_TX_RETRIES 10
#define nrf_write_buf(a,b,c) nrf_cmd_buf(a + WRITE_REG_NRF24L01,b,c)

static void SPI_send(SPI_Regs* SPIx, unsigned char data) {
  while(spi_status_txe(SPIx) == 0);
  spi_send(SPIx, data);

  while(spi_status_rxne(SPIx) == 0);
  spi_read(SPIx);
}

static unsigned char SPI_read(SPI_Regs* SPIx, unsigned char data) {
  while(spi_status_txe(SPIx) == 0);
  spi_send(SPIx, data);

  while(spi_status_rxne(SPIx) == 0);
  return spi_read(SPIx);
}

unsigned char nrf_write_reg(unsigned char reg, unsigned char value) {
	unsigned char ret;
	CSN(0);
	ret = SPI_read(nrf24l0_SPIx, WRITE_REG_NRF24L01 + reg);
	SPI_send(nrf24l0_SPIx, value);
	CSN(1);
	return(ret);
}
unsigned char nrf_read_reg(unsigned char reg) {
	unsigned char ret;
	CSN(0);
	SPI_send(nrf24l0_SPIx, reg);
	ret = SPI_read(nrf24l0_SPIx, 0);
	CSN(1);
	return(ret);
}
unsigned char nrf_read_buf(unsigned char reg, unsigned char *pBuf, unsigned char bytes) {
	unsigned char ret, ctr;
	CSN(0);
	ret = SPI_read(nrf24l0_SPIx, reg);
	for(ctr = 0;ctr<bytes;ctr++)
		pBuf[ctr]= SPI_read(nrf24l0_SPIx, 0xff);
	CSN(1);
	return(ret);
}
unsigned char nrf_cmd_buf(unsigned char reg, unsigned char *pBuf, unsigned char bytes) {
	unsigned char ret, ctr;
	CSN(0);
	ret = SPI_read(nrf24l0_SPIx, reg);
	for(ctr = 0;ctr<bytes;ctr++)
		SPI_send(nrf24l0_SPIx, *pBuf++);
	CSN(1);
	return(ret);
}

// CE  pb10 CSN pb1 IRQ pa4
void nrf24_init(void) {
  GPIO_Regs *greg;
  SPI_Regs *sreg;

  // enable gpiob, gpioa(spi)
  MMIO32(RCC_AHB1ENR) |= RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOA;
  // enable spi clock
  MMIO32(RCC_APB2ENR) |= RCC_APB2Periph_SPI1;

  greg = (GPIO_Regs *) GPIOB_BASE;
  greg->MODER &= ~((3 << (CSNPIN << 1)) | (3 << (CEPIN << 1)));
  greg->MODER |= (GPIO_Mode_OUT << (CEPIN << 1)) | (GPIO_Mode_OUT << (CSNPIN << 1));
  greg->OSPEEDR |= (GPIO_Speed_100MHz << (CSNPIN << 1)) | (GPIO_Speed_100MHz << (CEPIN << 1));
  greg->PUPDR &= ~((3 << (CSNPIN << 1)) | (3 << (CEPIN << 1)));
  greg->PUPDR |= (GPIO_PuPd_DOWN << (CEPIN << 1)) | (GPIO_PuPd_UP << (CSNPIN << 1));
  CE(0);
  CSN(1);

  greg = (GPIO_Regs *) GPIOA_BASE;
  // enable irq (pa4) and spi for gpioa_[5, 6, 7] (sck, miso, mosi)
  greg->AFR[0] &= ~((15 << (5 << 2)) | (15 << (6 << 2)) | (15 << (7 << 2)));
  greg->MODER  &= ~((3 << (5 << 1)) | (3 << (6 << 1)) | (3 << (7 << 1)) | (3 << (IRQPIN << 1)));
  greg->PUPDR  &= ~((3 << (5 << 1)) | (3 << (6 << 1)) | (3 << (7 << 1)) | (3 << (IRQPIN << 1)));
  greg->AFR[0] |=  (GPIO_AF_SPI1 << (5 << 2)) | (GPIO_AF_SPI1 << (6 << 2)) | (GPIO_AF_SPI1 << (7 << 2));
  greg->MODER |=   (GPIO_Mode_AF << (5 << 1)) |
                   (GPIO_Mode_AF << (6 << 1)) |
                   (GPIO_Mode_AF << (7 << 1)) |
                   (GPIO_Mode_IN << (IRQPIN << 1));
  greg->OSPEEDR |= (GPIO_Speed_100MHz << (5 << 1)) |
                   (GPIO_Speed_100MHz << (6 << 1)) |
                   (GPIO_Speed_100MHz << (7 << 1)) |
                   (GPIO_Speed_100MHz << (IRQPIN << 1));
  greg->PUPDR |= (GPIO_PuPd_DOWN << (5 << 1)) |
                 (GPIO_PuPd_DOWN << (6 << 1)) |
                 (GPIO_PuPd_DOWN << (7 << 1)) |
                 (GPIO_PuPd_UP << (IRQPIN << 1));

  gpio_set(GPIOA_BASE, IRQPIN);

  sreg = (SPI_Regs *) SPI1_BASE;
  sreg->CR1 |= (unsigned short)(SPI_Direction_2Lines_FullDuplex | SPI_Mode_Master |
                                SPI_DataSize_8b | SPI_CPOL_Low  |
                                SPI_CPHA_1Edge | SPI_NSS_Soft |
                                SPI_BaudRatePrescaler_16 | SPI_FirstBit_MSB);

  /* Activate the SPI mode (Reset I2SMOD bit in I2SCFGR register) */
  sreg->I2SCFGR &= (unsigned short)~((unsigned short)SPI_I2SCFGR_I2SMOD);

  /* Write to SPIx CRCPOLY */
  sreg->CRCPR = 7;

  sreg->CR1 |= SPI_CR1_SPE;

  // enhanced shockburst
  nrf_write_reg(EN_AA, 0x3f);    // Enable all auto acks
  nrf_write_reg(DYNPD, 3);       // Enables DYNAMIC_PAYLOAD for pipes 0&1
  nrf_write_reg(FEATURE, 5);     // Enables the W_TX_PAYLOAD_NOACK + DYNAMIC_PAYLOAD command
  //nrf_write_reg(FEATURE, 1);     // Enables the W_TX_PAYLOAD_NOACK command

  // shockburst
  //nrf_write_reg(EN_AA, 0x0);     // disable able all auto acks
  //nrf_write_reg(DYNPD, 0);       // Disables DYNAMIC_PAYLOAD
  //nrf_write_reg(FEATURE, 0);     // Disables the W_TX_PAYLOAD_NOACK + DYNAMIC_PAYLOAD command

  nrf_write_reg(SETUP_AW, 0x3);    // 5 byte addr len
  //nrf_write_reg(SETUP_AW, 0x0);    // 2 byte mac len for sniffing

  // pipe0 is for host
  //nrf_write_reg(RX_PW_P0, PLOAD_WIDTH);
  // pipe1 is for broadcasts
  //nrf_write_buf(RX_ADDR_P1, BROADCAST_ADDRESS, ADDR_WIDTH);
  //nrf_write_reg(RX_PW_P1, PLOAD_WIDTH);
  //nrf_write_reg(EN_RXADDR, 0x3);   // enable 0&1 data pipes
  nrf_write_reg(EN_RXADDR, 0x1);   // enable data pipe 0

  nrf_write_reg(SETUP_RETR, 0x2f); // 750us/15 retransmits

  //nrf_write_reg(RF_CH, CHANNEL);      // set channel
  nrf_write_reg(RF_SETUP, RF_DR_HIGH | RF_PWR_0dbm);
}

void nrf_open(uint8_t chan, unsigned char* address, uint8_t rx) {
  CE(0);
  nrf_write_reg(RF_CH, chan);      // set channel
  nrf_write_buf(RX_ADDR_P0, address, ADDR_WIDTH);
  nrf_write_buf(TX_ADDR, address, ADDR_WIDTH);

  // flush stuff
  nrf_write_reg(STATUS, RX_DR | TX_DS | MAX_RT);
  CSN(0); SPI_send(nrf24l0_SPIx, FLUSH_RX); CSN(1);
  CSN(0); SPI_send(nrf24l0_SPIx, FLUSH_TX); CSN(1);

  // power up
  if(!rx) {
    nrf_write_reg(CONFIG, EN_CRC | CRC0 | PWR_UP);
    //nrf_write_reg(CONFIG, EN_CRC | PWR_UP);
    //nrf_write_reg(CONFIG, PWR_UP);
  } else {
    nrf_write_reg(CONFIG, EN_CRC | CRC0 | PWR_UP | PRIM_RX);  // 2-byte crc
    // nrf_write_reg(CONFIG, EN_CRC | PWR_UP | PRIM_RX);  // 1-byte crc
    //nrf_write_reg(CONFIG, PWR_UP | PRIM_RX);  // no crc
    // start listening
    CE(1);
  }
}

void nrf_send(unsigned char * tx_buf, unsigned char tx_buf_len) {
  //volatile unsigned char status=0;
  unsigned char status=0;
  unsigned int i;
  // blocking send
  nrf_cmd_buf(WR_TX_PLOAD, tx_buf, tx_buf_len);
  for(i=0;i<MAX_TX_RETRIES;i++) {
    CE(1);
    while(IRQ); // lamely blocking
    CE(0);
    status = nrf_read_reg(STATUS);
    //rsp_dump((uint8_t*) &status, 1);
    if(status & TX_DS) {
      nrf_write_reg(STATUS, RX_DR | TX_DS | MAX_RT);
      break;
    }
    if(status & MAX_RT) {
      nrf_write_reg(STATUS, RX_DR | TX_DS | MAX_RT);
    }
  }
}

/* char nrf_recv(unsigned char * buf, unsigned char buflen) { */
/*   unsigned char len; */
/*   nrf_write_reg(STATUS, RX_DR | TX_DS | MAX_RT); */
/*   CSN(0); SPI_send(nrf24l0_SPIx, FLUSH_RX); CSN(1); */
/*   CSN(0); SPI_send(nrf24l0_SPIx, FLUSH_TX); CSN(1); */
/*   while(IRQ); // lamely blocking */

/*   if(!(nrf_read_reg(STATUS) & RX_DR) || */
/*      nrf_read_reg(FIFO_STATUS) & RX_EMPTY) { */
/*     nrf_write_reg(STATUS, RX_DR | TX_DS | MAX_RT); */
/*     CSN(0); SPI_send(nrf24l0_SPIx, FLUSH_RX); CSN(1); */
/*     CSN(0); SPI_send(nrf24l0_SPIx, FLUSH_TX); CSN(1); */
/*     return 0; */
/*   } */
/*   nrf_read_buf(RD_RX_PLOAD_WID,(unsigned char*) &len,1); */
/*   if(len>32 || len==0 || len>buflen) { */
/*     CSN(0); SPI_send(nrf24l0_SPIx, FLUSH_RX); CSN(1); */
/*     nrf_write_reg(STATUS, RX_DR | TX_DS | MAX_RT); */
/*     return 0; */
/*   } */
/*   nrf_read_buf(RD_RX_PLOAD, buf, len);// read receive payload from RX_FIFO buffer */
/*   nrf_write_reg(STATUS, RX_DR | TX_DS | MAX_RT); */
/*   return len; */
/* } */

char nrf_recv(unsigned char * buf, unsigned char buflen) {
  unsigned char len;
  uint32_t retries=1024;
  nrf_write_reg(STATUS, RX_DR | TX_DS | MAX_RT);
  CSN(0); SPI_send(nrf24l0_SPIx, FLUSH_RX); CSN(1);
  CSN(0); SPI_send(nrf24l0_SPIx, FLUSH_TX); CSN(1);

  int i;
  for(i=0;i<retries;i++) {
    if(IRQ) {
        if((nrf_read_reg(STATUS) & RX_DR)) {
          if(nrf_read_reg(FIFO_STATUS) & RX_EMPTY) {
            nrf_write_reg(STATUS, RX_DR | TX_DS | MAX_RT);
            CSN(0); SPI_send(nrf24l0_SPIx, FLUSH_RX); CSN(1);
            CSN(0); SPI_send(nrf24l0_SPIx, FLUSH_TX); CSN(1);
          } else {
            nrf_read_buf(RD_RX_PLOAD_WID,(unsigned char*) &len,1);
            if(len>32 || len==0 || len>buflen) {
              nrf_write_reg(STATUS, RX_DR | TX_DS | MAX_RT);
              CSN(0); SPI_send(nrf24l0_SPIx, FLUSH_RX); CSN(1);
              CSN(0); SPI_send(nrf24l0_SPIx, FLUSH_TX); CSN(1);
              continue;
            }
            nrf_read_buf(RD_RX_PLOAD, buf, len);// read receive payload from RX_FIFO buffer
            nrf_write_reg(STATUS, RX_DR | TX_DS | MAX_RT);
            break;
          }
        }
        nrf_write_reg(STATUS, RX_DR | TX_DS | MAX_RT);
    }
    uDelay(10);
  }
  if(i>=retries) return 0;
  rsp_dump((uint8_t*) &i, 4);
  return len;
}
