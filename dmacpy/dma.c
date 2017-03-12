#include <libopencm3/usb/usbd.h>
#include <libopencm3/stm32/otg_fs.h>
#include <libopencm3/stm32/otg_hs.h>
#include "usb_private.h"
#include "stm32f.h"
#include "dma.h"
#include "usb.h"

#define USB_DMA_STREAM	             1
#define USB_DMA_PORT                 DMA2
#define USB_DMA_CLK                  RCC_AHB1Periph_DMA2
#define USB_DMA_IRQn                 NVIC_DMA2_STREAM1_IRQ
#define USB_DMA_STREAM_REGS      	 ((DMA_Stream_Regs *) DMA2_Stream1_BASE)
#define USB_DMA_CHANNEL          	 DMA_SxCR_CHSEL_0
#define USB_DMA_FLAG_TCIF            DMA_FLAG_TCIF1

#define dev_base_address (usbd_dev->driver->base_address)
#define REBASE(x)        MMIO32((x) + (dev_base_address))
#define REBASE_FIFO(x)   (&MMIO32((dev_base_address) + (OTG_FIFO(x))))
#define OTG_FIFO(x)			(((x) + 1) << 12)
#define OTG_DOEPCTL(x)			(0xB00 + 0x20*(x))
#define OTG_DOEPTSIZ(x)			(0xB10 + 0x20*(x))

unsigned char addr;
char out;

void usb_dma_init(void) {
  RCC->AHB1ENR |= USB_DMA_CLK;
  //NVIC_IPR(USB_DMA_IRQn) = 0;
  //irq_enable(USB_DMA_IRQn);
}

void usb_dma_open(unsigned int _dir, unsigned char endpoint) {
  out = 1;
  addr = endpoint;

  /* DMA disable */
  DMA_SCR(USB_DMA_PORT, USB_DMA_STREAM) &= ~((unsigned int) DMA_SxCR_EN);
  /* wait till cleared */
  while(DMA_SCR(USB_DMA_PORT, USB_DMA_STREAM) & ((unsigned int) DMA_SxCR_EN));
  /* DMA deConfig */
  DMA_DeInit(USB_DMA_STREAM_REGS);
  // setup DMA
  DMA_SPAR(USB_DMA_PORT, USB_DMA_STREAM) =  (void*) REBASE_FIFO(addr & 0x7F); // peripheral (source) address
  /* DMA Config */
  DMA_SCR(USB_DMA_PORT, USB_DMA_STREAM) |= (USB_DMA_CHANNEL | out |
                                            DMA_SxCR_MINC | DMA_SxCR_PINC |
                                            DMA_SxCR_PSIZE_32BIT | DMA_SxCR_MSIZE_32BIT |
                                            /*DMA_SxCR_PBURST_INCR4 | DMA_SxCR_MBURST_INCR4 |*/
                                            DMA_SxCR_PL_VERY_HIGH );
}

void dmacpy(void* dest, const void *buf, unsigned short len) {
  /* DMA disable */
  DMA_SCR(USB_DMA_PORT, USB_DMA_STREAM) &= ~((unsigned int) DMA_SxCR_EN);
  /* wait till cleared */
  while(DMA_SCR(USB_DMA_PORT, USB_DMA_STREAM) & ((unsigned int) DMA_SxCR_EN));
  /* DMA deConfig */
  DMA_DeInit(USB_DMA_STREAM_REGS);
  // setup DMA
  /* memory source address */
  DMA_SPAR(USB_DMA_PORT, USB_DMA_STREAM) = (void*) buf;
  DMA_SNDTR(USB_DMA_PORT, USB_DMA_STREAM) = len;
  /* dest address */
  DMA_SM0AR(USB_DMA_PORT, USB_DMA_STREAM) =  dest;
  /* DMA Config */
  DMA_SCR(USB_DMA_PORT, USB_DMA_STREAM) |= (USB_DMA_CHANNEL | DMA_SxCR_DIR_MEM_TO_MEM |
                                            DMA_SxCR_MINC | DMA_SxCR_PINC |
                                            DMA_SxCR_PSIZE_32BIT | DMA_SxCR_MSIZE_32BIT |
                                            DMA_SxCR_TCIE | DMA_SxCR_HTIE | DMA_SxCR_TEIE | DMA_SxCR_DMEIE |/* enable tx complete interrupt */
                                            /*DMA_SxCR_PBURST_INCR4 | DMA_SxCR_MBURST_INCR4 |*/
                                            DMA_SxCR_PL_VERY_HIGH );

  /* DMA enable */
  DMA_SCR(USB_DMA_PORT, USB_DMA_STREAM) |= DMA_SxCR_EN;
}

unsigned short usb_dma_write(const void *buf, unsigned short len) {
  /* Return if endpoint is already enabled. */
  if (REBASE(OTG_DIEPTSIZ(USB_CRYPTO_EP_DATA_OUT & 0x7F)) & OTG_FS_DIEPSIZ0_PKTCNT) {
    return 0;
  }

  out = 0;

  /* Enable endpoint for transmission. */
  REBASE(OTG_DIEPTSIZ(USB_CRYPTO_EP_DATA_OUT & 0x7F)) = OTG_FS_DIEPSIZ0_PKTCNT | len;
  REBASE(OTG_DIEPCTL(USB_CRYPTO_EP_DATA_OUT & 0x7F)) |= OTG_FS_DIEPCTL0_EPENA | OTG_FS_DIEPCTL0_CNAK;

  dmacpy((void*) REBASE_FIFO(USB_CRYPTO_EP_DATA_OUT & 0x7F), buf, len);

  return len;
}

void usbdma_irq_handler(void) {
  /* reset irq flags */
#if USB_DMA_STREAM < 4
  DMA_LIFCR(USB_DMA_PORT) = DMA_ISR_MASK(USB_DMA_STREAM);
#else
  DMA_HIFCR(USB_DMA_PORT) = DMA_ISR_MASK(USB_DMA_STREAM);
#endif

  /* DMA disable */
  DMA_SCR(USB_DMA_PORT, USB_DMA_STREAM) &= ~((unsigned int) DMA_SxCR_EN);

  if(out == 1) { // reading from usb
    /* signal read finish to usb */
    REBASE(OTG_DOEPTSIZ(addr)) = usbd_dev->doeptsiz[addr];
    REBASE(OTG_DOEPCTL(addr)) |= OTG_FS_DOEPCTL0_EPENA |
      (usbd_dev->force_nak[addr] ?
       OTG_FS_DOEPCTL0_SNAK : OTG_FS_DOEPCTL0_CNAK);
  }
}

unsigned short usb_dma_read(void *buf, unsigned short len) {
  len = MIN(len, usbd_dev->rxbcnt);
  usbd_dev->rxbcnt -= len;

  usb_dma_open(DMA_SxCR_DIR_MEM_TO_MEM, USB_CRYPTO_EP_DATA_IN);

  DMA_SM0AR(USB_DMA_PORT, USB_DMA_STREAM) = (void*) buf;         // memory (desination) address
  DMA_SNDTR(USB_DMA_PORT, USB_DMA_STREAM) = len;
  /* DMA enable */
  DMA_SCR(USB_DMA_PORT, USB_DMA_STREAM) |= DMA_SxCR_EN;
  return len;
}

unsigned int usb_dma_status(void) {
  return (unsigned int)DMA_GetFlagStatus(USB_DMA_STREAM_REGS, ~0);
}

unsigned int usb_dma_endoftransfer(void) {
  return (unsigned int)DMA_GetFlagStatus(USB_DMA_STREAM_REGS, USB_DMA_FLAG_TCIF);
}

/* unsigned short usb_dma_write(const void *buf, unsigned short len) { */
/*   /\* Return if endpoint is already enabled. *\/ */
/*   if (REBASE(OTG_DIEPTSIZ(USB_CRYPTO_EP_DATA_OUT & 0x7F)) & OTG_FS_DIEPSIZ0_PKTCNT) { */
/*     return 0; */
/*   } */

/*   out = 0; */

/*   /\* DMA disable *\/ */
/*   DMA_SCR(USB_DMA_PORT, USB_DMA_STREAM) &= ~((unsigned int) DMA_SxCR_EN); */
/*   /\* wait till cleared *\/ */
/*   while(DMA_SCR(USB_DMA_PORT, USB_DMA_STREAM) & ((unsigned int) DMA_SxCR_EN)); */
/*   /\* DMA deConfig *\/ */
/*   DMA_DeInit(USB_DMA_STREAM_REGS); */
/*   // setup DMA */
/*   /\* memory source address *\/ */
/*   DMA_SPAR(USB_DMA_PORT, USB_DMA_STREAM) = (void*) buf; */
/*   DMA_SNDTR(USB_DMA_PORT, USB_DMA_STREAM) = len; */
/*   /\* dest address *\/ */
/*   DMA_SM0AR(USB_DMA_PORT, USB_DMA_STREAM) =  (void*) REBASE_FIFO(USB_CRYPTO_EP_DATA_OUT & 0x7F); */
/*   /\* DMA Config *\/ */
/*   DMA_SCR(USB_DMA_PORT, USB_DMA_STREAM) |= (USB_DMA_CHANNEL | DMA_SxCR_DIR_MEM_TO_MEM | */
/*                                             DMA_SxCR_MINC | DMA_SxCR_PINC | */
/*                                             DMA_SxCR_PSIZE_32BIT | DMA_SxCR_MSIZE_32BIT | */
/*                                             DMA_SxCR_TCIE | DMA_SxCR_HTIE | DMA_SxCR_TEIE | DMA_SxCR_DMEIE |/\* enable tx complete interrupt *\/ */
/*                                             /\*DMA_SxCR_PBURST_INCR4 | DMA_SxCR_MBURST_INCR4 |*\/ */
/*                                             DMA_SxCR_PL_VERY_HIGH ); */

/*   /\* Enable endpoint for transmission. *\/ */
/*   REBASE(OTG_DIEPTSIZ(USB_CRYPTO_EP_DATA_OUT & 0x7F)) = OTG_FS_DIEPSIZ0_PKTCNT | len; */
/*   REBASE(OTG_DIEPCTL(USB_CRYPTO_EP_DATA_OUT & 0x7F)) |= OTG_FS_DIEPCTL0_EPENA | OTG_FS_DIEPCTL0_CNAK; */

/*   /\* DMA enable *\/ */
/*   DMA_SCR(USB_DMA_PORT, USB_DMA_STREAM) |= DMA_SxCR_EN; */

/*   return len; */
/* } */
