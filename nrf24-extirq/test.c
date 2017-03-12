#include <libopencm3/usb/usbd.h>
#include "stm32f.h"
#include "nrf24l01.h"
#include "pitchfork.h"
#include <string.h>
#include <delay.h>
#include <led.h>
#include "../rsp.h"

#define CHANNEL 81

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
void btn3_IRQHandler(void) {
  keypressed=1;
  EXTI_Regs *exticfg = (EXTI_Regs*) EXTI_BASE;
  exticfg->PR |= EXTI_PR_PR8;
}

struct {
  uint8_t rx;
} nrf_ctx;

volatile uint8_t netevent=0;
void nrf_IRQHandler(void) {
  netevent=1;
  volatile unsigned char status=0;
  status = nrf_read_reg(STATUS);
  rsp_dump((uint8_t*) &status,1);
  if(nrf_ctx.rx == 1) {
    // RX mode
  } else {
    // TX mode
  }
  EXTI_Regs *exticfg = (EXTI_Regs*) EXTI_BASE;
  exticfg->PR |= EXTI_PR_PR4;
}

void initirq(void) {
// btn3 == pa8
// nrf_irq = pa4
  SYSCFG_Regs *syscfg = (SYSCFG_Regs *) SYSCFG_BASE;
  syscfg->EXTICR[1] |= SYSCFG_EXTICR2_EXTI4_PA;
  syscfg->EXTICR[2] |= SYSCFG_EXTICR3_EXTI8_PA;

  EXTI_Regs *exticfg = (EXTI_Regs*) EXTI_BASE;
  exticfg->IMR |= EXTI_IMR_MR8 | EXTI_IMR_MR4;
  exticfg->RTSR &= ~(EXTI_RTSR_TR8|EXTI_RTSR_TR4);
  exticfg->FTSR |= (EXTI_FTSR_TR8|EXTI_FTSR_TR4);

  nvic_table[26]=(uint32_t) nrf_IRQHandler; // exti4
  nvic_table[39]=(uint32_t) btn3_IRQHandler; // exti5-9

  // enable btn3 irq
  NVIC_IPR(EXTI9_5_IRQn) = 0;
  irq_enable(EXTI9_5_IRQn);

  //enable nrf irq
  NVIC_IPR(EXTI4_IRQn) = 0;
  irq_enable(EXTI4_IRQn);
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
  netevent=0;

  move_vtor(nvic_table);
  initirq();
  while(!keypressed);

  //rsp_detach();
  //rsp_dump((uint8_t*) "Listening", 9);
  while(1) {
    //receive
    //rsp_dump((uint8_t*) "Listening", 9);
    nrf_ctx.rx = 1;
    nrf_open(CHANNEL, (unsigned char*) "\x1\x2\x3\x5\x5", 1);
    listen();
    // send
    //rsp_dump((uint8_t*) "sending", 7);
    nrf_ctx.rx = 0;
    nrf_open(CHANNEL, (unsigned char*) "\x1\x2\x3\x5\x5", 0);
    nrf_send((unsigned char*) "it worksA!!5!\x0                  ",32);
    rsp_detach();
  }
  rsp_finish();
}
