#include <stdint.h>
#include <string.h>

#include "oled.h"
#include "stm32f.h"
#include "pbkdf2_generichash.h"

void * __stack_chk_guard = NULL;

void __stack_chk_guard_setup() {
  __stack_chk_guard = (void*) 0xdeadb33f;
}

void __attribute__((noreturn)) __stack_chk_fail() {
  for(;;);
}

void test(void) {
  const uint8_t password[]="hello world";
  volatile uint8_t mk[32];
  uint8_t *salt=(uint8_t*) (OTP_START_ADDR + 2 * OTP_BYTES_IN_BLOCK);
  oled_clear();
  oled_print(0,0,"start pbkdf 1000", Font_8x8);
  pbkdf2_generichash((uint8_t*) mk, password, strlen((char*) password), salt);
  oled_print(0,9,"end pbkdf 1000", Font_8x8);
  while(1);
}
