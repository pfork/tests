#include <stdint.h>
#include <string.h>
#include "oled.h"
#include "irq.h"
#include "ntohex.h"
#include "../rsp.h"
#include "libopencm3/stm32/flash.h"

#define OTP_LOCK_ADDR	(0x1FFF7A00)
#define OTP_START_ADDR	(0x1FFF7800)
#define OTP_BYTES_IN_BLOCK	32
#define OTP_BLOCKS	16

// todo these should come from the otp bytes
const unsigned char pk[]= {0xe2,0x97,0x55,0x21,0x53,0x9f,0xb2,0xa3, 0xfd,0x49,0xef,0xba,0xc4,0xd9,0x2e,0x14,
                           0x4d,0x2f,0x18,0x47,0x8d,0x93,0x21,0x44, 0x9c,0xca,0x4d,0x75,0x27,0x18,0x12,0xd6 };

static char read_otp(uint32_t block, uint32_t byte) {
  return *(char*)(OTP_START_ADDR + block * OTP_BYTES_IN_BLOCK + byte);
}

void test_otp(void) {
  oled_clear();
  int i;
  for(i=0;i<32;i++) {
    char b=read_otp(0,i);
    uint8_t bs[3]="  ";
    ntohex(bs,b);
    ntohex(bs+1,b>>4);
    oled_print(i%8*16,(i/8)*8,(char*)bs,Font_8x8);
  }
}

void test(void) {
  int block=0; // only write in the 1st block
  test_otp();
  int i;

  disable_irqs();

  flash_unlock();
  flash_clear_status_flags();
  // write pk into OTP block0
  for(i=0;i<sizeof(pk);i++) {
    flash_program_byte((OTP_START_ADDR + block * OTP_BYTES_IN_BLOCK + i), pk[i]);
  }
  // lock the 1st block
  flash_program_byte(OTP_LOCK_ADDR + block, 0x00);
  flash_lock();

  rsp_detach();
  rsp_finish();
}
