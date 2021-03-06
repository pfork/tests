#include "newhope.h"
#include "poly.h"
#include "../../../crypto/randombytes_salsa20_random.h"
#include "error_correction.h"
#include <string.h>
#include "../../rsp.h"
#include "oled.h"

void test_keys(){
  poly sk_a;
  u8 key_b[32], key_a[32];
  u8 senda[POLY_BYTES];
  u8 sendb[POLY_BYTES];

  // rotate screen 180
  oled_cmd(0xa1);
  oled_cmd(0xc8);
  oled_clear();

  //Alice generates a public key
  oled_print(0,0, "keygen", Font_8x8);
  newhope_keygen(senda, &sk_a);
  oled_print(0,0, "keygen ok", Font_8x8);

  //Bob derives a secret key and creates a response
  oled_print(0,9, "sharedb", Font_8x8);
  newhope_sharedb(key_b, sendb, senda);
  oled_clear();
  oled_print(0,0, "keygen ok", Font_8x8);
  oled_print(0,9, "sharedb ok", Font_8x8);
  oled_print(0,18, "shareda", Font_8x8);
  newhope_shareda(key_a, &sk_a, sendb);
  oled_clear();
  oled_print(0,0, "keygen ok", Font_8x8);
  oled_print(0,9, "sharedb ok", Font_8x8);
  oled_print(0,18, "shareda ok", Font_8x8);

  if(memcmp(key_a, key_b, 32)!=0)
     rsp_dump((uint8_t*) "ERROR5", 6);
  else
    oled_print(0,27, "pqkex ok", Font_8x8);
}

void test(void){
  test_keys();
  rsp_detach();
}
