#include <stdint.h>
#include "pitchfork.h"
#include "stm32f.h"
#include "dma.h"
#include "../rsp.h"

#define BUFSIZE 64

// dma2, stream0, channel1
#define DMACPY_CHANNEL DMA_SxCR_CHSEL_1
#define DMACPY_FLAG_TCIF DMA_LISR_TCIF0
#define DMACPY_STREAM_REGS ((DMA_Stream_Regs *) DMA2_Stream0_BASE)

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
}

static void dma(void* dest, const void *buf, unsigned short len, unsigned int cfg) {
  DMA_Stream_Regs *regs = DMACPY_STREAM_REGS;
  /* DMA Config */
  regs->CR = cfg;
  // setup DMA
  /* memory source address */
  regs->PAR = (uint32_t) buf;
  regs->NDTR = len;
  /* dest address */
  regs->M0AR = (uint32_t) dest;
  /* DMA enable */
  regs->CR |= DMA_SxCR_EN;
}

void dmacpy32(void* dest, const void *buf, unsigned short len) {
  dma(dest, buf, len, (DMACPY_CHANNEL | DMA_SxCR_DIR_MEM_TO_MEM |
                          DMA_SxCR_MINC | DMA_SxCR_PINC |
                          DMA_SxCR_PSIZE_32BIT | DMA_SxCR_MSIZE_32BIT |
                          //DMA_SxCR_TCIE | DMA_SxCR_HTIE | DMA_SxCR_TEIE | DMA_SxCR_DMEIE |/* enable tx complete interrupt */
                          DMA_SxCR_TCIE |
                          DMA_SxCR_PL_VERY_HIGH ));
}

void dmacpy16(void* dest, const void *buf, unsigned short len) {
  dma(dest, buf, len, (DMACPY_CHANNEL | DMA_SxCR_DIR_MEM_TO_MEM |
                          DMA_SxCR_MINC | DMA_SxCR_PINC |
                          DMA_SxCR_PSIZE_16BIT | DMA_SxCR_MSIZE_16BIT |
                          DMA_SxCR_TCIE |
                          DMA_SxCR_PL_VERY_HIGH ));
}

void dmacpy8(void* dest, const void *buf, unsigned short len) {
  dma(dest, buf, len, (DMACPY_CHANNEL | DMA_SxCR_DIR_MEM_TO_MEM |
                          DMA_SxCR_MINC | DMA_SxCR_PINC |
                          DMA_SxCR_PSIZE_8BIT | DMA_SxCR_MSIZE_8BIT |
                          DMA_SxCR_TCIE |
                          DMA_SxCR_PL_VERY_HIGH ));
}

void dmazero32(void* dest, const int val, unsigned short len) {
  dma(dest, &val, len, (DMACPY_CHANNEL | DMA_SxCR_DIR_MEM_TO_MEM |
                        DMA_SxCR_MINC | 
                        DMA_SxCR_PSIZE_32BIT | DMA_SxCR_MSIZE_32BIT |
                        DMA_SxCR_TCIE |
                        DMA_SxCR_PL_VERY_HIGH ));
}

void dmazero16(void* dest, const int val, unsigned short len) {
  dma(dest, &val, len, (DMACPY_CHANNEL | DMA_SxCR_DIR_MEM_TO_MEM |
                        DMA_SxCR_MINC | 
                        DMA_SxCR_PSIZE_16BIT | DMA_SxCR_MSIZE_16BIT |
                        DMA_SxCR_TCIE |
                        DMA_SxCR_PL_VERY_HIGH ));
}

void dmazero8(void* dest, const int val, unsigned short len) {
  dma(dest, &val, len, (DMACPY_CHANNEL | DMA_SxCR_DIR_MEM_TO_MEM |
                        DMA_SxCR_MINC | 
                        DMA_SxCR_PSIZE_8BIT | DMA_SxCR_MSIZE_8BIT |
                        DMA_SxCR_TCIE |
                        DMA_SxCR_PL_VERY_HIGH ));
}

void dmawait(void) {
  while((DMA2_LISR & DMACPY_FLAG_TCIF) == 0);

  DMA2_LIFCR |= DMA_LIFCR_CFEIF0 | DMA_LIFCR_CDMEIF0 |
                DMA_LIFCR_CTEIF0 | DMA_LIFCR_CHTIF0 |
                DMA_LIFCR_CTCIF0;
}

int cmp(const void * a, const void *b, const size_t size) {
  const unsigned char *_a = (const unsigned char *) a;
  const unsigned char *_b = (const unsigned char *) b;
  unsigned char result = 0;
  size_t i;

  for (i = 0; i < size; i++) {
    result |= _a[i] ^ _b[i];
  }

  return result; /* returns 0 if equal, nonzero otherwise */
}

void test(void) {
  unsigned char buf[BUFSIZE];
  unsigned char dest[BUFSIZE];
  unsigned char i, j=0;

  rsp_dump((uint8_t*) "letsgo", 6);
  dmacpy_init();
  for(i=0;i<BUFSIZE;i++) buf[i]=i;
  while(1) {
    dmacpy32(dest,buf,BUFSIZE);
    dmawait();

    if(cmp(dest, buf,BUFSIZE) == 0) {
      if(j>30) {
        rsp_dump((uint8_t*) "yippie", 6);
        rsp_detach();
      }
    } else {
      rsp_dump((uint8_t*) "ohnoez", 6);
      rsp_dump((uint8_t*) dest, 64);
      rsp_dump((uint8_t*) buf, 64);
      rsp_detach();
    }
    dmazero8(dest,0,BUFSIZE);
    for(i=0;i<BUFSIZE;i++) buf[i]++;
    j++;

    // wait for dmazero to finish - which it already should be
    dmawait();
    for(i=0;i<BUFSIZE;i++) if(dest[i]!=0) {
        rsp_dump((uint8_t*) "nozero", 6);
        rsp_detach();
      };
  }
}
