#include "stm32f.h"
#include "../rsp.h"

#define ALIGN(x) __attribute__((aligned(x)))
ALIGN(128) uint32_t nvic_table[512];

/*
 *
 *  Vector table. If the program changes an entry in the vector table, and then
 *  enables the corresponding exception, use a DMB instruction between the
 *  operations. This ensures that if the exception is taken immediately after
 *  being enabled the processor uses the new exception vector.
 *
 */

void move_vtor(uint32_t* dest) {
  if(((unsigned long)dest & 127) != 0) {
    rsp_dump((uint8_t*) "fail!!!", 7);
    return; // must be 128 aligned
  }
  size_t i;
  uint32_t *src = (uint32_t*) SCB->VTOR;
  if(src == dest) return;
  //rsp_dump((unsigned char*) src, 512);
  for(i=0;i<512;i++) {
    dest[i] = src[i];
  }
  //rsp_dump((unsigned char*) dest, 512);
  SCB->VTOR = (uint32_t) dest;
}

volatile uint8_t keypressed=0;
void myEXTI9_5_IRQHandler(void) {
  keypressed=1;
  EXTI_Regs *exticfg = (EXTI_Regs*) EXTI_BASE;
  exticfg->PR |= EXTI_PR_PR8;
}

void initirq(void) {
// btn3 == pa8
  SYSCFG_Regs *syscfg = (SYSCFG_Regs *) SYSCFG_BASE;
  syscfg->EXTICR[3] |= SYSCFG_EXTICR3_EXTI8_PA;
  EXTI_Regs *exticfg = (EXTI_Regs*) EXTI_BASE;
  exticfg->IMR |= EXTI_IMR_MR8;
  exticfg->RTSR &= ~EXTI_RTSR_TR8;
  exticfg->FTSR |= EXTI_FTSR_TR8;

  nvic_table[39]=(uint32_t) myEXTI9_5_IRQHandler;

  NVIC_IPR(EXTI9_5_IRQn) = 0;
  irq_enable(EXTI9_5_IRQn);
}

volatile int i=0;
static void listen() {
  int len = 0;
  //nrf_recv(outbuf, PLOAD_WIDTH);
  if((len=nrf_recv(outbuf, PLOAD_WIDTH))>0) {
    rsp_dump(outbuf, len);
    if(i<20) i++;
  }
  if(i==20) {
    rsp_detach();
    i=21;
  }
}

void test(void) {
  nrf24_init();
  i=0;
  keypressed=0;

  move_vtor(nvic_table);
  initirq();
  while(!keypressed);

  rsp_detach();
}
