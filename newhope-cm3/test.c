#include "newhope.h"
#include "poly.h"
#include "randombytes_pitchfork.h"
#include "error_correction.h"
#include <string.h>
#include "oled.h"
#include "poly.h"

void * __stack_chk_guard = NULL;

void __stack_chk_guard_setup() {
  __stack_chk_guard = (void*) 0xdeadb33f;
}

void __attribute__((noreturn)) __stack_chk_fail() {
  for(;;);
}

int test_keys(){
  poly sk_a;
  uint8_t key_b[32], key_a[32];
  uint8_t senda[POLY_BYTES];
  uint8_t sendb[POLY_BYTES];

  oled_clear();

  //Alice generates a public key
  //oled_print(0,0, "keygen", Font_8x8);
  newhope_keygen(senda, &sk_a);
  //oled_print(0,0, "keygen ok", Font_8x8);

  //Bob derives a secret key and creates a response
  //oled_print(0,9, "sharedb", Font_8x8);
  newhope_sharedb(key_b, sendb, senda);
  //oled_clear();
  //oled_print(0,0, "keygen ok", Font_8x8);
  //oled_print(0,9, "sharedb ok", Font_8x8);
  //oled_print(0,18, "shareda", Font_8x8);
  newhope_shareda(key_a, &sk_a, sendb);
  //oled_clear();
  //oled_print(0,0, "keygen ok", Font_8x8);
  //oled_print(0,9, "sharedb ok", Font_8x8);
  //oled_print(0,18, "shareda ok", Font_8x8);

  if(memcmp(key_a, key_b, 32)!=0)
     return 1;
    //oled_print(0,27, "pqkex err", Font_8x8);
  else
     return 0;
    //oled_print(0,27, "pqkex ok", Font_8x8);
}

void test(void){
  test_keys();
  while(1);
}
