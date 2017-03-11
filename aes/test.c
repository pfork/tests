#include <stdint.h>
#include "stm32f.h"
#include "delay.h"
#include "../rsp.h"

#define CRYP_AlgoMode_AES_ECB     ((uint16_t)0x0020)
#define CRYP_AlgoMode_AES_CBC     ((uint16_t)0x0028)
#define CRYP_AlgoMode_AES_CTR     ((uint16_t)0x0030)
#define CRYP_AlgoMode_AES_Key     ((uint16_t)0x0038)

#define CRYP_DataType_32b         ((uint16_t)0x0000)
#define CRYP_DataType_16b         ((uint16_t)0x0040)
#define CRYP_DataType_8b          ((uint16_t)0x0080)
#define CRYP_DataType_1b          ((uint16_t)0x00C0)

#define CRYP_KeySize_128b         ((uint16_t)0x0000)
#define CRYP_KeySize_192b         ((uint16_t)0x0100)
#define CRYP_KeySize_256b         ((uint16_t)0x0200)

#define MODE_ENCRYPT              ((uint8_t)0x01)
#define MODE_DECRYPT              ((uint8_t)0x00)

#define CRYP_CR_FFLUSH            (1 << 14)
#define CRYP_CR_CRYPEN            (1 << 15)

#define RCC_AHB2Periph_CRYP       ((uint32_t)0x00000010)

#define 	CRYP_SR_IFEM             (1 << 0)
#define 	CRYP_SR_IFNF             (1 << 1)
#define 	CRYP_SR_OFNE             (1 << 2)
#define 	CRYP_SR_OFFU             (1 << 3)
#define 	CRYP_SR_BUSY             (1 << 4)
#define 	CRYP_DMACR_DIEN          (1 << 0)
#define 	CRYP_DMACR_DOEN          (1 << 1)
#define 	CRYP_IMSCR_INIM          (1 << 0)
#define 	CRYP_IMSCR_OUTIM         (1 << 1)
#define 	CRYP_RISR_INRIS          (1 << 0)
#define 	CRYP_RISR_OUTRIS         (1 << 0)
#define 	CRYP_MISR_INMIS          (1 << 0)
#define 	CRYP_MISR_OUTMIS         (1 << 0)


typedef struct {
  volatile unsigned int CR;
  volatile unsigned int SR;
  volatile unsigned int DIN;
  volatile unsigned int DOUT;
  volatile unsigned int DMACR;
  volatile unsigned int IMSCR;
  volatile unsigned int RISR;
  volatile unsigned int MISR;
  volatile unsigned int K0LR;
  volatile unsigned int K0RR;
  volatile unsigned int K1LR;
  volatile unsigned int K1RR;
  volatile unsigned int K2LR;
  volatile unsigned int K2RR;
  volatile unsigned int K3LR;
  volatile unsigned int K3RR;
  volatile unsigned int IV0LR;
  volatile unsigned int IV0RR;
  volatile unsigned int IV1LR;
  volatile unsigned int IV1RR;
} CRYP_Regs;

#define PERIPH_BASE_AHB2 (0x50000000U)
#define CRYP_BASE ((CRYP_Regs*) (PERIPH_BASE_AHB2 + 0x60000))

void test(void) {
  CRYP_Regs *cryp = CRYP_BASE;
  const uint32_t IV[4] = {0x01234567, 0x89abcdef, 0x01234567, 0x89abcdef};
  const uint32_t K[4] = {0xaaaaaaaa, 0x55555555, 0xffffffff, 0x00000000};
  const uint8_t msg[] = "0123456789abcdef";
  uint8_t ct[4*16];
  int i, blocks;

  MMIO32(RCC_AHB2ENR) |= RCC_AHB2Periph_CRYP;

  cryp->CR &= ~((uint32_t) CRYP_CR_CRYPEN);
  while(cryp->SR & CRYP_SR_BUSY); // block until not busy
  cryp->CR |= CRYP_AlgoMode_AES_CBC | CRYP_KeySize_128b | MODE_ENCRYPT | CRYP_DataType_32b;

  cryp->K3LR = K[3];
  cryp->K3RR = K[2];
  cryp->K2LR = K[1];
  cryp->K2RR = K[0];

  cryp->IV0LR = IV[0];
  cryp->IV0RR = IV[1];
  cryp->IV1LR = IV[2];
  cryp->IV1RR = IV[3];

  cryp->CR |= CRYP_CR_FFLUSH;

  /*
     1. Enable the cryptographic processor by setting the CRYPEN bit in the CRYP_CR
        register.
     2. Write the first blocks in the input FIFO (2 to 8 words).

     Repeat the following sequence until the complete message has been processed:

     a) Wait for OFNE=1, then read the OUT-FIFO (1 block or until the FIFO is empty)
     b) Wait for IFNF=1, then write the IN FIFO (1 block or until the FIFO is full)

     At the end of the processing, BUSY=0 and both FIFOs are empty (IFEM=1 and
     OFNE=0). You can disable the peripheral by clearing the CRYPEN bit.
  */
  rsp_dump((uint8_t*) msg, 16);
  cryp->CR |= CRYP_CR_CRYPEN;
  uint32_t *out = (uint32_t*) ct;
  for(blocks=4;blocks>0;blocks--) {
    uint32_t *in = (uint32_t*) msg;
    for(i=0;i<16;i++) cryp->DIN = in[i%4]; // this fails
  rsp_dump((uint8_t*)&(cryp->SR),4);
    while((cryp->SR & CRYP_SR_OFNE)==0); // block until output not empty
  //rsp_dump((uint8_t*)&(cryp->SR),4);
    for(i=0;i<4;i++) {
  //rsp_dump((uint8_t*)"zxcv",4);
      *out++ = cryp->DOUT;
    }
  //rsp_dump((uint8_t*)"hjkl",4);
    while(cryp->SR & CRYP_SR_IFNF); // block until input not full
  //rsp_dump((uint8_t*)"yuio",4);
  }
  cryp->CR &= ~((uint32_t) CRYP_CR_CRYPEN);
  rsp_dump((uint8_t*) ct, 16*4);

  //rsp_detach();
  rsp_finish();
}
