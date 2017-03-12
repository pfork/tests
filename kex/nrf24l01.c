/* uses pins:
gpio B 13|14|15 -> irq, ce, csn
gpio A 5|6|7 -> sck, miso, mosi
*/
#include <stdint.h>
#include "stm32f.h"
#include "nrf24l01.h"
#include "delay.h"
#include "../rsp.h"
#include <string.h>
#include "dma.h"

#define CHANNEL 81
#define MAX_TX_RETRIES 4000
#define nrf_write_buf(a,b,c) nrf_cmd_buf(a + WRITE_REG_NRF24L01,b,c)

// dma2, stream0, channel1
#define DMACPY_CHANNEL DMA_SxCR_CHSEL_1
#define DMACPY_FLAG_TCIF DMA_LISR_TCIF0
#define DMACPY_STREAM_REGS ((DMA_Stream_Regs *) DMA2_Stream0_BASE)

void dmacpy(void* dest, const void *buf, unsigned short len) {
  DMA_Stream_Regs *regs = DMACPY_STREAM_REGS;
  // setup DMA
  /* memory source address */
  regs->PAR = (uint32_t) buf;
  regs->NDTR = len;
  /* dest address */
  regs->M0AR = (uint32_t) dest;
  /* DMA enable */
  regs->CR |= DMA_SxCR_EN;
}

void dmacpy_init(void) {
  RCC->AHB1ENR |= RCC_AHB1Periph_DMA2;

  irq_disable(NVIC_DMA2_STREAM0_IRQ);
  DMA_Stream_Regs *regs = DMACPY_STREAM_REGS;
  /* DMA disable */
  regs->CR &= ~((unsigned int) DMA_SxCR_EN);
  /* wait till cleared */
  while(regs->CR & ((unsigned int) DMA_SxCR_EN));
  /* DMA deConfig */
  DMA_DeInit(DMACPY_STREAM_REGS);
  // setup DMA
  /* DMA Config */
  regs->CR |= (DMACPY_CHANNEL | DMA_SxCR_DIR_MEM_TO_MEM |
               DMA_SxCR_MINC | DMA_SxCR_PINC |
               DMA_SxCR_PSIZE_32BIT | DMA_SxCR_MSIZE_32BIT |
               //DMA_SxCR_TCIE | DMA_SxCR_HTIE | DMA_SxCR_TEIE | DMA_SxCR_DMEIE |/* enable tx complete interrupt */
               DMA_SxCR_TCIE |
               DMA_SxCR_PL_VERY_HIGH );
}

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

static void nrf_reset(void) {
  CSN(0); SPI_send(nrf24l0_SPIx, FLUSH_RX); CSN(1);
  CSN(0); SPI_send(nrf24l0_SPIx, FLUSH_TX); CSN(1);
  nrf_write_reg(STATUS, RX_DR | TX_DS | MAX_RT);
}

#define HEADER_LEN 6

uint32_t listen(uint8_t chan, uint8_t* address, uint8_t* msg, uint32_t len) {
  CE(0);
  nrf_write_reg(RF_CH, chan);      // set channel
  nrf_write_buf(RX_ADDR_P0, address, ADDR_WIDTH);
  nrf_write_buf(TX_ADDR, address, ADDR_WIDTH);
  nrf_write_reg(CONFIG, EN_CRC | CRC0 | PWR_UP | PRIM_RX);  // 2-byte crc
  CE(1);
  uDelay(10);

  uint32_t rlen=0, mlen=0, idx=0, retries=1<<22, i;
  uint8_t status, plen, pkt[32];

  nrf_reset();

  while(rlen<len && (mlen==0 || rlen<mlen)) {
    for(i=0;i<retries;i++) {
      if(!IRQ) {
        // something happened
        //CE(0);
        status = nrf_read_reg(STATUS);
        if(status & RX_DR) {
          // is it something confusing?
          if((status & 0xe) == 0xe) {
            nrf_reset();
            //CE(1);
            //rsp_dump((uint8_t*) "0xe", 3);
            //uDelay(1);
            continue;
          }
          // or is it something else confusing?
          if(nrf_read_reg(FIFO_STATUS) & RX_EMPTY) {
            nrf_reset();
            //CE(1);
            //rsp_dump((uint8_t*) "emp", 3);
            //uDelay(1);
            continue;
          }
          // let's check the length
          nrf_read_buf(RD_RX_PLOAD_WID,(unsigned char*) &plen,1);
          if(plen>32 || plen==0) {
            nrf_reset();
            //CE(1);
            //rsp_dump((uint8_t*) &plen, 1);
            //uDelay(1);
            continue;
          }
          // read receive payload from RX_FIFO buffer
          nrf_read_buf(RD_RX_PLOAD, pkt, plen);
          // clear stuff according to man
          nrf_write_reg(STATUS, RX_DR | TX_DS | MAX_RT);
          if(nrf_read_reg(FIFO_STATUS) & RX_EMPTY) {
            CSN(0); SPI_send(nrf24l0_SPIx, FLUSH_RX); CSN(1);
          }
          // start listening again
          //CE(1);
          // for debug
          //if(plen<32) {
          //  rsp_dump((uint8_t*) pkt, plen);
          //  rsp_dump((uint8_t*) &plen, 1);
          //}
          if((plen-(mlen==0?6:4))+rlen>len) {
            //if(plen!=32) rsp_dump((uint8_t*) &plen, 1);
            //rsp_dump((uint8_t*) &rlen, 1);
            //rsp_dump((uint8_t*) &len, 1);
            //rsp_dump((uint8_t*) pkt, 32);
            //rsp_dump((uint8_t*) "big", 3);
            //uDelay(1);
            continue;
            //rsp_dump((uint8_t*) "overflow", 8);
            //CE(0);
            //return 0;
          }
          if(mlen==0) { // 1st packet tells us mlen
            if(*((uint16_t*) pkt) != 0x3031) {
              // not our packet
              //rsp_dump((uint8_t*) pkt, plen);
              continue;
            }
            //rsp_dump((uint8_t*) "recv0", 5);
            //rsp_dump((uint8_t*) pkt, 6);
            mlen = *((uint32_t*) (pkt+2));
            //rsp_dump((uint8_t*) &mlen, 4);
            if(len<plen-HEADER_LEN) {
              rsp_dump((uint8_t*) "pmess1", 6);
              CE(0);
              return 0;
            }
            dmacpy(msg,pkt+HEADER_LEN,plen-HEADER_LEN);
            //rsp_dump((uint8_t*) pkt+HEADER_LEN, (plen<26)?plen:26);
            rlen+=plen-HEADER_LEN;
            //rsp_dump((uint8_t*) &rlen, 1);
          } else {
            // body packet
            //rsp_dump((uint8_t*) "recv1", 5);
            if(*((uint16_t*) pkt) == 0x3031) {
              continue;
            }
            if(idx<*((uint32_t*)pkt)) {
              // packet loss - abort
              //rsp_dump((uint8_t*) &idx, 4);
              rsp_dump((uint8_t*) &pkt, 4);
              //rsp_dump((uint8_t*) "pmess2", 6);
              continue;
              CE(0);
              return 0;
            } else if(idx>*((uint32_t*)pkt)) {
              // already seen packet
              //uDelay(1);
              continue;
            }
            if(len-rlen<plen-4) {
              // packet too big
              rsp_dump((uint8_t*) &len, 4);
              rsp_dump((uint8_t*) &rlen, 4);
              rsp_dump((uint8_t*) &plen, 1);
              rsp_dump((uint8_t*) "pmess3", 6);
              CE(0);
              return 0;
            }
            dmacpy(msg,pkt+4,plen-4);
            //rsp_dump((uint8_t*) pkt+4, (plen<28)?plen:28);
            rlen+=plen-4;
          }
          break;
        } else {
          nrf_reset();
        }
        //CE(1);
      }
      //ASM_DELAY(4800);
      uDelay(30);
    }
    if(i>=retries) {
      rsp_dump((uint8_t*) &idx, 4);
      rsp_dump((uint8_t*) "timeout", 7);
      CE(0);
      return 0;
    }
  }

  CE(0);
  return rlen;
}

uint32_t send(uint8_t chan, uint8_t* address, uint8_t* msg, uint32_t len) {
  CE(0);
  nrf_write_reg(RF_CH, chan);      // set channel
  nrf_write_buf(RX_ADDR_P0, address, ADDR_WIDTH);
  nrf_write_buf(TX_ADDR, address, ADDR_WIDTH);

  // flush stuff
  nrf_reset();

  // power up
  nrf_write_reg(CONFIG, EN_CRC | CRC0 | PWR_UP);

  uint8_t status=0;
  unsigned int i, n=len;
  uint8_t pkt[32];
  uint32_t idx=0;

  *((uint16_t*) pkt)=0x3031; // version
  *((uint32_t*) (pkt+2))=len;    // raw len in bytes

  memcpy(pkt+6,msg,(len<(32-HEADER_LEN))?len:(32-HEADER_LEN));
  nrf_cmd_buf(WR_TX_PLOAD, pkt, (len<(32-HEADER_LEN))?len+HEADER_LEN:32);
  n-=(n<(32-HEADER_LEN))?n:(32-HEADER_LEN);
  while(1) {
    // blocking send with max retries
    for(i=0;i<MAX_TX_RETRIES;i++) {
      CE(1);
      while(IRQ); // lamely blocking
      status = nrf_read_reg(STATUS);
      //rsp_dump((uint8_t*) &status, 1);
      if(status & TX_DS) {
        nrf_write_reg(STATUS, RX_DR | TX_DS | MAX_RT);
        break;
      }
      if(status & MAX_RT) {
        nrf_write_reg(STATUS, RX_DR | TX_DS | MAX_RT);
      }
      if(i==0) {
        CSN(0); SPI_send(nrf24l0_SPIx, REUSE_TX_PL); CSN(1);
        ASM_DELAY(4000);
      }
      CE(0);
    }

    //if(i>=MAX_TX_RETRIES) {
    // "timeout"
    //  CE(0);
    //  return 0;
    //}

    if(n==0) {
      CE(0);
      return len;
    }

    nrf_reset();
    *((uint32_t*) pkt)=idx++;
    memcpy(pkt+4,msg+(len-n),(n<28)?n:28);
    rsp_dump((uint8_t*) pkt, (n<28)?n+4:32);
    nrf_cmd_buf(WR_TX_PLOAD, pkt, (n<28)?n+4:32);
    n-=(n<28)?n:28;
    if(idx>5) rsp_detach();
    //rsp_dump((uint8_t*) "2ndpkt", 6);
  }
}
